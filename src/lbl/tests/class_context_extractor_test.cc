#include "gtest/gtest.h"

#include <boost/make_shared.hpp>

#include "lbl/class_context_extractor.h"

namespace ar = boost::archive;

namespace oxlm {

class ClassContextExtractorTest : public testing::Test {
 protected:
  void SetUp() {
    vector<int> data = {2, 2, 2, 3, 1};
    vector<int> classes = {0, 2, 3, 4};
    boost::shared_ptr<Corpus> corpus = boost::make_shared<Corpus>(data);
    boost::shared_ptr<WordToClassIndex> index =
        boost::make_shared<WordToClassIndex>(classes);
    boost::shared_ptr<ContextProcessor> processor =
        boost::make_shared<ContextProcessor>(corpus, 2);
    boost::shared_ptr<FeatureContextHasher> hasher =
        boost::make_shared<FeatureContextHasher>(corpus, index, processor, 2);
    extractor = ClassContextExtractor(hasher);
  }

  ClassContextExtractor extractor;
};

TEST_F(ClassContextExtractorTest, TestBasic) {
  vector<int> context = {0};
  vector<int> expected_feature_ids = {0};
  EXPECT_EQ(expected_feature_ids, extractor.getFeatureContextIds(context));
  context = {0, 0};
  expected_feature_ids = {0, 1};
  EXPECT_EQ(expected_feature_ids, extractor.getFeatureContextIds(context));
  context = {2};
  expected_feature_ids = {2};
  EXPECT_EQ(expected_feature_ids, extractor.getFeatureContextIds(context));
  context = {2, 0};
  expected_feature_ids = {2, 3};
  EXPECT_EQ(expected_feature_ids, extractor.getFeatureContextIds(context));
  context = {2, 2};
  expected_feature_ids = {2, 4};
  EXPECT_EQ(expected_feature_ids, extractor.getFeatureContextIds(context));
  context = {3};
  expected_feature_ids = {5};
  EXPECT_EQ(expected_feature_ids, extractor.getFeatureContextIds(context));
  context = {3, 2};
  expected_feature_ids = {5, 6};
  EXPECT_EQ(expected_feature_ids, extractor.getFeatureContextIds(context));
}

TEST_F(ClassContextExtractorTest, TestSerialization) {
  boost::shared_ptr<FeatureContextExtractor> extractor_ptr =
      boost::make_shared<ClassContextExtractor>(extractor);
  boost::shared_ptr<FeatureContextExtractor> extractor_copy_ptr;

  stringstream stream(ios_base::binary | ios_base::out | ios_base::in);
  ar::binary_oarchive output_stream(stream, ar::no_header);
  output_stream << extractor_ptr;

  ar::binary_iarchive input_stream(stream, ar::no_header);
  input_stream >> extractor_copy_ptr;

  boost::shared_ptr<ClassContextExtractor> expected_ptr =
      dynamic_pointer_cast<ClassContextExtractor>(extractor_ptr);
  boost::shared_ptr<ClassContextExtractor> actual_ptr =
      dynamic_pointer_cast<ClassContextExtractor>(extractor_copy_ptr);

  EXPECT_NE(nullptr, expected_ptr);
  EXPECT_NE(nullptr, actual_ptr);
  EXPECT_EQ(*expected_ptr, *actual_ptr);
}

} // namespace oxlm
