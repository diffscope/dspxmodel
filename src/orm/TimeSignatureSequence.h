#ifndef DSPXMODEL_TIMESIGNATURESEQUENCE_H
#define DSPXMODEL_TIMESIGNATURESEQUENCE_H

#include <QList>
#include <QObject>

#include <dspxmodelORM/DSPXModelORMGlobal.h>

namespace dspx {

    class Model;
    class TimeSignature;

    class TimeSignatureSequencePrivate;

    /**
     * @brief Time signature sequence.
     */
    class DSPXMODEL_ORM_EXPORT TimeSignatureSequence : public QObject {
        Q_OBJECT
        Q_PROPERTY(int size READ size NOTIFY sizeChanged)
        Q_PROPERTY(TimeSignature *firstItem READ firstItem NOTIFY firstItemChanged)
        Q_PROPERTY(TimeSignature *lastItem READ lastItem NOTIFY lastItemChanged)
        Q_PROPERTY(Model *model READ model CONSTANT)
    public:
        ~TimeSignatureSequence() override;

        /**
         * @brief Gets size.
         * @post size() >= 0.
         */
        int size() const;
        /**
         * @brief Gets first item.
         */
        TimeSignature *firstItem() const;
        /**
         * @brief Gets last item.
         */
        TimeSignature *lastItem() const;

        /**
         * @brief Gets slice.
         * @pre position >= 0.
         * @pre length >= 0.
         * @returns Items of which the index property is in the range [position, position + length).
         */
        Q_INVOKABLE QList<TimeSignature *> slice(int position, int length) const;
        /**
         * @brief Gets whether item is contained.
         * @pre item == nullptr || item->model() == model().
         */
        Q_INVOKABLE bool contains(TimeSignature *item) const;
        /**
         * @brief Inserts item.
         * @pre item != nullptr && item->model() == model().
         * @post If successful, item is contained in this sequence.
         * @returns true if successful, false if item is already contained in this sequence or another sequence.
         */
        Q_INVOKABLE bool insertItem(TimeSignature *item);
        /**
         * @brief Removes item.
         * @pre item != nullptr && item->model() == model().
         * @post If successful, item is not contained in this sequence.
         * @returns true if successful, false if item is not contained in this sequence.
         */
        Q_INVOKABLE bool removeItem(TimeSignature *item);
        /**
         * @brief Gets model.
         * @post model() != nullptr.
         */
        Model *model() const;

    signals:
        void sizeChanged(int size);
        void firstItemChanged(TimeSignature *firstItem);
        void lastItemChanged(TimeSignature *lastItem);
        void itemAboutToInsert(TimeSignature *item);
        void itemInserted(TimeSignature *item);
        void itemAboutToRemove(TimeSignature *item);
        void itemRemoved(TimeSignature *item);

    private:
        explicit TimeSignatureSequence(Model *model);

        QScopedPointer<TimeSignatureSequencePrivate> d_ptr;
    };

}

#endif // DSPXMODEL_TIMESIGNATURESEQUENCE_H
