#ifndef DSPXMODEL_TIMESIGNATURESEQUENCE_H
#define DSPXMODEL_TIMESIGNATURESEQUENCE_H

#include <vector>

#include <QList>
#include <QObject>
#include <QScopedPointer>
#include <qqmlintegration.h>

#include <dspxmodelORM/DSPXModelORMGlobal.h>
#include <dspxmodelORM/RangeHelpers.h>

namespace opendspx {
    struct TimeSignature;
}

namespace dspx {

    class Model;
    class TimeSignature;

    class TimeSignatureSequencePrivate;

    /**
     * @brief Time signature sequence.
     */
    class DSPXMODEL_ORM_EXPORT TimeSignatureSequence : public QObject {
        Q_OBJECT
        QML_ELEMENT
        QML_UNCREATABLE("")
        Q_DECLARE_PRIVATE(TimeSignatureSequence)
        Q_PROPERTY(int size READ size NOTIFY sizeChanged)
        Q_PROPERTY(TimeSignature *firstItem READ firstItem NOTIFY firstItemChanged)
        Q_PROPERTY(TimeSignature *lastItem READ lastItem NOTIFY lastItemChanged)
        Q_PROPERTY(Model *model READ model CONSTANT)
        Q_PRIVATE_PROPERTY(d_func()->jsIterable, QJSValue iterable READ iterable CONSTANT)
    public:
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
         * @pre model()->document()->transaction() != nullptr && model()->document()->transaction()->state() == dini::TransactionState::Active.
         * @pre item != nullptr && item->model() == model().
         * @pre No other item in this sequence has item->index().
         * @post If successful, item is contained in this sequence.
         * @returns true if successful, false if item is already contained in this sequence or another sequence.
         */
        Q_INVOKABLE bool insertItem(TimeSignature *item);
        /**
         * @brief Removes item.
         * @pre model()->document()->transaction() != nullptr && model()->document()->transaction()->state() == dini::TransactionState::Active.
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

        auto asRange() const {
            return impl::SequenceRange(this);
        }

        /**
         * @brief Converts to OpenDSPX time signature sequence.
         */
        std::vector<opendspx::TimeSignature> toOpenDSPX() const;
        /**
         * @brief Converts from OpenDSPX time signature sequence.
         * @note Typically, this method SHOULD only be called on a newly created object.
         * @pre model()->document()->transaction() != nullptr && model()->document()->transaction()->state() == dini::TransactionState::Active.
         * @pre timeSignatures must be valid.
         */
        void fromOpenDSPX(const std::vector<opendspx::TimeSignature> &timeSignatures);

    signals:
        void sizeChanged(int size);
        void firstItemChanged(TimeSignature *firstItem);
        void lastItemChanged(TimeSignature *lastItem);
        void itemAboutToInsert(TimeSignature *item);
        void itemInserted(TimeSignature *item);
        void itemAboutToRemove(TimeSignature *item);
        void itemRemoved(TimeSignature *item);

    private:
        ~TimeSignatureSequence() override;

        explicit TimeSignatureSequence(Model *model);

        QScopedPointer<TimeSignatureSequencePrivate> d_ptr;
    };

}

#endif // DSPXMODEL_TIMESIGNATURESEQUENCE_H
