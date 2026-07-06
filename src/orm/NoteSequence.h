#ifndef DSPXMODEL_NOTESEQUENCE_H
#define DSPXMODEL_NOTESEQUENCE_H

#include <vector>

#include <QList>
#include <QObject>
#include <QScopedPointer>
#include <qqmlintegration.h>

#include <dspxmodelORM/DSPXModelORMGlobal.h>

namespace opendspx {
    struct Note;
}

namespace dspx {

    class Note;
    class SingingClip;

    class NoteSequencePrivate;

    /**
     * @brief Note sequence.
     */
    class DSPXMODEL_ORM_EXPORT NoteSequence : public QObject {
        Q_OBJECT
        QML_ELEMENT
        QML_UNCREATABLE("")
        Q_DECLARE_PRIVATE(NoteSequence)
        Q_PROPERTY(int size READ size NOTIFY sizeChanged)
        Q_PROPERTY(Note *firstItem READ firstItem NOTIFY firstItemChanged)
        Q_PROPERTY(Note *lastItem READ lastItem NOTIFY lastItemChanged)
        Q_PROPERTY(SingingClip *singingClip READ singingClip CONSTANT)
    public:
        /**
         * @brief Gets size.
         * @post size() >= 0.
         */
        int size() const;
        /**
         * @brief Gets first item.
         */
        Note *firstItem() const;
        /**
         * @brief Gets last item.
         */
        Note *lastItem() const;

        /**
         * @brief Gets slice.
         * @pre position >= 0.
         * @pre length >= 0.
         * @returns Items of which the [position index, position index + length index) intersects with [position, position + length).
         */
        Q_INVOKABLE QList<Note *> slice(int position, int length) const;
        /**
         * @brief Gets whether item is contained.
         */
        Q_INVOKABLE bool contains(Note *item) const;
        /**
         * @brief Inserts item.
         * @pre item is not null.
         * @post If successful, item is contained in this sequence.
         * @returns true if successful, false if item is already contained in this sequence or another sequence.
         */
        Q_INVOKABLE bool insertItem(Note *item);
        /**
         * @brief Removes item.
         * @pre item is not null.
         * @post If successful, item is not contained in this sequence.
         * @returns true if successful, false if item is not contained in this sequence.
         */
        Q_INVOKABLE bool removeItem(Note *item);
        /**
         * @brief Moves item.
         * @pre item is not null.
         * @pre sequence is not null.
         * @post If successful, item is contained in sequence.
         * @returns true if successful, false if item is not contained in this sequence, or item is already contained in
         * the target sequence or another sequence.
         */
        Q_INVOKABLE bool moveItem(Note *item, NoteSequence *sequence);

        /**
         * @brief Gets singing clip.
         * @post singingClip() != nullptr.
         */
        SingingClip *singingClip() const;

        /**
         * @brief Converts to OpenDSPX note sequence.
         */
        std::vector<opendspx::Note> toOpenDSPX() const;
        /**
         * @brief Converts from OpenDSPX note sequence.
         * @note Typically, this method SHOULD only be called on a newly created object.
         * @pre singingClip()->model()->document()->transaction() != nullptr && singingClip()->model()->document()->transaction()->state() == dini::TransactionState::Active.
         * @pre notes must be valid.
         */
        void fromOpenDSPX(const std::vector<opendspx::Note> &notes);

    signals:
        void sizeChanged(int size);
        void firstItemChanged(Note *firstItem);
        void lastItemChanged(Note *lastItem);
        void itemAboutToInsert(Note *item, NoteSequence *sequenceFromWhichMoved = nullptr);
        void itemInserted(Note *item, NoteSequence *sequenceFromWhichMoved = nullptr);
        void itemAboutToRemove(Note *item, NoteSequence *sequenceToWhichMoved = nullptr);
        void itemRemoved(Note *item, NoteSequence *sequenceToWhichMoved = nullptr);

    private:
        ~NoteSequence() override;

        explicit NoteSequence(SingingClip *singingClip);

        QScopedPointer<NoteSequencePrivate> d_ptr;
    };

}

#endif // DSPXMODEL_NOTESEQUENCE_H
