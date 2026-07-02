#ifndef DSPXMODEL_LABEL_H
#define DSPXMODEL_LABEL_H

#include <QString>

#include <dspxmodelORM/EntityObject.h>

namespace dspx{

    class LabelSequence;

    class LabelPrivate;

    /**
     * @brief Label.
     */
    class DSPXMODEL_ORM_EXPORT Label : public EntityObject {
        Q_OBJECT
        Q_PROPERTY(int position READ position WRITE setPosition NOTIFY positionChanged)
        Q_PROPERTY(QString text READ text WRITE setText NOTIFY textChanged)
        Q_PROPERTY(Label *previousItem READ previousItem NOTIFY previousItemChanged)
        Q_PROPERTY(Label *nextItem READ nextItem NOTIFY nextItemChanged)
        Q_PROPERTY(LabelSequence *labelSequence READ labelSequence NOTIFY labelSequenceChanged)
    public:
        ~Label() override;

        /**
         * @brief Gets position.
         *
         * This property is the position index of the label in the label sequence.
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
         * @brief Gets text.
         */
        QString text() const;
        /**
         * @brief Sets text.
         * @post text() == text.
         */
        void setText(const QString &text);

        /**
         * @brief Gets previous item.
         */
        Label *previousItem() const;
        /**
         * @brief Gets next item.
         */
        Label *nextItem() const;

        /**
         * @brief Gets label sequence.
         */
        LabelSequence *labelSequence() const;

    signals:
        void positionChanged(int position);
        void textChanged(const QString &text);
        void previousItemChanged(Label *previousItem);
        void nextItemChanged(Label *nextItem);
        void labelSequenceChanged(LabelSequence *labelSequence);

    private:
        explicit Label(Handle handle, Model *model);

        QScopedPointer<LabelPrivate> d_ptr;
    };

}

#endif // DSPXMODEL_LABEL_H
