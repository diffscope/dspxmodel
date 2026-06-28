#ifndef DSPXMODEL_VIBRATOPOINTDATAARRAY_H
#define DSPXMODEL_VIBRATOPOINTDATAARRAY_H

#include <QList>
#include <QObject>
#include <QPointF>

#include <dspxmodelORM/DSPXModelORMGlobal.h>

namespace dspx {

    class Note;

    class VibratoPointDataArrayPrivate;

    /**
     * @brief Vibrato point data array.
     */
    class DSPXMODEL_ORM_EXPORT VibratoPointDataArray : public QObject {
        Q_OBJECT
        Q_PROPERTY(int size READ size NOTIFY sizeChanged)
        Q_PROPERTY(QList<QPointF> items READ items NOTIFY itemsChanged)
        Q_PROPERTY(VibratoPointRole role READ role CONSTANT)
        Q_PROPERTY(Note *note READ note CONSTANT)
    public:
        /**
         * @brief Vibrato point data role.
         */
        enum VibratoPointRole {
            Amplitude,
            Frequency,
        };
        Q_ENUM(VibratoPointRole)

        ~VibratoPointDataArray() override;

        /**
         * @brief Gets size.
         * @post size() >= 0.
         */
        int size() const;
        /**
         * @brief Gets items.
         */
        QList<QPointF> items() const;

        /**
         * @brief Gets slice.
         * @pre index >= 0.
         * @pre length >= 0.
         * @returns Items in the range [index, index + length).
         */
        Q_INVOKABLE QList<QPointF> slice(int index, int length) const;
        /**
         * @brief Splices items.
         * @pre index >= 0.
         * @pre length >= 0.
         * @post If successful, items are spliced.
         */
        Q_INVOKABLE bool splice(int index, int length, const QList<QPointF> &values);
        /**
         * @brief Rotates items.
         * @pre leftIndex >= 0.
         * @pre middleIndex >= leftIndex.
         * @pre rightIndex >= middleIndex.
         * @post If successful, items are rotated.
         */
        Q_INVOKABLE bool rotate(int leftIndex, int middleIndex, int rightIndex);

        /**
         * @brief Gets role.
         */
        VibratoPointRole role() const;
        /**
         * @brief Gets note.
         * @post note() != nullptr.
         */
        Note *note() const;

    signals:
        void sizeChanged(int size);
        void itemsChanged();
        void aboutToSplice(int index, int length, const QList<QPointF> &values);
        void spliced(int index, int length, const QList<QPointF> &values);
        void aboutToRotate(int leftIndex, int middleIndex, int rightIndex);
        void rotated(int leftIndex, int middleIndex, int rightIndex);

    private:
        explicit VibratoPointDataArray(Note *note, VibratoPointRole role);

        QScopedPointer<VibratoPointDataArrayPrivate> d_ptr;
    };

}

#endif // DSPXMODEL_VIBRATOPOINTDATAARRAY_H
