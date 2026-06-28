#ifndef DSPXMODEL_LABELSEQUENCE_H
#define DSPXMODEL_LABELSEQUENCE_H

#include <QList>
#include <QObject>

#include <dspxmodelORM/DSPXModelORMGlobal.h>

namespace dspx {

    class Label;
    class Model;

    class LabelSequencePrivate;

    /**
     * @brief Label sequence.
     */
    class DSPXMODEL_ORM_EXPORT LabelSequence : public QObject {
        Q_OBJECT
        Q_PROPERTY(int size READ size NOTIFY sizeChanged)
        Q_PROPERTY(Label *firstItem READ firstItem NOTIFY firstItemChanged)
        Q_PROPERTY(Label *lastItem READ lastItem NOTIFY lastItemChanged)
        Q_PROPERTY(Model *model READ model CONSTANT)
    public:
        ~LabelSequence() override;

        /**
         * @brief Gets size.
         * @post size() >= 0.
         */
        int size() const;
        /**
         * @brief Gets first item.
         */
        Label *firstItem() const;
        /**
         * @brief Gets last item.
         */
        Label *lastItem() const;

        /**
         * @brief Gets slice.
         * @pre position >= 0.
         * @pre length >= 0.
         * @returns Items of which the position index property is in the range [position, position + length).
         */
        Q_INVOKABLE QList<Label *> slice(int position, int length) const;
        /**
         * @brief Gets whether item is contained.
         */
        Q_INVOKABLE bool contains(Label *item) const;
        /**
         * @brief Inserts item.
         * @pre item is not null.
         * @post If successful, item is contained in this sequence.
         * @returns true if successful, false if item is already contained in this sequence or another sequence.
         */
        Q_INVOKABLE bool insertItem(Label *item);
        /**
         * @brief Removes item.
         * @pre item is not null.
         * @post If successful, item is not contained in this sequence.
         * @returns true if successful, false if item is not contained in this sequence.
         */
        Q_INVOKABLE bool removeItem(Label *item);
        /**
         * @brief Moves item.
         * @pre item is not null.
         * @pre sequence is not null.
         * @post If successful, item is contained in sequence.
         * @returns true if successful, false if item is not contained in this sequence, or item is already contained in
         * the target sequence or another sequence.
         */
        Q_INVOKABLE bool moveItem(Label *item, LabelSequence *sequence);

        /**
         * @brief Gets model.
         * @post model() != nullptr.
         */
        Model *model() const;

    signals:
        void sizeChanged(int size);
        void firstItemChanged(Label *firstItem);
        void lastItemChanged(Label *lastItem);
        void itemAboutToInsert(Label *item, LabelSequence *sequenceFromWhichMoved = nullptr);
        void itemInserted(Label *item, LabelSequence *sequenceFromWhichMoved = nullptr);
        void itemAboutToRemove(Label *item, LabelSequence *sequenceToWhichMoved = nullptr);
        void itemRemoved(Label *item, LabelSequence *sequenceToWhichMoved = nullptr);

    private:
        explicit LabelSequence(Model *model);

        QScopedPointer<LabelSequencePrivate> d_ptr;
    };

}

#endif // DSPXMODEL_LABELSEQUENCE_H
