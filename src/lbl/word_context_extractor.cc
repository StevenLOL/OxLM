#include "lbl/word_context_extractor.h"

namespace oxlm {

WordContextExtractor::WordContextExtractor() {}

WordContextExtractor::WordContextExtractor(
    int class_id, const boost::shared_ptr<FeatureContextHasher>& hasher)
    : classId(class_id), hasher(hasher) {}

vector<int> WordContextExtractor::getFeatureContextIds(
    const vector<int>& context) const {
  return hasher->getWordContextIds(classId, context);
}

} // namespace oxlm

BOOST_CLASS_EXPORT_IMPLEMENT(oxlm::WordContextExtractor)
