#ifndef DSPXMODEL_KEYSIGNATUREPROPERTYMAPPER_H
#define DSPXMODEL_KEYSIGNATUREPROPERTYMAPPER_H

#include <QObject>
#include <QScopedPointer>
#include <QVariant>
#include <qqmlintegration.h>

#include <dspxmodelPropertyMapper/DSPXModelPropertyMapperGlobal.h>

namespace dspx {
    class SelectionModel;
    class KeySignaturePropertyMapperPrivate;

    class DSPXMODEL_PROPERTYMAPPER_EXPORT KeySignaturePropertyMapper : public QObject {
        Q_OBJECT
        QML_ELEMENT
        Q_DECLARE_PRIVATE(KeySignaturePropertyMapper)

        Q_PROPERTY(dspx::SelectionModel *selectionModel READ selectionModel WRITE setSelectionModel NOTIFY selectionModelChanged)
        Q_PROPERTY(QVariant position READ position WRITE setPosition NOTIFY positionChanged)
        Q_PROPERTY(QVariant mode READ mode WRITE setMode NOTIFY modeChanged)
        Q_PROPERTY(QVariant tonality READ tonality WRITE setTonality NOTIFY tonalityChanged)
        Q_PROPERTY(QVariant accidentalType READ accidentalType WRITE setAccidentalType NOTIFY accidentalTypeChanged)

    public:
        explicit KeySignaturePropertyMapper(QObject *parent = nullptr);
        ~KeySignaturePropertyMapper() override;

        dspx::SelectionModel *selectionModel() const;
        void setSelectionModel(dspx::SelectionModel *selectionModel);

        QVariant position() const;
        void setPosition(const QVariant &position);

        QVariant mode() const;
        void setMode(const QVariant &mode);

        QVariant tonality() const;
        void setTonality(const QVariant &tonality);

        QVariant accidentalType() const;
        void setAccidentalType(const QVariant &accidentalType);

    signals:
        void selectionModelChanged();
        void positionChanged();
        void modeChanged();
        void tonalityChanged();
        void accidentalTypeChanged();

    private:
        QScopedPointer<KeySignaturePropertyMapperPrivate> d_ptr;
    };
}

#endif // DSPXMODEL_KEYSIGNATUREPROPERTYMAPPER_H
