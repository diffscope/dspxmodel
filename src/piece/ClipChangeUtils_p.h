#ifndef DSPXMODEL_CLIPCHANGEUTILS_P_H
#define DSPXMODEL_CLIPCHANGEUTILS_P_H

#include <QList>

#include "ClipChangeRange.h"

namespace dspx {

    QList<ClipChangeRange> normalizeClipChangeRanges(QList<ClipChangeRange> ranges);

}

#endif // DSPXMODEL_CLIPCHANGEUTILS_P_H
