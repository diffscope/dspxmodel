// SPDX-FileCopyrightText: Team OpenVPI
// SPDX-License-Identifier: Apache-2.0

#ifndef DSPXMODEL_DYNAMICMIXINGANCHORPROPERTYMAPPER_H
#define DSPXMODEL_DYNAMICMIXINGANCHORPROPERTYMAPPER_H

#include <QObject>
#include <QScopedPointer>
#include <QVariant>
#include <qqmlintegration.h>

#include <dspxmodelPropertyMapper/DSPXModelPropertyMapperGlobal.h>

namespace dspx {

    class DynamicMixingAnchorPropertyMapperPrivate;
    class SelectionModel;

    class DSPXMODEL_PROPERTYMAPPER_EXPORT DynamicMixingAnchorPropertyMapper : public QObject {
        Q_OBJECT
        QML_ELEMENT
        Q_DECLARE_PRIVATE(DynamicMixingAnchorPropertyMapper)
        Q_PROPERTY(dspx::SelectionModel *selectionModel READ selectionModel WRITE setSelectionModel NOTIFY selectionModelChanged)
        Q_PROPERTY(QVariant position READ position NOTIFY positionChanged)
        Q_PROPERTY(int voiceCount READ voiceCount NOTIFY singersChanged)
        Q_PROPERTY(QObjectList singers READ singers NOTIFY singersChanged)
        Q_PROPERTY(QVariant ratios READ ratios NOTIFY ratiosChanged)

    public:
        explicit DynamicMixingAnchorPropertyMapper(QObject *parent = nullptr);
        ~DynamicMixingAnchorPropertyMapper() override;

        SelectionModel *selectionModel() const;
        void setSelectionModel(SelectionModel *selectionModel);

        QVariant position() const;
        int voiceCount() const;
        QObjectList singers() const;
        QVariant ratios() const;

        Q_INVOKABLE QVariant ratioAt(int singerIndex) const;
        Q_INVOKABLE QVariant maximumRatioAt(int singerIndex) const;
        Q_INVOKABLE bool setSingerRatio(int singerIndex, double ratio);
        Q_INVOKABLE bool setAdjacentRatios(int leftSingerIndex, double leftRatio);

    Q_SIGNALS:
        void selectionModelChanged();
        void positionChanged();
        void singersChanged();
        void ratiosChanged();

    private:
        QScopedPointer<DynamicMixingAnchorPropertyMapperPrivate> d_ptr;
    };

}

#endif // DSPXMODEL_DYNAMICMIXINGANCHORPROPERTYMAPPER_H
