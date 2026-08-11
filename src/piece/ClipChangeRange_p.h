#ifndef DSPXMODEL_CLIPCHANGERANGE_P_H
#define DSPXMODEL_CLIPCHANGERANGE_P_H

#include <QSharedData>

#include "ClipChangeRange.h"

namespace dspx {

    class ClipChangeRangePrivate : public QSharedData {
    public:
        int position = 0;
        int length = 0;
    };

}

#endif // DSPXMODEL_CLIPCHANGERANGE_P_H
