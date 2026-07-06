#ifndef DSPXMODEL_VIBRATOPOINTDATAARRAY_P_H
#define DSPXMODEL_VIBRATOPOINTDATAARRAY_P_H

#include <dspxmodelORM/VibratoPointDataArray.h>

#include <dini/value.h>

#include <dspxmodelORM/Handle.h>
#include <dspxmodelORM/private/ORMUtils_p.h>

namespace dspx {

    class VibratoPointDataArrayPrivate {
        Q_DECLARE_PUBLIC(VibratoPointDataArray)
    public:
        VibratoPointDataArrayPrivate(VibratoPointDataArray *q, Note *note, VibratoPointDataArray::VibratoPointRole role);

        DSPXMODEL_DECLARE_GET(VibratoPointDataArray)
        DSPXMODEL_FORWARD_CONSTRUCTOR(VibratoPointDataArray)

        Handle relationHandle() const;
        dini::Value associationValue() const;
        void refresh(bool notify, bool itemsChanged);

        VibratoPointDataArray *q_ptr = nullptr;
        Note *note = nullptr;
        VibratoPointDataArray::VibratoPointRole role = VibratoPointDataArray::Amplitude;
        int size = 0;
        bool suppressNotifications = false;
    };

}

#endif // DSPXMODEL_VIBRATOPOINTDATAARRAY_P_H
