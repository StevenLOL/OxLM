#pragma once

#include <vector>

#include <boost/serialization/serialization.hpp>

#include "lbl/context_extractor.h"
#include "lbl/feature_context.h"
#include "lbl/utils.h"
#include "utils/serialization_helpers.h"

using namespace std;

namespace oxlm {

/**
 * Given a context of words [w_{n-1}, w_{n-2}, ...] generates all the feature
 * ids that match the context.
 *
 * Preprocesses the training corpus to populate the hash table with all the
 * possible feature ids to guarantee thread-safe operations later on.
 **/
class FeatureGenerator {
 public:
  FeatureGenerator(
      const Corpus& corpus,
      const ContextExtractor& extractor,
      size_t feature_context_size);

  vector<FeatureContextId> getFeatureContextIds(
      const vector<WordId>& history) const;

private:
  vector<FeatureContext> getFeatureContexts(
      const vector<WordId>& history) const;

  friend class boost::serialization::access;

  template<class Archive>
  void serialize(Archive& ar, const unsigned int version) {
    ar & feature_context_size & featureContextsMap;
  }

  size_t feature_context_size;
  unordered_map<FeatureContext, FeatureContextId, hash<FeatureContext>>
      featureContextsMap;
};

} // namespace oxlm
