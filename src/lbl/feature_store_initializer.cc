#include "lbl/feature_store_initializer.h"

#include <boost/make_shared.hpp>

#include "lbl/sparse_feature_store.h"
#include "lbl/unconstrained_feature_store.h"

namespace oxlm {

FeatureStoreInitializer::FeatureStoreInitializer(
    const ModelData& config,
    const WordToClassIndex& index,
    const FeatureMatcher& matcher)
    : config(config), index(index), matcher(matcher) {}

void FeatureStoreInitializer::initialize(
    boost::shared_ptr<FeatureStore>& U,
    vector<boost::shared_ptr<FeatureStore>>& V,
    bool random_weights) const {
  if (config.sparse_features) {
    initializeSparseStores(U, V, matcher.getFeatures(), random_weights);
  } else {
    initializeUnconstrainedStores(U, V);
  }
}

void FeatureStoreInitializer::initialize(
    boost::shared_ptr<FeatureStore>& U,
    vector<boost::shared_ptr<FeatureStore>>& V,
    const vector<int>& minibatch_indices, bool random_weights) const {
  if (config.sparse_features) {
    initializeSparseStores(
        U, V, matcher.getFeatures(minibatch_indices), random_weights);
  } else {
    initializeUnconstrainedStores(U, V);
  }
}

void FeatureStoreInitializer::initializeUnconstrainedStores(
    boost::shared_ptr<FeatureStore>& U,
    vector<boost::shared_ptr<FeatureStore>>& V) const {
  U = boost::make_shared<UnconstrainedFeatureStore>(config.classes);
  V.resize(config.classes);
  for (int i = 0; i < config.classes; ++i) {
    V[i] = boost::make_shared<UnconstrainedFeatureStore>(index.getClassSize(i));
  }
}

void FeatureStoreInitializer::initializeSparseStores(
    boost::shared_ptr<FeatureStore>& U,
    vector<boost::shared_ptr<FeatureStore>>& V,
    FeatureIndexesPairPtr feature_indexes_pair,
    bool random_weights) const {
  U = boost::make_shared<SparseFeatureStore>(
      config.classes, feature_indexes_pair->getClassIndexes(), random_weights);
  V.resize(config.classes);
  for (int i = 0; i < config.classes; ++i) {
    V[i] = boost::make_shared<SparseFeatureStore>(
        index.getClassSize(i),
        feature_indexes_pair->getWordIndexes(i),
        random_weights);
  }
}

} // namespace oxlm
