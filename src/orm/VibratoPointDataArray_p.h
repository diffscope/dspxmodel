#ifndef DSPXMODEL_VIBRATOPOINTDATAARRAY_P_H
#define DSPXMODEL_VIBRATOPOINTDATAARRAY_P_H

#include <cstdint>

#include <dspxmodelORM/VibratoPointDataArray.h>

#include <dini/value.h>

#include <dspxmodelORM/Handle.h>
#include <dspxmodelORM/private/ORMUtils_p.h>
#include <dspxmodelORM/private/JSIterable_p.h>

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
        void applySplice(int index, int length, const QList<QPointF> &values, bool notify);
        void applyRotate(int leftIndex, int middleIndex, int rightIndex, bool notify);

        VibratoPointDataArray *q_ptr = nullptr;
        Note *note = nullptr;
        VibratoPointDataArray::VibratoPointRole role = VibratoPointDataArray::Amplitude;
        mutable Handle cachedRelationHandle;
        mutable std::uint64_t cachedRelationEpoch = 0;
        QList<QPointF> items;
        int size = 0;
        bool suppressNotifications = false;

        JSIterable *jsIterable = nullptr;
    };

}

#endif // DSPXMODEL_VIBRATOPOINTDATAARRAY_P_H
