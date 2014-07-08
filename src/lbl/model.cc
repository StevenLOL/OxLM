#include "lbl/model.h"

#include <iomanip>

#include <boost/make_shared.hpp>
#include <boost/archive/binary_oarchive.hpp>
#include <boost/serialization/shared_ptr.hpp>

#include "lbl/factored_metadata.h"
#include "lbl/factored_maxent_metadata.h"
#include "lbl/factored_weights.h"
#include "lbl/global_factored_maxent_weights.h"
#include "lbl/metadata.h"
#include "lbl/minibatch_factored_maxent_weights.h"
#include "lbl/model_utils.h"
#include "lbl/operators.h"
#include "lbl/weights.h"
#include "utils/conditional_omp.h"

namespace oxlm {

template<class GlobalWeights, class MinibatchWeights, class Metadata>
Model<GlobalWeights, MinibatchWeights, Metadata>::Model(ModelData& config)
    : config(config) {
  metadata = boost::make_shared<Metadata>(config, dict);
}

template<class GlobalWeights, class MinibatchWeights, class Metadata>
void Model<GlobalWeights, MinibatchWeights, Metadata>::learn() {
  // Initialize the dictionary now, if it hasn't been initialized when the
  // vocabulary was partitioned in classes.
  bool immutable_dict = config.classes > 0 || config.class_file.size();
  boost::shared_ptr<Corpus> training_corpus =
      readCorpus(config.training_file, dict, immutable_dict);
  config.vocab_size = dict.size();
  cout << "Done reading training corpus..." << endl;

  boost::shared_ptr<Corpus> test_corpus;
  if (config.test_file.size()) {
    test_corpus = readCorpus(config.test_file, dict);
    cout << "Done reading test corpus..." << endl;
  }

  metadata->initialize(training_corpus);
  weights = boost::make_shared<GlobalWeights>(
      config, metadata, training_corpus);

  vector<int> indices(training_corpus->size());
  iota(indices.begin(), indices.end(), 0);

  Real best_perplexity = numeric_limits<Real>::infinity();
  Real global_objective = 0, test_objective = 0;
  boost::shared_ptr<MinibatchWeights> global_gradient;
  boost::shared_ptr<GlobalWeights> adagrad =
      boost::make_shared<GlobalWeights>(config, metadata);

  omp_set_num_threads(config.threads);
  #pragma omp parallel
  {
    int minibatch_counter = 1;
    int minibatch_size = config.minibatch_size;
    for (int iter = 0; iter < config.iterations; ++iter) {
      auto iteration_start = GetTime();

      #pragma omp master
      {
        if (config.randomise) {
          random_shuffle(indices.begin(), indices.end());
        }
        global_objective = 0;
      }
      // Wait until the master threads finishes shuffling the indices.
      #pragma omp barrier

      size_t start = 0;
      while (start < training_corpus->size()) {
        size_t end = min(training_corpus->size(), start + minibatch_size);

        #pragma omp master
        {
          vector<int> minibatch(indices.begin() + start, indices.begin() + end);
          global_gradient = boost::make_shared<MinibatchWeights>(
              config, metadata, minibatch);
        }

        // Wait until the global gradient is reset to 0. Otherwise, some
        // gradient updates may be ignored if the global gradient is reset
        // afterwards.
        #pragma omp barrier

        vector<int> minibatch = scatterMinibatch(start, end, indices);
        Real objective;
        boost::shared_ptr<MinibatchWeights> gradient =
            weights->getGradient(training_corpus, minibatch, objective);

        #pragma omp critical
        {
          global_gradient->update(gradient);
          global_objective += objective;
        }

        #pragma omp barrier

        #pragma omp master
        {
          update(global_gradient, adagrad);

          Real minibatch_factor =
              static_cast<Real>(end - start) / training_corpus->size();
          global_objective += regularize(global_gradient, minibatch_factor);
        }

        // Wait for master thread to update model.
        #pragma omp barrier
        if ((minibatch_counter % 100 == 0 && minibatch_counter <= 1000) ||
            minibatch_counter % 1000 == 0) {
          evaluate(test_corpus, iteration_start, minibatch_counter,
                   test_objective, best_perplexity);
        }

        ++minibatch_counter;
        start = end;
      }

      evaluate(test_corpus, iteration_start, minibatch_counter,
               test_objective, best_perplexity);
      #pragma omp master
      {
        Real iteration_time = GetDuration(iteration_start, GetTime());
        cout << "Iteration: " << iter << ", "
             << "Time: " << iteration_time << " seconds, "
             << "Objective: " << global_objective / training_corpus->size()
             << endl;
        cout << endl;
      }
    }
  }

  cout << "Overall minimum perplexity: " << best_perplexity << endl;
}

template<class GlobalWeights, class MinibatchWeights, class Metadata>
void Model<GlobalWeights, MinibatchWeights, Metadata>::update(
    const boost::shared_ptr<MinibatchWeights>& global_gradient,
    const boost::shared_ptr<GlobalWeights>& adagrad) {
  adagrad->updateSquared(global_gradient);
  weights->updateAdaGrad(global_gradient, adagrad);
}

template<class GlobalWeights, class MinibatchWeights, class Metadata>
Real Model<GlobalWeights, MinibatchWeights, Metadata>::regularize(
    const boost::shared_ptr<MinibatchWeights>& global_gradient,
    Real minibatch_factor) {
  return weights->regularizerUpdate(global_gradient, minibatch_factor);
}

template<class GlobalWeights, class MinibatchWeights, class Metadata>
void Model<GlobalWeights, MinibatchWeights, Metadata>::evaluate(
    const boost::shared_ptr<Corpus>& test_corpus, const Time& iteration_start,
    int minibatch_counter, Real& objective, Real& best_perplexity) const {
  if (test_corpus != nullptr) {
    #pragma omp master
    {
      cout << "Calculating perplexity for " << test_corpus->size()
           << " tokens..." << endl;
      objective = 0;
    }

    // Each thread must wait until the perplexity is set to 0.
    // Otherwise, partial results might get overwritten.
    #pragma omp barrier

    vector<int> indices(test_corpus->size());
    iota(indices.begin(), indices.end(), 0);
    vector<int> minibatch = scatterMinibatch(0, indices.size(), indices);

    Real local_objective = weights->getObjective(test_corpus, minibatch);
    #pragma omp critical
    objective += local_objective;

    // Wait for all the threads to compute the perplexity for their slice of
    // test data.
    #pragma omp barrier
    #pragma omp master
    {
      Real perplexity = exp(objective / test_corpus->size());
      Real iteration_time = GetDuration(iteration_start, GetTime());
      cout << "\tMinibatch " << minibatch_counter << ", "
           << "Time: " << GetDuration(iteration_start, GetTime()) << " seconds, "
           << "Test Perplexity: " << perplexity << endl;

      if (perplexity < best_perplexity) {
        best_perplexity = perplexity;
        save();
      }
    }
  } else {
    #pragma omp master
    save();
  }
}

template<class GlobalWeights, class MinibatchWeights, class Metadata>
void Model<GlobalWeights, MinibatchWeights, Metadata>::save() const {
  if (config.model_output_file.size()) {
    cout << "Writing model to " << config.model_output_file << "..." << endl;
    ofstream fout(config.model_output_file);
    boost::archive::binary_oarchive oar(fout);
    oar << config;
    oar << dict;
    oar << weights;
    oar << metadata;
    cout << "Done..." << endl;
  }
}

template<class GlobalWeights, class MinibatchWeights, class Metadata>
Dict Model<GlobalWeights, MinibatchWeights, Metadata>::getDict() const {
  return dict;
}

template class Model<Weights, Weights, Metadata>;
template class Model<FactoredWeights, FactoredWeights, FactoredMetadata>;
template class Model<GlobalFactoredMaxentWeights, MinibatchFactoredMaxentWeights, FactoredMaxentMetadata>;

} // namespace oxlm
