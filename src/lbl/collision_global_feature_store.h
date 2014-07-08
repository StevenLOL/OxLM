#pragma once

#include <boost/serialization/shared_ptr.hpp>

#include "lbl/archive_export.h"
#include "lbl/collision_minibatch_feature_store.h"
#include "lbl/collision_space.h"
#include "lbl/feature_context_generator.h"
#include "lbl/feature_context_keyer.h"
#include "lbl/global_feature_store.h"

namespace oxlm {

class CollisionGlobalFeatureStore : public GlobalFeatureStore {
 public:
  CollisionGlobalFeatureStore();

  CollisionGlobalFeatureStore(
      int vector_size, int hash_space_size, int feature_context_size,
      const boost::shared_ptr<CollisionSpace>& space,
      const boost::shared_ptr<FeatureContextKeyer>& keyer,
      const boost::shared_ptr<FeatureFilter>& filter);

  virtual VectorReal get(const vector<int>& context) const;

  virtual void l2GradientUpdate(
      const boost::shared_ptr<MinibatchFeatureStore>& store, Real sigma);

  virtual Real l2Objective(
      const boost::shared_ptr<MinibatchFeatureStore>& store, Real sigma) const;

  virtual void updateSquared(
      const boost::shared_ptr<MinibatchFeatureStore>& store);

  virtual void updateAdaGrad(
      const boost::shared_ptr<MinibatchFeatureStore>& gradient_store,
      const boost::shared_ptr<GlobalFeatureStore>& adagrad_store,
      Real step_size);

  virtual size_t size() const;

  static boost::shared_ptr<CollisionGlobalFeatureStore> cast(
      const boost::shared_ptr<GlobalFeatureStore>& base_store);

  bool operator==(const CollisionGlobalFeatureStore& other) const;

  virtual ~CollisionGlobalFeatureStore();

 private:
  void deepCopy(const CollisionGlobalFeatureStore& other);

  friend class boost::serialization::access;

  template<class Archive>
  void serialize(Archive& ar, const unsigned int version) {
    ar & boost::serialization::base_object<GlobalFeatureStore>(*this);

    ar & vectorSize;
    ar & hashSpaceSize;
    ar & generator;
    ar & keyer;
    ar & filter;
    ar & space;
  }

  int vectorSize, hashSpaceSize;
  FeatureContextGenerator generator;
  boost::shared_ptr<FeatureContextKeyer> keyer;
  boost::shared_ptr<FeatureFilter> filter;
  boost::shared_ptr<CollisionSpace> space;
  Real* featureWeights;
};

} // namespace oxlm

BOOST_CLASS_EXPORT_KEY(oxlm::CollisionGlobalFeatureStore)
