#ifndef DSPXMODEL_KEYSIGNATURE_H
#define DSPXMODEL_KEYSIGNATURE_H

#include <dspxmodelORM/EntityObject.h>

namespace dspx {

    class KeySignatureSequence;

    class KeySignaturePrivate;

    /**
     * @brief Key signature.
     */
    class DSPXMODEL_ORM_EXPORT KeySignature : public EntityObject {
        Q_OBJECT
        Q_PROPERTY(int pos READ pos WRITE setPos NOTIFY posChanged)
        Q_PROPERTY(int mode READ mode WRITE setMode NOTIFY modeChanged)
        Q_PROPERTY(int tonality READ tonality WRITE setTonality NOTIFY tonalityChanged)
        Q_PROPERTY(AccidentalType accidentalType READ accidentalType WRITE setAccidentalType NOTIFY accidentalTypeChanged)
        Q_PROPERTY(KeySignature *previousItem READ previousItem NOTIFY previousItemChanged)
        Q_PROPERTY(KeySignature *nextItem READ nextItem NOTIFY nextItemChanged)
        Q_PROPERTY(KeySignatureSequence *keySignatureSequence READ keySignatureSequence NOTIFY keySignatureSequenceChanged)
    public:
        /**
         * @brief Accidental type.
         */
        enum AccidentalType {
            Flat,
            Sharp,
        };
        Q_ENUM(AccidentalType)

        ~KeySignature() override;

        /**
         * @brief Gets pos.
         *
         * This property is the position index of the key signature in the key signature sequence.
         *
         * @post pos() >= 0.
         */
        int pos() const;
        /**
         * @brief Sets pos.
         * @pre pos >= 0.
         * @post pos() == pos.
         */
        void setPos(int pos);

        /**
         * @brief Gets mode.
         *
         * Bitwise flag: Starting from the least significant bit, it indicates whether a note offset from the tonic by
         * `n` semitones belongs to the scale.
         *
         * @post mode() >= 0 && mode() < 4096.
         */
        int mode() const;
        /**
         * @brief Sets mode.
         * @pre mode >= 0 && mode < 4096.
         * @post mode() == mode.
         */
        void setMode(int mode);

        /**
         * @brief Gets tonality.
         *
         * 0 for C, 1 for C#, 2 for D, ...
         *
         * @post tonality() >= 0 && tonality() < 12.
         */
        int tonality() const;
        /**
         * @brief Sets tonality.
         * @pre tonality >= 0 && tonality < 12.
         * @post tonality() == tonality.
         */
        void setTonality(int tonality);

        /**
         * @brief Gets accidental type.
         */
        AccidentalType accidentalType() const;
        /**
         * @brief Sets accidental type.
         * @post accidentalType() == accidentalType.
         */
        void setAccidentalType(AccidentalType accidentalType);

        /**
         * @brief Gets previous item.
         */
        KeySignature *previousItem() const;
        /**
         * @brief Gets next item.
         */
        KeySignature *nextItem() const;

        /**
         * @brief Gets key signature sequence.
         */
        KeySignatureSequence *keySignatureSequence() const;

    signals:
        void posChanged(int pos);
        void modeChanged(int mode);
        void tonalityChanged(int tonality);
        void accidentalTypeChanged(AccidentalType accidentalType);
        void previousItemChanged(KeySignature *previousItem);
        void nextItemChanged(KeySignature *nextItem);
        void keySignatureSequenceChanged(KeySignatureSequence *keySignatureSequence);

    private:
        explicit KeySignature(Handle handle, Model *model);

        QScopedPointer<KeySignaturePrivate> d_ptr;
    };

}

#endif // DSPXMODEL_KEYSIGNATURE_H
