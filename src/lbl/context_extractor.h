#pragma once

#include <vector>

#include "lbl/utils.h"

using namespace std;

namespace oxlm {

class ContextExtractor {
 public:
  ContextExtractor(
      const Corpus& corpus, int context_size,
      int start_id, int end_id);

  vector<WordId> extract(int position) const;

 private:
  const Corpus& corpus;
  int contextSize, startId, endId;
};

} // namespace oxlm
