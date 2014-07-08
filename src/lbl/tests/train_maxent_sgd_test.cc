#include "gtest/gtest.h"

#include "lbl/tests/test_sgd.h"
#include "lbl/train_maxent_sgd.h"

namespace oxlm {

TEST_F(TestSGD, TestTrainMaxentSGD) {
  config.l2_maxent = 2;
  config.feature_context_size = 3;

  config.test_file = "test.txt";
  boost::shared_ptr<FactoredNLM> model = learn(config);
  boost::shared_ptr<Corpus> test_corpus =
      readCorpus(config.training_file, model->label_set());
  double log_pp = perplexity(model, test_corpus);
  EXPECT_NEAR(101.731446, exp(-log_pp / test_corpus->size()), 1e-3);
}

TEST_F(TestSGD, TestTrainMaxentSGDSparseFeatures) {
  config.l2_maxent = 0.1;
  config.feature_context_size = 3;
  config.sparse_features = true;

  config.test_file = "test.txt";
  boost::shared_ptr<FactoredNLM> model = learn(config);
  boost::shared_ptr<Corpus> test_corpus =
      readCorpus(config.training_file, model->label_set());
  double log_pp = perplexity(model, test_corpus);
  EXPECT_NEAR(138.587834, exp(-log_pp / test_corpus->size()), 1e-3);
}

} // namespace oxlm
