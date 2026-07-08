#ifndef DSPXMODEL_CLIPSEQUENCE_H
#define DSPXMODEL_CLIPSEQUENCE_H

#include <QList>
#include <QObject>
#include <qqmlintegration.h>

#include <memory>
#include <vector>

#include <dspxmodelORM/DSPXModelORMGlobal.h>
#include <dspxmodelORM/RangeHelpers.h>

namespace opendspx {
    struct Clip;
}

namespace dspx {

    class Clip;
    class Track;

    class ClipSequencePrivate;

    /**
     * @brief Clip sequence.
     */
    class DSPXMODEL_ORM_EXPORT ClipSequence : public QObject {
        Q_OBJECT
        QML_ELEMENT
        QML_UNCREATABLE("")
        Q_DECLARE_PRIVATE(ClipSequence)
        Q_PROPERTY(int size READ size NOTIFY sizeChanged)
        Q_PROPERTY(Clip *firstItem READ firstItem NOTIFY firstItemChanged)
        Q_PROPERTY(Clip *lastItem READ lastItem NOTIFY lastItemChanged)
        Q_PROPERTY(Track *track READ track CONSTANT)
        Q_PRIVATE_PROPERTY(d_func()->jsIterable, QJSValue iterable READ iterable CONSTANT)
    public:
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
         * @brief Converts to OpenDSPX clip refs.
         */
        std::vector<std::shared_ptr<opendspx::Clip>> toOpenDSPX() const;
        /**
         * @brief Converts from OpenDSPX clip refs.
         * @note Typically, this method SHOULD only be called on a newly created object.
         * @pre track()->model()->document()->transaction() != nullptr && track()->model()->document()->transaction()->state() == dini::TransactionState::Active.
         * @pre clips must be valid.
         */
        void fromOpenDSPX(const std::vector<std::shared_ptr<opendspx::Clip>> &clips);

        /**
         * @brief Gets track.
         * @post track() != nullptr.
         */
        Track *track() const;

        auto asRange() const {
            return impl::SequenceRange(this);
        }

    signals:
        void sizeChanged(int size);
        void firstItemChanged(Clip *firstItem);
        void lastItemChanged(Clip *lastItem);
        void itemAboutToInsert(Clip *item, ClipSequence *sequenceFromWhichMoved = nullptr);
        void itemInserted(Clip *item, ClipSequence *sequenceFromWhichMoved = nullptr);
        void itemAboutToRemove(Clip *item, ClipSequence *sequenceToWhichMoved = nullptr);
        void itemRemoved(Clip *item, ClipSequence *sequenceToWhichMoved = nullptr);

    private:
        ~ClipSequence() override;

        explicit ClipSequence(Track *track);

        QScopedPointer<ClipSequencePrivate> d_ptr;
    };

}

#endif // DSPXMODEL_CLIPSEQUENCE_H
