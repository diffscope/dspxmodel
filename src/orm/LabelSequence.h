#ifndef DSPXMODEL_LABELSEQUENCE_H
#define DSPXMODEL_LABELSEQUENCE_H

#include <vector>

#include <QList>
#include <QObject>
#include <QScopedPointer>
#include <qqmlintegration.h>

#include <dspxmodelORM/DSPXModelORMGlobal.h>

namespace opendspx {
    struct Label;
}

namespace dspx {

    class Label;
    class Model;

    class LabelSequencePrivate;

    /**
     * @brief Label sequence.
     */
    class DSPXMODEL_ORM_EXPORT LabelSequence : public QObject {
        Q_OBJECT
        QML_ELEMENT
        QML_UNCREATABLE("")
        Q_DECLARE_PRIVATE(LabelSequence)
        Q_PROPERTY(int size READ size NOTIFY sizeChanged)
        Q_PROPERTY(Label *firstItem READ firstItem NOTIFY firstItemChanged)
        Q_PROPERTY(Label *lastItem READ lastItem NOTIFY lastItemChanged)
        Q_PROPERTY(Model *model READ model CONSTANT)
    public:
        /**
         * @brief Gets size.
         * @post size() >= 0.
         */
        int size() const;
        /**
         * @brief Gets first item.
         */
        Label *firstItem() const;
        /**
         * @brief Gets last item.
         */
        Label *lastItem() const;

        /**
         * @brief Gets slice.
         * @pre position >= 0.
         * @pre length >= 0.
         * @returns Items of which the position index property is in the range [position, position + length).
         */
        Q_INVOKABLE QList<Label *> slice(int position, int length) const;
        /**
         * @brief Gets whether item is contained.
         * @pre item == nullptr || item->model() == model().
         */
        Q_INVOKABLE bool contains(Label *item) const;
        /**
         * @brief Inserts item.
         * @pre model()->document()->transaction() != nullptr && model()->document()->transaction()->state() == dini::TransactionState::Active.
         * @pre item != nullptr && item->model() == model().
         * @post If successful, item is contained in this sequence.
         * @returns true if successful, false if item is already contained in this sequence or another sequence.
         */
        Q_INVOKABLE bool insertItem(Label *item);
        /**
         * @brief Removes item.
         * @pre model()->document()->transaction() != nullptr && model()->document()->transaction()->state() == dini::TransactionState::Active.
         * @pre item != nullptr && item->model() == model().
         * @post If successful, item is not contained in this sequence.
         * @returns true if successful, false if item is not contained in this sequence.
         */
        Q_INVOKABLE bool removeItem(Label *item);
        /**
         * @brief Gets model.
         * @post model() != nullptr.
         */
        Model *model() const;

        /**
         * @brief Converts to OpenDSPX label sequence.
         */
        std::vector<opendspx::Label> toOpenDSPX() const;
        /**
         * @brief Converts from OpenDSPX label sequence.
         * @note Typically, this method SHOULD only be called on a newly created object.
         * @pre model()->document()->transaction() != nullptr && model()->document()->transaction()->state() == dini::TransactionState::Active.
         * @pre labels must be valid.
         */
        void fromOpenDSPX(const std::vector<opendspx::Label> &labels);

    signals:
        void sizeChanged(int size);
        void firstItemChanged(Label *firstItem);
        void lastItemChanged(Label *lastItem);
        void itemAboutToInsert(Label *item);
        void itemInserted(Label *item);
        void itemAboutToRemove(Label *item);
        void itemRemoved(Label *item);

    private:
        ~LabelSequence() override;

        explicit LabelSequence(Model *model);

        QScopedPointer<LabelSequencePrivate> d_ptr;
    };

}

#endif // DSPXMODEL_LABELSEQUENCE_H
