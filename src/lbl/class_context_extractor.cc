#include "lbl/class_context_extractor.h"

namespace oxlm {

ClassContextExtractor::ClassContextExtractor() {}

ClassContextExtractor::ClassContextExtractor(
    const boost::shared_ptr<FeatureContextHasher>& hasher)
    : hasher(hasher) {}

vector<int> ClassContextExtractor::getFeatureContextIds(
    const vector<int>& context) const {
  return hasher->getClassContextIds(context);
}

bool ClassContextExtractor::operator==(
    const ClassContextExtractor& other) const {
  return *hasher == *other.hasher;
}

} // namespace oxlm

BOOST_CLASS_EXPORT_IMPLEMENT(oxlm::ClassContextExtractor)
