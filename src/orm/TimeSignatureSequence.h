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
         */
        Q_INVOKABLE bool contains(TimeSignature *item) const;
        /**
         * @brief Inserts item.
         * @pre item is not null.
         * @post If successful, item is contained in this sequence.
         * @returns true if successful, false if item is already contained in this sequence or another sequence.
         */
        Q_INVOKABLE bool insertItem(TimeSignature *item);
        /**
         * @brief Removes item.
         * @pre item is not null.
         * @post If successful, item is not contained in this sequence.
         * @returns true if successful, false if item is not contained in this sequence.
         */
        Q_INVOKABLE bool removeItem(TimeSignature *item);
        /**
         * @brief Moves item.
         * @pre item is not null.
         * @pre sequence is not null.
         * @post If successful, item is contained in sequence.
         * @returns true if successful, false if item is not contained in this sequence, or item is already contained in
         * the target sequence or another sequence.
         */
        Q_INVOKABLE bool moveItem(TimeSignature *item, TimeSignatureSequence *sequence);

        /**
         * @brief Gets model.
         * @post model() != nullptr.
         */
        Model *model() const;

    signals:
        void sizeChanged(int size);
        void firstItemChanged(TimeSignature *firstItem);
        void lastItemChanged(TimeSignature *lastItem);
        void itemAboutToInsert(TimeSignature *item, TimeSignatureSequence *sequenceFromWhichMoved = nullptr);
        void itemInserted(TimeSignature *item, TimeSignatureSequence *sequenceFromWhichMoved = nullptr);
        void itemAboutToRemove(TimeSignature *item, TimeSignatureSequence *sequenceToWhichMoved = nullptr);
        void itemRemoved(TimeSignature *item, TimeSignatureSequence *sequenceToWhichMoved = nullptr);

    private:
        explicit TimeSignatureSequence(Model *model);

        QScopedPointer<TimeSignatureSequencePrivate> d_ptr;
    };

}

#endif // DSPXMODEL_TIMESIGNATURESEQUENCE_H
