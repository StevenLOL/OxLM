#include <boost/shared_ptr.hpp>
#include <boost/lexical_cast.hpp>
#include <boost/random.hpp>
#include <boost/archive/text_iarchive.hpp>


#include <math.h>
#include <iostream>
#include <functional>
#include <fstream>
#include <vector>
#include <random>
#include <cstring>
#include <omp.h>

#include "cg/jcg/additive-cnlm.h"
#include "cg/jcg/gcnlm.h"
#include "cg/utils.h"

using namespace std;
using namespace boost;
using namespace oxlm;
using namespace std::placeholders;

static boost::mt19937 linear_model_rng(static_cast<unsigned> (std::time(0)));
static uniform_01<> linear_model_uniform_dist;


ConditionalNLM::ConditionalNLM() : GeneralConditionalNLM(), S(0,0,0), g_S(0,0,0) {}

ConditionalNLM::ConditionalNLM(const ModelData& config,
                               const Dict& source_labels,
                               const Dict& target_labels,
                               const std::vector<int>& classes)
  : GeneralConditionalNLM(config, target_labels, classes), S(0,0,0), g_S(0,0,0),
  m_source_labels(source_labels) {
    init(true);
    initWordToClass();
  }

void ConditionalNLM::init(bool init_weights) {
  calculateDataSize(true);  // Calculates space requirements for this class and
                            //the parent and allocates space accordingly.

  new (&W) WeightsType(m_data, m_data_size);
  if (init_weights) {
    std::random_device rd;
    std::mt19937 gen(rd());
    std::normal_distribution<Real> gaussian(0,0.1);
    for (int i=0; i<m_data_size; i++)
      W(i) = gaussian(gen);
  }
  else W.setZero();

  Real* ptr = W.data();
  // cerr << "Ptr: " << ptr << endl;
  map_parameters(ptr, S, T);
  // cerr << "Ptr: " << ptr << endl;
  GeneralConditionalNLM::map_parameters(ptr, R, Q, F, C, B, FB);
  // cerr << "Ptr: " << ptr << endl;
}

int ConditionalNLM::calculateDataSize(bool allocate) {
  int parent_size = GeneralConditionalNLM::calculateDataSize(false);

  int num_source_words = source_types();
  int word_width = config.word_representation_size;
  int window_width = max(config.source_window_width, 0);

  int S_size = num_source_words * word_width;;
  int T_size = (2*window_width + 1) * (config.diagonal ? word_width : word_width*word_width);

  int data_size = parent_size + S_size + T_size;
  if (allocate) {
    m_data_size = data_size;
    m_data = new Real[m_data_size];
  }
  return data_size;
}

void ConditionalNLM::source_representation(const Sentence& source, int target_index, VectorReal& result) const {
  result = VectorReal::Zero(config.word_representation_size);
  int window = config.source_window_width;

  if (target_index < 0 || window < 0) {
    for (auto s_i : source)
      result += S.row(s_i);
  }
  else {
    int source_len = source.size();
    int centre = min(floor(Real(target_index)*length_ratio + 0.5), double(source_len-1));
    int start = max(centre-window, 0);
    int end = min(source_len, centre+window+1);

    for (int i=start; i < end; ++i)
      result += window_product(i-centre+window, S.row(source.at(i))).transpose();
  }
}

Real ConditionalNLM::log_prob(const WordId w, const std::vector<WordId>& context, const Sentence& source,
                              bool cache, int target_index) const {
  VectorReal s;
  source_representation(source, target_index, s);
  return GeneralConditionalNLM::log_prob(w, context, s, cache);
}

Real ConditionalNLM::gradient(std::vector<Sentence>& source_corpus_,
                              const std::vector<Sentence>& target_corpus,
                              const TrainingInstances &training_instances,
                              Real l2, Real source_l2, WeightsType& g_W) {

  source_corpus = source_corpus_;

  Real* ptr = g_W.data();
  map_parameters(ptr, g_S, g_T);  // Allocates data for child.

  Real f = 0.0;
  f = gradient_(target_corpus, training_instances, l2, source_l2, ptr);  // Allocates data for parent.

  #pragma omp master
  {
    if (source_l2 > 0.0) {
      // l2 objective contributions
      f += (0.5*source_l2*S.squaredNorm());
      for (size_t t=0; t<T.size(); ++t)
        f += (0.5*source_l2*T.at(t).squaredNorm());

      // l2 gradient contributions
      g_S.array() += (source_l2*S.array());
      for (size_t t=0; t<T.size(); ++t)
        g_T.at(t).array() += (source_l2*T.at(t).array());
    }
  }
  return f;
}

void ConditionalNLM::source_repr_callback(TrainingInstance t, int t_i, VectorReal& r) {
  source_representation(source_corpus.at(t), t_i, r);
}

void ConditionalNLM::source_grad_callback(TrainingInstance t, int t_i, int instance_counter, const VectorReal& grads) {
  // Source word representations gradient
  const Sentence& source_sent = source_corpus.at(t);
  int source_len = source_sent.size();
  int window = config.source_window_width;
  if (window < 0) {
    for (auto s_i : source_sent)
      g_S.row(s_i) += grads;
  }
  else {
    int centre = min(floor(Real(t_i)*length_ratio + 0.5), double(source_len-1));
    int start = max(centre-window, 0);
    int end = min(source_len, centre+window+1);
    for (int i=start; i < end; ++i) {
      g_S.row(source_sent.at(i)) += window_product(i-centre+window, grads, true);
      context_gradient_update(g_T.at(i-centre+window), S.row(source_sent.at(i)), grads);
    }
  }
}

void ConditionalNLM::map_parameters(Real*& ptr, WordVectorsType& s,
                                    ContextTransformsType& t) const {
  int num_source_words = source_types();
  int word_width = config.word_representation_size;
  int window_width = max(config.source_window_width,0);

  int S_size = num_source_words * word_width;
  // TODO(kmh): T_size probably wrong - take window width into account.
  int T_size = (config.diagonal ? word_width : word_width*word_width);

  // Real* ptr = wa.data();

  new (&s) WordVectorsType(ptr, num_source_words, word_width);
  ptr += S_size;

  t.clear();
  for (int i=0; i<(2*window_width+1); i++) {
    if (config.diagonal) t.push_back(ContextTransformType(ptr, word_width, 1));
    else                 t.push_back(ContextTransformType(ptr, word_width, word_width));
    ptr += T_size;
  }
}