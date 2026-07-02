#ifndef DSPXMODEL_TEMPO_H
#define DSPXMODEL_TEMPO_H

#include <dspxmodelORM/EntityObject.h>

namespace dspx {

    class TempoSequence;

    class TempoPrivate;

    /**
     * @brief Tempo.
     */
    class DSPXMODEL_ORM_EXPORT Tempo : public EntityObject {
        Q_OBJECT
        Q_PROPERTY(int position READ position WRITE setPosition NOTIFY positionChanged)
        Q_PROPERTY(double value READ value WRITE setValue NOTIFY valueChanged)
        Q_PROPERTY(Tempo *previousItem READ previousItem NOTIFY previousItemChanged)
        Q_PROPERTY(Tempo *nextItem READ nextItem NOTIFY nextItemChanged)
        Q_PROPERTY(TempoSequence *tempoSequence READ tempoSequence NOTIFY tempoSequenceChanged)
    public:
        ~Tempo() override;

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
         * @pre position >= 0.
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

    signals:
        void positionChanged(int position);
        void valueChanged(double value);
        void previousItemChanged(Tempo *previousItem);
        void nextItemChanged(Tempo *nextItem);
        void tempoSequenceChanged(TempoSequence *tempoSequence);

    private:
        explicit Tempo(Handle handle, Model *model);

        QScopedPointer<TempoPrivate> d_ptr;
    };

}

#endif // DSPXMODEL_TEMPO_H
