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
        Q_PROPERTY(int pos READ pos WRITE setPos NOTIFY posChanged)
        Q_PROPERTY(QString text READ text WRITE setText NOTIFY textChanged)
        Q_PROPERTY(Label *previousItem READ previousItem NOTIFY previousItemChanged)
        Q_PROPERTY(Label *nextItem READ nextItem NOTIFY nextItemChanged)
        Q_PROPERTY(LabelSequence *labelSequence READ labelSequence NOTIFY labelSequenceChanged)
    public:
        ~Label() override;

        /**
         * @brief Gets pos.
         *
         * This property is the position index of the label in the label sequence.
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
        void posChanged(int pos);
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
