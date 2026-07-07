#ifndef DSPXMODEL_LABELPROPERTYMAPPER_H
#define DSPXMODEL_LABELPROPERTYMAPPER_H

#include <QObject>
#include <QScopedPointer>
#include <QVariant>
#include <qqmlintegration.h>

#include <dspxmodelPropertyMapper/DSPXModelPropertyMapperGlobal.h>

namespace dspx {
    class SelectionModel;
    class LabelPropertyMapperPrivate;

    class DSPXMODEL_PROPERTYMAPPER_EXPORT LabelPropertyMapper : public QObject {
        Q_OBJECT
        QML_ELEMENT
        Q_DECLARE_PRIVATE(LabelPropertyMapper)

        Q_PROPERTY(dspx::SelectionModel *selectionModel READ selectionModel WRITE setSelectionModel NOTIFY selectionModelChanged)
        Q_PROPERTY(QVariant position READ position WRITE setPosition NOTIFY positionChanged)
        Q_PROPERTY(QVariant text READ text WRITE setText NOTIFY textChanged)

    public:
        explicit LabelPropertyMapper(QObject *parent = nullptr);
        ~LabelPropertyMapper() override;

        dspx::SelectionModel *selectionModel() const;
        void setSelectionModel(dspx::SelectionModel *selectionModel);

        QVariant position() const;
        void setPosition(const QVariant &position);

        QVariant text() const;
        void setText(const QVariant &text);

    signals:
        void selectionModelChanged();
        void positionChanged();
        void textChanged();

    private:
        QScopedPointer<LabelPropertyMapperPrivate> d_ptr;
    };
}

#endif // DSPXMODEL_LABELPROPERTYMAPPER_H
