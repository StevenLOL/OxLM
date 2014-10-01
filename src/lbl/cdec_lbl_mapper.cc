#include "lbl/cdec_lbl_mapper.h"

#include "hg.h"

namespace oxlm {

CdecLBLMapper::CdecLBLMapper(
    const boost::shared_ptr<Vocabulary>& vocab) : vocab(vocab) {
  kUNKNOWN = this->vocab->convert("<unk>");
  for (int i = 0; i < vocab->size(); ++i) {
    add(i, TD::Convert(vocab->convert(i)));
  }
}

void CdecLBLMapper::add(int lbl_id, int cdec_id) {
  if (cdec_id >= cdec2lbl.size()) {
    cdec2lbl.resize(cdec_id + 1, kUNKNOWN);
  }
  cdec2lbl[cdec_id] = lbl_id;
}

int CdecLBLMapper::convert(int cdec_id) const {
  if (cdec_id < 0 || cdec_id >= cdec2lbl.size()) {
    return kUNKNOWN;
  } else {
    return cdec2lbl[cdec_id];
  }
}

} // namespace oxlm
