#ifndef DSPXMODEL_DYNAMICMIXINGANCHORSEQUENCE_H
#define DSPXMODEL_DYNAMICMIXINGANCHORSEQUENCE_H

#include <QList>
#include <QObject>
#include <QScopedPointer>
#include <vector>

#include <dspxmodelORM/DSPXModelORMGlobal.h>

namespace opendspx {
    struct DynamicMixingAnchor;
}

namespace dspx {

    class DynamicMixingAnchor;
    class Sources;

    class DynamicMixingAnchorSequencePrivate;

    /**
     * @brief Dynamic mixing anchor sequence.
     */
    class DSPXMODEL_ORM_EXPORT DynamicMixingAnchorSequence : public QObject {
        Q_OBJECT
        Q_DECLARE_PRIVATE(DynamicMixingAnchorSequence)
        Q_PROPERTY(int size READ size NOTIFY sizeChanged)
        Q_PROPERTY(DynamicMixingAnchor *firstItem READ firstItem NOTIFY firstItemChanged)
        Q_PROPERTY(DynamicMixingAnchor *lastItem READ lastItem NOTIFY lastItemChanged)
        Q_PROPERTY(Sources *sources READ sources CONSTANT)
    public:
        /**
         * @brief Gets size.
         * @post size() >= 0.
         */
        int size() const;
        /**
         * @brief Gets first item.
         */
        DynamicMixingAnchor *firstItem() const;
        /**
         * @brief Gets last item.
         */
        DynamicMixingAnchor *lastItem() const;

        /**
         * @brief Gets slice.
         * @pre position >= 0.
         * @pre length >= 0.
         * @returns Items of which the position index property is in the range [position, position + length).
         */
        Q_INVOKABLE QList<DynamicMixingAnchor *> slice(int position, int length) const;
        /**
         * @brief Gets whether item is contained.
         */
        Q_INVOKABLE bool contains(DynamicMixingAnchor *item) const;
        /**
         * @brief Inserts item.
         * @pre item is not null.
         * @post If successful, item is contained in this sequence.
         * @returns true if successful, false if item is already contained in this sequence or another sequence.
         */
        Q_INVOKABLE bool insertItem(DynamicMixingAnchor *item);
        /**
         * @brief Removes item.
         * @pre item is not null.
         * @post If successful, item is not contained in this sequence.
         * @returns true if successful, false if item is not contained in this sequence.
         */
        Q_INVOKABLE bool removeItem(DynamicMixingAnchor *item);
        /**
         * @brief Moves item.
         * @pre item is not null.
         * @pre sequence is not null.
         * @post If successful, item is contained in sequence.
         * @returns true if successful, false if item is not contained in this sequence, or item is already contained in
         * the target sequence or another sequence.
         */
        Q_INVOKABLE bool moveItem(DynamicMixingAnchor *item, DynamicMixingAnchorSequence *sequence);

        /**
         * @brief Converts to OpenDSPX dynamic mixing anchors.
         */
        std::vector<opendspx::DynamicMixingAnchor> toOpenDSPX() const;
        /**
         * @brief Converts from OpenDSPX dynamic mixing anchors.
         * @note Typically, this method SHOULD only be called on a newly created object.
         * @pre sources()->model()->document()->transaction() != nullptr && sources()->model()->document()->transaction()->state() == dini::TransactionState::Active.
         */
        void fromOpenDSPX(const std::vector<opendspx::DynamicMixingAnchor> &anchors);

        /**
         * @brief Gets sources.
         * @post sources() != nullptr.
         */
        Sources *sources() const;

    signals:
        void sizeChanged(int size);
        void firstItemChanged(DynamicMixingAnchor *firstItem);
        void lastItemChanged(DynamicMixingAnchor *lastItem);
        void itemAboutToInsert(DynamicMixingAnchor *item, DynamicMixingAnchorSequence *sequenceFromWhichMoved = nullptr);
        void itemInserted(DynamicMixingAnchor *item, DynamicMixingAnchorSequence *sequenceFromWhichMoved = nullptr);
        void itemAboutToRemove(DynamicMixingAnchor *item, DynamicMixingAnchorSequence *sequenceToWhichMoved = nullptr);
        void itemRemoved(DynamicMixingAnchor *item, DynamicMixingAnchorSequence *sequenceToWhichMoved = nullptr);

    private:
        ~DynamicMixingAnchorSequence() override;

        explicit DynamicMixingAnchorSequence(Sources *sources);

        QScopedPointer<DynamicMixingAnchorSequencePrivate> d_ptr;
    };

}

#endif // DSPXMODEL_DYNAMICMIXINGANCHORSEQUENCE_H
