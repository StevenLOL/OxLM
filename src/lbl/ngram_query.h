#pragma once

#include <vector>

#include <boost/serialization/serialization.hpp>
#include <boost/serialization/vector.hpp>

#include "lbl/utils.h"

using namespace std;

namespace oxlm {

struct NGramQuery {
  NGramQuery();

  NGramQuery(int word, const vector<int>& context);

  bool operator==(const NGramQuery& other) const;

  int word;
  vector<int> context;

 private:
  friend class boost::serialization::access;

  template<class Archive>
  void serialize(Archive& ar, const unsigned int version) {
    ar & word & context;
  }
};

} // namespace oxlm


namespace std {

template<> struct hash<oxlm::NGramQuery> {
  inline size_t operator()(const oxlm::NGramQuery query) const {
    vector<int> data(1, query.word);
    data.insert(data.end(), query.context.begin(), query.context.end());
    return oxlm::MurmurHash(data);
  }
};

} // namespace std
