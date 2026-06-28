#ifndef DSPXMODEL_SINGERLIST_H
#define DSPXMODEL_SINGERLIST_H

#include <QList>
#include <QObject>

#include <dspxmodelORM/DSPXModelORMGlobal.h>

namespace dspx {

    class MixedSinger;
    class Singer;
    class Sources;

    class SingerListPrivate;

    /**
     * @brief Singer list.
     */
    class DSPXMODEL_ORM_EXPORT SingerList : public QObject {
        Q_OBJECT
        Q_PROPERTY(int size READ size NOTIFY sizeChanged)
        Q_PROPERTY(QList<Singer *> items READ items NOTIFY itemsChanged)
        Q_PROPERTY(Sources *sources READ sources CONSTANT)
        Q_PROPERTY(MixedSinger *mixedSinger READ mixedSinger CONSTANT)
    public:
        ~SingerList() override;

        /**
         * @brief Gets size.
         * @post size() >= 0.
         */
        int size() const;
        /**
         * @brief Gets items.
         */
        QList<Singer *> items() const;

        /**
         * @brief Gets whether item is contained.
         */
        Q_INVOKABLE bool contains(Singer *item) const;
        /**
         * @brief Gets item.
         * @pre index >= 0.
         */
        Q_INVOKABLE Singer *item(int index) const;
        /**
         * @brief Inserts item.
         * @pre index >= 0.
         * @pre item is not null.
         * @post If successful, item is contained in this list.
         */
        Q_INVOKABLE bool insertItem(int index, Singer *item);
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
        Q_INVOKABLE bool moveItem(int index, SingerList *list, int newIndex);
        /**
         * @brief Rotates items.
         * @pre leftIndex >= 0.
         * @pre middleIndex >= leftIndex.
         * @pre rightIndex >= middleIndex.
         * @post If successful, items are rotated.
         */
        Q_INVOKABLE bool rotate(int leftIndex, int middleIndex, int rightIndex);

        /**
         * @brief Gets sources.
         */
        Sources *sources() const;
        /**
         * @brief Gets mixed singer.
         */
        MixedSinger *mixedSinger() const;

    signals:
        void sizeChanged(int size);
        void itemsChanged();
        void itemAboutToInsert(int index, Singer *item, SingerList *listFromWhichMoved = nullptr);
        void itemInserted(int index, Singer *item, SingerList *listFromWhichMoved = nullptr);
        void itemAboutToRemove(int index, Singer *item, SingerList *listToWhichMoved = nullptr);
        void itemRemoved(int index, Singer *item, SingerList *listToWhichMoved = nullptr);
        void aboutToRotate(int leftIndex, int middleIndex, int rightIndex);
        void rotated(int leftIndex, int middleIndex, int rightIndex);

    private:
        explicit SingerList(Sources *sources);
        explicit SingerList(MixedSinger *mixedSinger);

        QScopedPointer<SingerListPrivate> d_ptr;
    };

}

#endif // DSPXMODEL_SINGERLIST_H
