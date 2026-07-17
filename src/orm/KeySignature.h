#ifndef DSPXMODEL_KEYSIGNATURE_H
#define DSPXMODEL_KEYSIGNATURE_H

#include <QScopedPointer>
#include <qqmlintegration.h>

#include <dspxmodelORM/EntityObject.h>
#include <nlohmann/json_fwd.hpp>

namespace dspx {

    class KeySignatureSequence;

    class KeySignaturePrivate;

    /**
     * @brief Key signature.
     */
    class DSPXMODEL_ORM_EXPORT KeySignature : public EntityObject {
        Q_OBJECT
        QML_ELEMENT
        QML_UNCREATABLE("")
        Q_DECLARE_PRIVATE(KeySignature)
        Q_PROPERTY(int position READ position WRITE setPosition NOTIFY positionChanged)
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

        /**
         * @brief Gets position.
         *
         * This property is the position index of the key signature in the key signature sequence.
         *
         * @post position() >= 0.
         */
        int position() const;
        /**
         * @brief Sets position.
         * @pre model()->document()->transaction() != nullptr && model()->document()->transaction()->state() == dini::TransactionState::Active.
         * @pre position >= 0.
         * @pre If keySignatureSequence() != nullptr, no other item in keySignatureSequence() has position.
         * @post position() == position.
         */
        void setPosition(int position);

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
         * @pre model()->document()->transaction() != nullptr && model()->document()->transaction()->state() == dini::TransactionState::Active.
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
         * @pre model()->document()->transaction() != nullptr && model()->document()->transaction()->state() == dini::TransactionState::Active.
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
         * @pre model()->document()->transaction() != nullptr && model()->document()->transaction()->state() == dini::TransactionState::Active.
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

        /**
         * @brief Converts to OpenDSPX key signature.
         */
        nlohmann::json toOpenDSPX() const;
        /**
         * @brief Converts from OpenDSPX key signature.
         * @note Typically, this method SHOULD only be called on a newly created object.
         * @pre model()->document()->transaction() != nullptr && model()->document()->transaction()->state() == dini::TransactionState::Active.
         * @pre keySignature must be valid.
         */
        void fromOpenDSPX(const nlohmann::json &keySignature);

    signals:
        void positionChanged(int position);
        void modeChanged(int mode);
        void tonalityChanged(int tonality);
        void accidentalTypeChanged(AccidentalType accidentalType);
        void previousItemChanged(KeySignature *previousItem);
        void nextItemChanged(KeySignature *nextItem);
        void keySignatureSequenceChanged(KeySignatureSequence *keySignatureSequence);
        void positionChangedAfterCommit(int position);
        void modeChangedAfterCommit(int mode);
        void tonalityChangedAfterCommit(int tonality);
        void accidentalTypeChangedAfterCommit(AccidentalType accidentalType);
        void previousItemChangedAfterCommit(KeySignature *previousItem);
        void nextItemChangedAfterCommit(KeySignature *nextItem);
        void keySignatureSequenceChangedAfterCommit(KeySignatureSequence *keySignatureSequence);

    private:
        ~KeySignature() override;

        explicit KeySignature(Handle handle, Model *model);

        QScopedPointer<KeySignaturePrivate> d_ptr;
    };

}

#endif // DSPXMODEL_KEYSIGNATURE_H
