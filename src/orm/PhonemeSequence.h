#ifndef DSPXMODEL_PHONEMESEQUENCE_H
#define DSPXMODEL_PHONEMESEQUENCE_H

#include <vector>

#include <QList>
#include <QObject>
#include <QScopedPointer>
#include <qqmlintegration.h>

#include <dspxmodelORM/DSPXModelORMGlobal.h>
#include <dspxmodelORM/RangeHelpers.h>

namespace opendspx {
    struct Phoneme;
}

namespace dspx {

    class Note;
    class Phoneme;

    class PhonemeSequencePrivate;

    /**
     * @brief Phoneme sequence.
     */
    class DSPXMODEL_ORM_EXPORT PhonemeSequence : public QObject {
        Q_OBJECT
        QML_ELEMENT
        QML_UNCREATABLE("")
        Q_DECLARE_PRIVATE(PhonemeSequence)
        Q_PROPERTY(int size READ size NOTIFY sizeChanged)
        Q_PROPERTY(Phoneme *firstItem READ firstItem NOTIFY firstItemChanged)
        Q_PROPERTY(Phoneme *lastItem READ lastItem NOTIFY lastItemChanged)
        Q_PROPERTY(PhonemeRole role READ role CONSTANT)
        Q_PROPERTY(Note *note READ note CONSTANT)
        Q_PRIVATE_PROPERTY(d_func()->jsIterable, QJSValue iterable READ iterable CONSTANT)
    public:
        /**
         * @brief Phoneme role.
         */
        enum PhonemeRole {
            Original,
            Edited,
        };
        Q_ENUM(PhonemeRole)

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

        auto asRange() const {
            return impl::SequenceRange(this);
        }

        /**
         * @brief Converts to OpenDSPX phonemes.
         */
        std::vector<opendspx::Phoneme> toOpenDSPX() const;
        /**
         * @brief Converts from OpenDSPX phonemes.
         * @note Typically, this method SHOULD only be called on a newly created object.
         * @pre note()->model()->document()->transaction() != nullptr && note()->model()->document()->transaction()->state() == dini::TransactionState::Active.
         */
        void fromOpenDSPX(const std::vector<opendspx::Phoneme> &phonemes);

    signals:
        void sizeChanged(int size);
        void firstItemChanged(Phoneme *firstItem);
        void lastItemChanged(Phoneme *lastItem);
        void itemAboutToInsert(Phoneme *item, PhonemeSequence *sequenceFromWhichMoved = nullptr);
        void itemInserted(Phoneme *item, PhonemeSequence *sequenceFromWhichMoved = nullptr);
        void itemAboutToRemove(Phoneme *item, PhonemeSequence *sequenceToWhichMoved = nullptr);
        void itemRemoved(Phoneme *item, PhonemeSequence *sequenceToWhichMoved = nullptr);

    private:
        ~PhonemeSequence() override;

        explicit PhonemeSequence(Note *note, PhonemeRole role);

        QScopedPointer<PhonemeSequencePrivate> d_ptr;
    };

}

#endif // DSPXMODEL_PHONEMESEQUENCE_H
