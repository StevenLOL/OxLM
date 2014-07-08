#include "gtest/gtest.h"

#include <boost/make_shared.hpp>

#include "lbl/factored_weights.h"
#include "utils/constants.h"

namespace oxlm {

class FactoredWeightsTest : public testing::Test {
 protected:
  void SetUp() {
    config.word_representation_size = 3;
    config.vocab_size = 5;
    config.ngram_order = 3;

    vector<int> data = {2, 3, 4, 1};
    vector<int> classes = {0, 2, 4, 5};
    corpus = boost::make_shared<Corpus>(data);
    index = boost::make_shared<WordToClassIndex>(classes);
    metadata = boost::make_shared<FactoredMetadata>(config, dict, index);
  }

  ModelData config;
  Dict dict;
  boost::shared_ptr<WordToClassIndex> index;
  boost::shared_ptr<FactoredMetadata> metadata;
  boost::shared_ptr<Corpus> corpus;
};

TEST_F(FactoredWeightsTest, TestCheckGradient) {
  FactoredWeights weights(config, metadata, corpus);
  vector<int> indices = {0, 1, 2, 3};
  Real objective;
  boost::shared_ptr<FactoredWeights> gradient =
      weights.getGradient(corpus, indices, objective);

  EXPECT_NEAR(6.25879136, objective, EPS);
  EXPECT_TRUE(weights.checkGradient(corpus, indices, gradient));
}

TEST_F(FactoredWeightsTest, TestCheckGradientDiagonal) {
  config.diagonal_contexts = true;
  FactoredWeights weights(config, metadata, corpus);
  vector<int> indices = {0, 1, 2, 3};
  Real objective;
  boost::shared_ptr<FactoredWeights> gradient =
      weights.getGradient(corpus, indices, objective);

  EXPECT_NEAR(6.25521610, objective, EPS);
  EXPECT_TRUE(weights.checkGradient(corpus, indices, gradient));
}

} // namespace oxlm
