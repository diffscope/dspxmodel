#ifndef DSPXMODEL_KEYSIGNATURESEQUENCE_H
#define DSPXMODEL_KEYSIGNATURESEQUENCE_H

#include <vector>

#include <QList>
#include <QObject>
#include <QScopedPointer>
#include <qqmlintegration.h>

#include <dspxmodelORM/DSPXModelORMGlobal.h>
#include <dspxmodelORM/RangeHelpers.h>
#include <nlohmann/json_fwd.hpp>

namespace dspx {

    class KeySignature;
    class Model;

    class KeySignatureSequencePrivate;

    /**
     * @brief Key signature sequence.
     */
    class DSPXMODEL_ORM_EXPORT KeySignatureSequence : public QObject {
        Q_OBJECT
        QML_ELEMENT
        QML_UNCREATABLE("")
        Q_DECLARE_PRIVATE(KeySignatureSequence)
        Q_PROPERTY(int size READ size NOTIFY sizeChanged)
        Q_PROPERTY(KeySignature *firstItem READ firstItem NOTIFY firstItemChanged)
        Q_PROPERTY(KeySignature *lastItem READ lastItem NOTIFY lastItemChanged)
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
        KeySignature *firstItem() const;
        /**
         * @brief Gets last item.
         */
        KeySignature *lastItem() const;

        /**
         * @brief Gets slice.
         * @pre position >= 0.
         * @pre length >= 0.
         * @returns Items of which the position index property is in the range [position, position + length).
         */
        Q_INVOKABLE QList<KeySignature *> slice(int position, int length) const;
        /**
         * @brief Gets whether item is contained.
         * @pre item == nullptr || item->model() == model().
         */
        Q_INVOKABLE bool contains(KeySignature *item) const;
        /**
         * @brief Inserts item.
         * @pre model()->document()->transaction() != nullptr && model()->document()->transaction()->state() == dini::TransactionState::Active.
         * @pre item != nullptr && item->model() == model().
         * @pre No other item in this sequence has item->position().
         * @post If successful, item is contained in this sequence.
         * @returns true if successful, false if item is already contained in this sequence or another sequence.
         */
        Q_INVOKABLE bool insertItem(KeySignature *item);
        /**
         * @brief Removes item.
         * @pre model()->document()->transaction() != nullptr && model()->document()->transaction()->state() == dini::TransactionState::Active.
         * @pre item != nullptr && item->model() == model().
         * @post If successful, item is not contained in this sequence.
         * @returns true if successful, false if item is not contained in this sequence.
         */
        Q_INVOKABLE bool removeItem(KeySignature *item);
        /**
         * @brief Gets model.
         * @post model() != nullptr.
         */
        Model *model() const;

        auto asRange() const {
            return impl::SequenceRange(this);
        }

        /**
         * @brief Converts to OpenDSPX key signature sequence.
         */
        nlohmann::json toOpenDSPX() const;
        /**
         * @brief Converts from OpenDSPX key signature sequence.
         * @note Typically, this method SHOULD only be called on a newly created object.
         * @pre model()->document()->transaction() != nullptr && model()->document()->transaction()->state() == dini::TransactionState::Active.
         * @pre keySignatures must be valid.
         */
        void fromOpenDSPX(const nlohmann::json &keySignatures);

    signals:
        void sizeChanged(int size);
        void firstItemChanged(KeySignature *firstItem);
        void lastItemChanged(KeySignature *lastItem);
        void itemAboutToInsert(KeySignature *item);
        void itemInserted(KeySignature *item);
        void itemAboutToRemove(KeySignature *item);
        void itemRemoved(KeySignature *item);

    private:
        ~KeySignatureSequence() override;

        explicit KeySignatureSequence(Model *model);

        QScopedPointer<KeySignatureSequencePrivate> d_ptr;
    };

}

#endif // DSPXMODEL_KEYSIGNATURESEQUENCE_H
