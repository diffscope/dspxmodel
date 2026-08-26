#ifndef DSPXMODEL_FREEVALUEDATAARRAY_P_H
#define DSPXMODEL_FREEVALUEDATAARRAY_P_H

#include <cstdint>

#include <dspxmodelORM/FreeValueDataArray.h>

#include <dini/value.h>

#include <dspxmodelORM/Handle.h>
#include <dspxmodelORM/private/ORMUtils_p.h>
#include <dspxmodelORM/private/JSIterable_p.h>

namespace dspx {

    class FreeValueDataArrayPrivate {
        Q_DECLARE_PUBLIC(FreeValueDataArray)
    public:
        FreeValueDataArrayPrivate(FreeValueDataArray *q, Parameter *parameter, FreeValueDataArray::FreeValueRole role);

        DSPXMODEL_DECLARE_GET(FreeValueDataArray)
        DSPXMODEL_FORWARD_CONSTRUCTOR(FreeValueDataArray)

        Handle relationHandle() const;
        dini::Value associationValue() const;
        void refresh(bool notify, bool itemsChanged);
        void applySplice(int index, int length, const QList<QVariant> &values, bool notify);
        void applyRotate(int leftIndex, int middleIndex, int rightIndex, bool notify);

        FreeValueDataArray *q_ptr = nullptr;
        Parameter *parameter = nullptr;
        FreeValueDataArray::FreeValueRole role = FreeValueDataArray::Original;
        mutable Handle cachedRelationHandle;
        mutable std::uint64_t cachedRelationEpoch = 0;
        QList<QVariant> items;
        int size = 0;
        bool suppressNotifications = false;

        JSIterable *jsIterable = nullptr;
    };

}

#endif // DSPXMODEL_FREEVALUEDATAARRAY_P_H
