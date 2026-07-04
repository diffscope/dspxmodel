#ifndef DSPXMODEL_TEMPOSEQUENCE_H
#define DSPXMODEL_TEMPOSEQUENCE_H

#include <vector>

#include <QList>
#include <QObject>
#include <QScopedPointer>

#include <dspxmodelORM/DSPXModelORMGlobal.h>

namespace opendspx {
    struct Tempo;
}

namespace dspx {

    class Model;
    class Tempo;

    class TempoSequencePrivate;

    /**
     * @brief Tempo sequence.
     */
    class DSPXMODEL_ORM_EXPORT TempoSequence : public QObject {
        Q_OBJECT
        Q_DECLARE_PRIVATE(TempoSequence)
        Q_PROPERTY(int size READ size NOTIFY sizeChanged)
        Q_PROPERTY(Tempo *firstItem READ firstItem NOTIFY firstItemChanged)
        Q_PROPERTY(Tempo *lastItem READ lastItem NOTIFY lastItemChanged)
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
        Tempo *firstItem() const;
        /**
         * @brief Gets last item.
         */
        Tempo *lastItem() const;

        /**
         * @brief Gets slice.
         * @pre position >= 0.
         * @pre length >= 0.
         * @returns Items of which the position index property is in the range [position, position + length).
         */
        Q_INVOKABLE QList<Tempo *> slice(int position, int length) const;
        /**
         * @brief Gets whether item is contained.
         * @pre item == nullptr || item->model() == model().
         */
        Q_INVOKABLE bool contains(Tempo *item) const;
        /**
         * @brief Inserts item.
         *
         * If another item in this sequence already has the inserted item's position, that item is removed from this
         * sequence before the inserted item is added.
         *
         * @pre model()->document()->transaction() != nullptr && model()->document()->transaction()->state() == dini::TransactionState::Active.
         * @pre item != nullptr && item->model() == model().
         * @post If successful, item is contained in this sequence.
         * @returns true if successful, false if item is already contained in this sequence or another sequence.
         */
        Q_INVOKABLE bool insertItem(Tempo *item);
        /**
         * @brief Removes item.
         * @pre model()->document()->transaction() != nullptr && model()->document()->transaction()->state() == dini::TransactionState::Active.
         * @pre item != nullptr && item->model() == model().
         * @post If successful, item is not contained in this sequence.
         * @returns true if successful, false if item is not contained in this sequence.
         */
        Q_INVOKABLE bool removeItem(Tempo *item);
        /**
         * @brief Gets model.
         * @post model() != nullptr.
         */
        Model *model() const;

        /**
         * @brief Converts to OpenDSPX tempo sequence.
         */
        std::vector<opendspx::Tempo> toOpenDSPX() const;
        /**
         * @brief Converts from OpenDSPX tempo sequence.
         * @pre model()->document()->transaction() != nullptr && model()->document()->transaction()->state() == dini::TransactionState::Active.
         * @pre tempos must be valid.
         */
        void fromOpenDSPX(const std::vector<opendspx::Tempo> &tempos);

    signals:
        void sizeChanged(int size);
        void firstItemChanged(Tempo *firstItem);
        void lastItemChanged(Tempo *lastItem);
        void itemAboutToInsert(Tempo *item);
        void itemInserted(Tempo *item);
        void itemAboutToRemove(Tempo *item);
        void itemRemoved(Tempo *item);

    private:
        ~TempoSequence() override;

        explicit TempoSequence(Model *model);

        QScopedPointer<TempoSequencePrivate> d_ptr;
    };

}

#endif // DSPXMODEL_TEMPOSEQUENCE_H
