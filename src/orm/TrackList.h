#ifndef DSPXMODEL_TRACKLIST_H
#define DSPXMODEL_TRACKLIST_H

#include <QList>
#include <QObject>
#include <QScopedPointer>

#include <dspxmodelORM/DSPXModelORMGlobal.h>

namespace dspx {

    class Model;
    class Track;

    class TrackListPrivate;

    /**
     * @brief Track list.
     */
    class DSPXMODEL_ORM_EXPORT TrackList : public QObject {
        Q_OBJECT
        Q_DECLARE_PRIVATE(TrackList)
        Q_PROPERTY(int size READ size NOTIFY sizeChanged)
        Q_PROPERTY(QList<Track *> items READ items NOTIFY itemsChanged)
        Q_PROPERTY(Model *model READ model CONSTANT)
    public:
        /**
         * @brief Gets size.
         * @post size() >= 0.
         */
        int size() const;
        /**
         * @brief Gets items.
         */
        QList<Track *> items() const;

        /**
         * @brief Gets whether item is contained.
         * @pre item != nullptr && item->model() == model().
         */
        Q_INVOKABLE bool contains(Track *item) const;
        /**
         * @brief Gets item.
         * @pre index >= 0.
         */
        Q_INVOKABLE Track *item(int index) const;
        /**
         * @brief Inserts item.
         * @pre model()->document()->transaction() != nullptr && model()->document()->transaction()->state() == dini::TransactionState::Active.
         * @pre index >= 0.
         * @pre item != nullptr && item->model() == model().
         * @post If successful, item is contained in this list.
         */
        Q_INVOKABLE bool insertItem(int index, Track *item);
        /**
         * @brief Removes item.
         * @pre model()->document()->transaction() != nullptr && model()->document()->transaction()->state() == dini::TransactionState::Active.
         * @pre index >= 0.
         * @post If successful, size may change.
         */
        Q_INVOKABLE bool removeItem(int index);
        /**
         * @brief Rotates items.
         * @pre model()->document()->transaction() != nullptr && model()->document()->transaction()->state() == dini::TransactionState::Active.
         * @pre leftIndex >= 0 && middleIndex >= leftIndex && rightIndex >= middleIndex.
         * @post If successful, items are rotated.
         */
        Q_INVOKABLE bool rotate(int leftIndex, int middleIndex, int rightIndex);

        /**
         * @brief Gets model.
         * @post model() != nullptr.
         */
        Model *model() const;

    signals:
        void sizeChanged(int size);
        void itemsChanged();
        void itemAboutToInsert(int index, Track *item);
        void itemInserted(int index, Track *item);
        void itemAboutToRemove(int index, Track *item);
        void itemRemoved(int index, Track *item);
        void aboutToRotate(int leftIndex, int middleIndex, int rightIndex);
        void rotated(int leftIndex, int middleIndex, int rightIndex);

    private:
        ~TrackList() override;

        explicit TrackList(Model *model);

        QScopedPointer<TrackListPrivate> d_ptr;
    };

}

#endif // DSPXMODEL_TRACKLIST_H
