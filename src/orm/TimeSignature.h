#ifndef DSPXMODEL_TIMESIGNATURE_H
#define DSPXMODEL_TIMESIGNATURE_H

#include <QScopedPointer>

#include <dspxmodelORM/EntityObject.h>

namespace opendspx {
    struct TimeSignature;
}

namespace dspx {

    class TimeSignatureSequence;

    class TimeSignaturePrivate;

    /**
     * @brief Time signature.
     */
    class DSPXMODEL_ORM_EXPORT TimeSignature : public EntityObject {
        Q_OBJECT
        Q_DECLARE_PRIVATE(TimeSignature)
        Q_PROPERTY(int index READ index WRITE setIndex NOTIFY indexChanged)
        Q_PROPERTY(int numerator READ numerator WRITE setNumerator NOTIFY numeratorChanged)
        Q_PROPERTY(int denominator READ denominator WRITE setDenominator NOTIFY denominatorChanged)
        Q_PROPERTY(TimeSignature *previousItem READ previousItem NOTIFY previousItemChanged)
        Q_PROPERTY(TimeSignature *nextItem READ nextItem NOTIFY nextItemChanged)
        Q_PROPERTY(TimeSignatureSequence *timeSignatureSequence READ timeSignatureSequence NOTIFY timeSignatureSequenceChanged)
    public:
        /**
         * @brief Gets index.
         *
         * This property is the measure index of the time signature in the time signature sequence.
         *
         * @post index() >= 0.
         */
        int index() const;
        /**
         * @brief Sets index.
         *
         * If this time signature is contained in a time signature sequence and another item in the same sequence
         * already has the target index, that item is removed from the sequence before this index is updated.
         *
         * @pre model()->document()->transaction() != nullptr && model()->document()->transaction()->state() == dini::TransactionState::Active.
         * @pre index >= 0.
         * @post index() == index.
         */
        void setIndex(int index);

        /**
         * @brief Gets numerator.
         *
         * This property is the numerator of the time signature.
         *
         * @post numerator() > 0.
         */
        int numerator() const;
        /**
         * @brief Sets numerator.
         * @pre model()->document()->transaction() != nullptr && model()->document()->transaction()->state() == dini::TransactionState::Active.
         * @pre numerator > 0.
         * @post numerator() == numerator.
         */
        void setNumerator(int numerator);

        /**
         * @brief Gets denominator.
         *
         * This property is the denominator of the time signature.
         *
         * @post denominator() == 1 || denominator() == 2 || denominator() == 4 || denominator() == 8 || denominator() == 16 || denominator() == 32 || denominator() == 64 || denominator() == 128.
         */
        int denominator() const;
        /**
         * @brief Sets denominator.
         * @pre model()->document()->transaction() != nullptr && model()->document()->transaction()->state() == dini::TransactionState::Active.
         * @pre denominator == 1 || denominator == 2 || denominator == 4 || denominator == 8 || denominator == 16 || denominator == 32 || denominator == 64 || denominator == 128.
         * @post denominator() == denominator.
         */
        void setDenominator(int denominator);

        /**
         * @brief Gets previous item.
         */
        TimeSignature *previousItem() const;
        /**
         * @brief Gets next item.
         */
        TimeSignature *nextItem() const;

        /**
         * @brief Gets time signature sequence.
         */
        TimeSignatureSequence *timeSignatureSequence() const;

        /**
         * @brief Converts to OpenDSPX time signature.
         */
        opendspx::TimeSignature toOpenDSPX() const;
        /**
         * @brief Converts from OpenDSPX time signature.
         * @pre model()->document()->transaction() != nullptr && model()->document()->transaction()->state() == dini::TransactionState::Active.
         * @pre timeSignature must be valid.
         */
        void fromOpenDSPX(const opendspx::TimeSignature &timeSignature);

    signals:
        void indexChanged(int index);
        void numeratorChanged(int numerator);
        void denominatorChanged(int denominator);
        void previousItemChanged(TimeSignature *previousItem);
        void nextItemChanged(TimeSignature *nextItem);
        void timeSignatureSequenceChanged(TimeSignatureSequence *timeSignatureSequence);

    private:
        ~TimeSignature() override;

        explicit TimeSignature(Handle handle, Model *model);

        QScopedPointer<TimeSignaturePrivate> d_ptr;
    };

}

#endif // DSPXMODEL_TIMESIGNATURE_H
