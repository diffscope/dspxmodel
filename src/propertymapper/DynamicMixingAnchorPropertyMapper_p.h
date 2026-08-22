// SPDX-FileCopyrightText: Team OpenVPI
// SPDX-License-Identifier: Apache-2.0

#ifndef DSPXMODEL_DYNAMICMIXINGANCHORPROPERTYMAPPER_P_H
#define DSPXMODEL_DYNAMICMIXINGANCHORPROPERTYMAPPER_P_H

#include <QList>
#include <QMetaObject>

#include <dspxmodelPropertyMapper/DynamicMixingAnchorPropertyMapper.h>

namespace dspx {

    class DynamicMixingAnchor;
    class DynamicMixingAnchorSequence;

    class DynamicMixingAnchorPropertyMapperPrivate {
        Q_DECLARE_PUBLIC(DynamicMixingAnchorPropertyMapper)

    public:
        DynamicMixingAnchorPropertyMapper *q_ptr = nullptr;
        SelectionModel *selectionModel = nullptr;
        DynamicMixingAnchorSequence *sequence = nullptr;
        QList<QMetaObject::Connection> connections;

        void disconnectAll();
        void refreshConnections();
        QList<DynamicMixingAnchor *> selectedItems() const;
        QList<double> effectiveRatios(const DynamicMixingAnchor *item) const;
        void notifyAll();
    };

}

#endif // DSPXMODEL_DYNAMICMIXINGANCHORPROPERTYMAPPER_P_H
