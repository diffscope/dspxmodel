#ifndef DSPXMODEL_AUDIODSPLIST_H
#define DSPXMODEL_AUDIODSPLIST_H

#include <QList>
#include <QObject>
#include <QScopedPointer>
#include <qqmlintegration.h>

#include <stdcorelib/support/json.h>

#include <dspxmodelORM/DSPXModelORMGlobal.h>
#include <dspxmodelORM/RangeHelpers.h>

namespace dspx {

    class AudioDSP;
    class Model;
    class Track;

    class AudioDSPListPrivate;

    /**
     * @brief Audio DSP list.
     */
    class DSPXMODEL_ORM_EXPORT AudioDSPList : public QObject {
        Q_OBJECT
        QML_ELEMENT
        QML_UNCREATABLE("")
        Q_DECLARE_PRIVATE(AudioDSPList)
        Q_PROPERTY(int size READ size NOTIFY sizeChanged)
        Q_PROPERTY(QList<AudioDSP *> items READ items NOTIFY itemsChanged)
        Q_PROPERTY(Track *track READ track CONSTANT)
        Q_PRIVATE_PROPERTY(d_func()->jsIterable, QJSValue iterable READ iterable CONSTANT)
    public:
        /**
         * @brief Gets size.
         * @post size() >= 0.
         */
        int size() const;
        /**
         * @brief Gets items.
         */
        QList<AudioDSP *> items() const;

        /**
         * @brief Gets whether item is contained.
         * @pre item belongs to the same model as this list.
         */
        Q_INVOKABLE bool contains(AudioDSP *item) const;
        /**
         * @brief Gets item.
         * @pre index >= 0.
         */
        Q_INVOKABLE AudioDSP *item(int index) const;
        /**
         * @brief Inserts item.
         * @pre The owner model has an active transaction.
         * @pre index >= 0.
         * @pre item belongs to the same model as this list.
         * @post If successful, item is contained in this list.
         */
        Q_INVOKABLE bool insertItem(int index, AudioDSP *item);
        /**
         * @brief Removes item.
         * @pre The owner model has an active transaction.
         * @pre index >= 0.
         * @post If successful, size may change.
         */
        Q_INVOKABLE AudioDSP *removeItem(int index);
        /**
         * @brief Rotates items.
         * @pre The owner model has an active transaction.
         * @pre leftIndex >= 0 && middleIndex >= leftIndex && rightIndex >= middleIndex.
         * @post If successful, items are rotated.
         */
        Q_INVOKABLE bool rotate(int leftIndex, int middleIndex, int rightIndex);

        /**
         * @brief Gets track.
         * @note Returns nullptr when this list belongs to Model.
         */
        Track *track() const;

        /**
         * @brief Converts to OpenDSPX audio DSP array.
         */
        stdc::JsonArray toOpenDSPX() const;
        /**
         * @brief Converts from OpenDSPX audio DSP array.
         * @note Typically, this method SHOULD only be called on a newly created object.
         * @pre The owner model has an active transaction.
         * @pre audioDSPs must be valid.
         */
        void fromOpenDSPX(const stdc::JsonArray &audioDSPs);

    signals:
        void sizeChanged(int size);
        void itemsChanged();
        void itemAboutToInsert(int index, AudioDSP *item);
        void itemInserted(int index, AudioDSP *item);
        void itemAboutToRemove(int index, AudioDSP *item);
        void itemRemoved(int index, AudioDSP *item);
        void aboutToRotate(int leftIndex, int middleIndex, int rightIndex);
        void rotated(int leftIndex, int middleIndex, int rightIndex);

    private:
        ~AudioDSPList() override;

        explicit AudioDSPList(Track *track);
        explicit AudioDSPList(Model *model);

        QScopedPointer<AudioDSPListPrivate> d_ptr;
    };

}

#endif // DSPXMODEL_AUDIODSPLIST_H
