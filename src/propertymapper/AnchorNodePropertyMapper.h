#ifndef DSPXMODEL_ANCHORNODEPROPERTYMAPPER_H
#define DSPXMODEL_ANCHORNODEPROPERTYMAPPER_H

#include <QObject>
#include <QScopedPointer>
#include <QVariant>
#include <qqmlintegration.h>

#include <dspxmodelPropertyMapper/DSPXModelPropertyMapperGlobal.h>

namespace dspx {
    class AnchorNodePropertyMapperPrivate;
    class SelectionModel;

    class DSPXMODEL_PROPERTYMAPPER_EXPORT AnchorNodePropertyMapper : public QObject {
        Q_OBJECT
        QML_ELEMENT
        Q_DECLARE_PRIVATE(AnchorNodePropertyMapper)

        Q_PROPERTY(dspx::SelectionModel *selectionModel READ selectionModel WRITE setSelectionModel NOTIFY selectionModelChanged)
        Q_PROPERTY(QVariant position READ position NOTIFY positionChanged)
        Q_PROPERTY(QVariant value READ value WRITE setValue NOTIFY valueChanged)
        Q_PROPERTY(QVariant interpolationMode READ interpolationMode WRITE setInterpolationMode NOTIFY interpolationModeChanged)

    public:
        explicit AnchorNodePropertyMapper(QObject *parent = nullptr);
        ~AnchorNodePropertyMapper() override;

        SelectionModel *selectionModel() const;
        void setSelectionModel(SelectionModel *selectionModel);

        QVariant position() const;
        QVariant value() const;
        void setValue(const QVariant &value);
        QVariant interpolationMode() const;
        void setInterpolationMode(const QVariant &interpolationMode);

    Q_SIGNALS:
        void selectionModelChanged();
        void positionChanged();
        void valueChanged();
        void interpolationModeChanged();

    private:
        QScopedPointer<AnchorNodePropertyMapperPrivate> d_ptr;
    };
}

#endif // DSPXMODEL_ANCHORNODEPROPERTYMAPPER_H
