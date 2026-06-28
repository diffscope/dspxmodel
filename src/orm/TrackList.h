#ifndef DSPXMODEL_TRACKLIST_H
#define DSPXMODEL_TRACKLIST_H

#include <QList>
#include <QObject>

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
        Q_PROPERTY(int size READ size NOTIFY sizeChanged)
        Q_PROPERTY(QList<Track *> items READ items NOTIFY itemsChanged)
        Q_PROPERTY(Model *model READ model CONSTANT)
    public:
        ~TrackList() override;

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
         */
        Q_INVOKABLE bool contains(Track *item) const;
        /**
         * @brief Gets item.
         * @pre index >= 0.
         */
        Q_INVOKABLE Track *item(int index) const;
        /**
         * @brief Inserts item.
         * @pre index >= 0.
         * @pre item is not null.
         * @post If successful, item is contained in this list.
         */
        Q_INVOKABLE bool insertItem(int index, Track *item);
        /**
         * @brief Removes item.
         * @pre index >= 0.
         * @post If successful, size may change.
         */
        Q_INVOKABLE bool removeItem(int index);
        /**
         * @brief Moves item.
         * @pre index >= 0.
         * @pre list is not null.
         * @pre newIndex >= 0.
         * @post If successful, item is contained in list.
         */
        Q_INVOKABLE bool moveItem(int index, TrackList *list, int newIndex);
        /**
         * @brief Rotates items.
         * @pre leftIndex >= 0.
         * @pre middleIndex >= leftIndex.
         * @pre rightIndex >= middleIndex.
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
        void itemAboutToInsert(int index, Track *item, TrackList *listFromWhichMoved = nullptr);
        void itemInserted(int index, Track *item, TrackList *listFromWhichMoved = nullptr);
        void itemAboutToRemove(int index, Track *item, TrackList *listToWhichMoved = nullptr);
        void itemRemoved(int index, Track *item, TrackList *listToWhichMoved = nullptr);
        void aboutToRotate(int leftIndex, int middleIndex, int rightIndex);
        void rotated(int leftIndex, int middleIndex, int rightIndex);

    private:
        explicit TrackList(Model *model);

        QScopedPointer<TrackListPrivate> d_ptr;
    };

}

#endif // DSPXMODEL_TRACKLIST_H
