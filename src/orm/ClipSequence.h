#ifndef DSPXMODEL_CLIPSEQUENCE_H
#define DSPXMODEL_CLIPSEQUENCE_H

#include <QList>
#include <QObject>

namespace dspx {

    class Clip;
    class Track;

    class ClipSequencePrivate;

    /**
     * @brief Clip sequence.
     */
    class DSPXMODEL_ORM_EXPORT ClipSequence : public QObject {
        Q_OBJECT
        Q_PROPERTY(int size READ size NOTIFY sizeChanged)
        Q_PROPERTY(Clip *firstItem READ firstItem NOTIFY firstItemChanged)
        Q_PROPERTY(Clip *lastItem READ lastItem NOTIFY lastItemChanged)
        Q_PROPERTY(Track *track READ track CONSTANT)
    public:
        ~ClipSequence() override;

        /**
         * @brief Gets size.
         */
        int size() const;
        /**
         * @brief Gets first item.
         */
        Clip *firstItem() const;
        /**
         * @brief Gets last item.
         */
        Clip *lastItem() const;

        /**
         * @brief Gets slice.
         * @pre position >= 0.
         * @pre length >= 0.
         * @returns Items of which the range intersects with [position, position + length).
         */
        Q_INVOKABLE QList<Clip *> slice(int position, int length) const;
        /**
         * @brief Gets whether item is contained.
         */
        Q_INVOKABLE bool contains(Clip *item) const;
        /**
         * @brief Inserts item.
         * @pre item is not null.
         * @post If successful, item is contained in this sequence.
         * @returns true if successful, false if item is already contained in this sequence or another sequence.
         */
        Q_INVOKABLE bool insertItem(Clip *item);
        /**
         * @brief Removes item.
         * @pre item is not null.
         * @post If successful, item is not contained in this sequence.
         * @returns true if successful, false if item is not contained in this sequence.
         */
        Q_INVOKABLE bool removeItem(Clip *item);
        /**
         * @brief Moves item.
         * @pre item is not null.
         * @pre sequence is not null.
         * @post If successful, item is contained in sequence.
         * @returns true if successful, false if item is not contained in this sequence, or item is already contained in
         * the target sequence or another sequence.
         */
        Q_INVOKABLE bool moveItem(Clip *item, ClipSequence *sequence);

        /**
         * @brief Gets track.
         * @post track() != nullptr.
         */
        Track *track() const;

    signals:
        void sizeChanged(int size);
        void firstItemChanged(Clip *firstItem);
        void lastItemChanged(Clip *lastItem);
        void itemAboutToInsert(Clip *item, ClipSequence *sequenceFromWhichMoved = nullptr);
        void itemInserted(Clip *item, ClipSequence *sequenceFromWhichMoved = nullptr);
        void itemAboutToRemove(Clip *item, ClipSequence *sequenceToWhichMoved = nullptr);
        void itemRemoved(Clip *item, ClipSequence *sequenceToWhichMoved = nullptr);

    private:
        explicit ClipSequence(Track *track);

        QScopedPointer<ClipSequencePrivate> d_ptr;
    };

}

#endif // DSPXMODEL_CLIPSEQUENCE_H
