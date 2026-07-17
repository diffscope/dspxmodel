#ifndef DSPXMODEL_TEMPO_H
#define DSPXMODEL_TEMPO_H

#include <QScopedPointer>
#include <qqmlintegration.h>

#include <dspxmodelORM/EntityObject.h>

namespace opendspx {
    struct Tempo;
}

namespace dspx {

    class TempoSequence;

    class TempoPrivate;

    /**
     * @brief Tempo.
     */
    class DSPXMODEL_ORM_EXPORT Tempo : public EntityObject {
        Q_OBJECT
        QML_ELEMENT
        QML_UNCREATABLE("")
        Q_DECLARE_PRIVATE(Tempo)
        Q_PROPERTY(int position READ position WRITE setPosition NOTIFY positionChanged)
        Q_PROPERTY(double value READ value WRITE setValue NOTIFY valueChanged)
        Q_PROPERTY(Tempo *previousItem READ previousItem NOTIFY previousItemChanged)
        Q_PROPERTY(Tempo *nextItem READ nextItem NOTIFY nextItemChanged)
        Q_PROPERTY(TempoSequence *tempoSequence READ tempoSequence NOTIFY tempoSequenceChanged)
    public:
        /**
         * @brief Gets position.
         *
         * This property is the position index of the tempo in the tempo sequence.
         *
         * @post position() >= 0.
         */
        int position() const;
        /**
         * @brief Sets position.
         * @pre model()->document()->transaction() != nullptr && model()->document()->transaction()->state() == dini::TransactionState::Active.
         * @pre position >= 0.
         * @pre If tempoSequence() != nullptr, no other item in tempoSequence() has position.
         * @post position() == position.
         */
        void setPosition(int position);

        /**
         * @brief Gets value.
         *
         * This property is the BPM value of the tempo.
         *
         * @post value() >= 10 && value() <= 1000.
         */
        double value() const;
        /**
         * @brief Sets value.
         * @pre model()->document()->transaction() != nullptr && model()->document()->transaction()->state() == dini::TransactionState::Active.
         * @pre value >= 10 && value <= 1000.
         * @post value() == value.
         */
        void setValue(double value);

        /**
         * @brief Gets previous item.
         */
        Tempo *previousItem() const;
        /**
         * @brief Gets next item.
         */
        Tempo *nextItem() const;

        /**
         * @brief Gets tempo sequence.
         */
        TempoSequence *tempoSequence() const;

        /**
         * @brief Converts to OpenDSPX tempo.
         */
        opendspx::Tempo toOpenDSPX() const;
        /**
         * @brief Converts from OpenDSPX tempo.
         * @note Typically, this method SHOULD only be called on a newly created object.
         * @pre model()->document()->transaction() != nullptr && model()->document()->transaction()->state() == dini::TransactionState::Active.
         * @pre tempo must be valid.
         */
        void fromOpenDSPX(const opendspx::Tempo &tempo);

    signals:
        void positionChanged(int position);
        void valueChanged(double value);
        void previousItemChanged(Tempo *previousItem);
        void nextItemChanged(Tempo *nextItem);
        void tempoSequenceChanged(TempoSequence *tempoSequence);
        void positionChangedAfterCommit(int position);
        void valueChangedAfterCommit(double value);
        void previousItemChangedAfterCommit(Tempo *previousItem);
        void nextItemChangedAfterCommit(Tempo *nextItem);
        void tempoSequenceChangedAfterCommit(TempoSequence *tempoSequence);

    private:
        ~Tempo() override;

        explicit Tempo(Handle handle, Model *model);

        QScopedPointer<TempoPrivate> d_ptr;
    };

}

#endif // DSPXMODEL_TEMPO_H
