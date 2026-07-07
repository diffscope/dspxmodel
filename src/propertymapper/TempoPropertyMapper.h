#ifndef DSPXMODEL_TEMPOPROPERTYMAPPER_H
#define DSPXMODEL_TEMPOPROPERTYMAPPER_H

#include <QObject>
#include <QScopedPointer>
#include <QVariant>
#include <qqmlintegration.h>

#include <dspxmodelPropertyMapper/DSPXModelPropertyMapperGlobal.h>

namespace dspx {
    class SelectionModel;
    class TempoPropertyMapperPrivate;

    class DSPXMODEL_PROPERTYMAPPER_EXPORT TempoPropertyMapper : public QObject {
        Q_OBJECT
        QML_ELEMENT
        Q_DECLARE_PRIVATE(TempoPropertyMapper)

        Q_PROPERTY(dspx::SelectionModel *selectionModel READ selectionModel WRITE setSelectionModel NOTIFY selectionModelChanged)
        Q_PROPERTY(QVariant position READ position WRITE setPosition NOTIFY positionChanged)
        Q_PROPERTY(QVariant value READ value WRITE setValue NOTIFY valueChanged)

    public:
        explicit TempoPropertyMapper(QObject *parent = nullptr);
        ~TempoPropertyMapper() override;

        dspx::SelectionModel *selectionModel() const;
        void setSelectionModel(dspx::SelectionModel *selectionModel);

        QVariant position() const;
        void setPosition(const QVariant &position);

        QVariant value() const;
        void setValue(const QVariant &value);

    signals:
        void selectionModelChanged();
        void positionChanged();
        void valueChanged();

    private:
        QScopedPointer<TempoPropertyMapperPrivate> d_ptr;
    };
}

#endif // DSPXMODEL_TEMPOPROPERTYMAPPER_H
