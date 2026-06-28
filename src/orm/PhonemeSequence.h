#ifndef DSPXMODEL_PHONEMESEQUENCE_H
#define DSPXMODEL_PHONEMESEQUENCE_H

#include <QList>
#include <QObject>

#include <dspxmodelORM/DSPXModelORMGlobal.h>

namespace dspx {

    class Note;
    class Phoneme;

    class PhonemeSequencePrivate;

    /**
     * @brief Phoneme sequence.
     */
    class DSPXMODEL_ORM_EXPORT PhonemeSequence : public QObject {
        Q_OBJECT
        Q_PROPERTY(int size READ size NOTIFY sizeChanged)
        Q_PROPERTY(Phoneme *firstItem READ firstItem NOTIFY firstItemChanged)
        Q_PROPERTY(Phoneme *lastItem READ lastItem NOTIFY lastItemChanged)
        Q_PROPERTY(PhonemeRole role READ role CONSTANT)
        Q_PROPERTY(Note *note READ note CONSTANT)
    public:
        /**
         * @brief Phoneme role.
         */
        enum PhonemeRole {
            Original,
            Edited,
        };
        Q_ENUM(PhonemeRole)

        ~PhonemeSequence() override;

        /**
         * @brief Gets size.
         * @post size() >= 0.
         */
        int size() const;
        /**
         * @brief Gets first item.
         */
        Phoneme *firstItem() const;
        /**
         * @brief Gets last item.
         */
        Phoneme *lastItem() const;

        /**
         * @brief Gets slice.
         * @pre length >= 0.
         * @returns Items of which the start property is in the range [position, position + length).
         */
        Q_INVOKABLE QList<Phoneme *> slice(int position, int length) const;
        /**
         * @brief Gets whether item is contained.
         */
        Q_INVOKABLE bool contains(Phoneme *item) const;
        /**
         * @brief Inserts item.
         * @pre item is not null.
         * @post If successful, item is contained in this sequence.
         * @returns true if successful, false if item is already contained in this sequence or another sequence.
         */
        Q_INVOKABLE bool insertItem(Phoneme *item);
        /**
         * @brief Removes item.
         * @pre item is not null.
         * @post If successful, item is not contained in this sequence.
         * @returns true if successful, false if item is not contained in this sequence.
         */
        Q_INVOKABLE bool removeItem(Phoneme *item);
        /**
         * @brief Moves item.
         * @pre item is not null.
         * @pre sequence is not null.
         * @post If successful, item is contained in sequence.
         * @returns true if successful, false if item is not contained in this sequence, or item is already contained in
         * the target sequence or another sequence.
         */
        Q_INVOKABLE bool moveItem(Phoneme *item, PhonemeSequence *sequence);

        /**
         * @brief Gets role.
         */
        PhonemeRole role() const;
        /**
         * @brief Gets note.
         * @post note() != nullptr.
         */
        Note *note() const;

    signals:
        void sizeChanged(int size);
        void firstItemChanged(Phoneme *firstItem);
        void lastItemChanged(Phoneme *lastItem);
        void itemAboutToInsert(Phoneme *item, PhonemeSequence *sequenceFromWhichMoved = nullptr);
        void itemInserted(Phoneme *item, PhonemeSequence *sequenceFromWhichMoved = nullptr);
        void itemAboutToRemove(Phoneme *item, PhonemeSequence *sequenceToWhichMoved = nullptr);
        void itemRemoved(Phoneme *item, PhonemeSequence *sequenceToWhichMoved = nullptr);

    private:
        explicit PhonemeSequence(Note *note, PhonemeRole role);

        QScopedPointer<PhonemeSequencePrivate> d_ptr;
    };

}

#endif // DSPXMODEL_PHONEMESEQUENCE_H
