#pragma once

#include <unordered_map>
#include <unordered_set>

#include <boost/serialization/array.hpp>
#include <boost/serialization/serialization.hpp>
#include <boost/serialization/split_free.hpp>
#include <Eigen/Dense>
#include <Eigen/Sparse>

namespace boost {
namespace serialization {

template<class Archive, class Scalar, int Rows, int Cols, int Options, int MaxRows, int MaxCols>
inline void serialize(
    Archive& ar,
    Eigen::Matrix<Scalar, Rows, Cols, Options, MaxRows, MaxCols>& m,
    const unsigned int version) {
  ar & boost::serialization::make_array(m.data(), m.size());
}

// Serialization support for Eigen::SparseVector<Scalar>.

template<class Archive, class Scalar>
inline void save(
    Archive& ar, const Eigen::SparseVector<Scalar>& v,
    const unsigned int version) {
  int max_size = v.size();
  ar << max_size;
  int actual_size = v.nonZeros();
  ar << actual_size;
  for (typename Eigen::SparseVector<Scalar>::InnerIterator it(v); it; ++it) {
    int index = it.index();
    int value = it.value();
    ar << index << value;
  }
}

template<class Archive, class Scalar>
inline void load(
    Archive& ar, Eigen::SparseVector<Scalar>& v, const unsigned int version) {
  int max_size;
  ar >> max_size;
  v = Eigen::SparseVector<Scalar>(max_size);
  int actual_size;
  ar >> actual_size;
  for (int i = 0; i < actual_size; ++i) {
    int index, value;
    ar >> index >> value;
    v.coeffRef(index) = value;
  }
}

template<class Archive, class Scalar>
inline void serialize(
    Archive& ar, Eigen::SparseVector<Scalar>& v, const unsigned int version) {
  boost::serialization::split_free(ar, v, version);
}


// Serialization support for std::unordered_map<Key, Value>.

template<class Archive, class Key, class Value>
inline void save(
    Archive& ar, const std::unordered_map<Key, Value>& map,
    const unsigned int version) {
  size_t num_entries = map.size();
  ar << num_entries;
  for (const std::pair<Key, Value>& item: map) {
    ar << item.first << item.second;
  }
}

template<class Archive, class Key, class Value>
inline void load(
    Archive& ar, std::unordered_map<Key, Value>& map,
    const unsigned int version) {
  size_t num_entries;
  ar >> num_entries;
  for (size_t i = 0; i < num_entries; ++i) {
    Key key;
    Value value;
    ar >> key >> value;
    map.insert(make_pair(key, value));
  }
}

template<class Archive, class Key, class Value>
inline void serialize(
    Archive& ar, std::unordered_map<Key, Value>& map,
    const unsigned int version) {
  boost::serialization::split_free(ar, map, version);
}


// Serialization support for std::unordered_set<Value>.

template<class Archive, class Value>
inline void save(
    Archive& ar, const std::unordered_set<Value>& set,
    const unsigned int version) {
  size_t num_entries = set.size();
  ar << num_entries;
  for (const Value& value: set) {
    ar << value;
  }
}

template<class Archive, class Value>
inline void load(
    Archive& ar, std::unordered_set<Value>& set, const unsigned int version) {
  size_t num_entries;
  ar >> num_entries;
  for (size_t i = 0; i < num_entries; ++i) {
    Value value;
    ar >> value;
    set.insert(value);
  }
}

template<class Archive, class Value>
inline void serialize(
    Archive& ar, std::unordered_set<Value>& set, const unsigned int version) {
  boost::serialization::split_free(ar, set, version);
}

} // namespace serialization
} // namespace boost

