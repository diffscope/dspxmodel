#ifndef DSPXMODEL_VIBRATOPOINTDATAARRAY_H
#define DSPXMODEL_VIBRATOPOINTDATAARRAY_H

#include <QList>
#include <QObject>
#include <QPointF>
#include <QScopedPointer>
#include <qqmlintegration.h>
#include <vector>

#include <dspxmodelORM/DSPXModelORMGlobal.h>
#include <dspxmodelORM/RangeHelpers.h>

namespace opendspx {
    struct ControlPoint;
}

namespace dspx {

    class Note;

    class VibratoPointDataArrayPrivate;

    /**
     * @brief Vibrato point data array.
     */
    class DSPXMODEL_ORM_EXPORT VibratoPointDataArray : public QObject {
        Q_OBJECT
        QML_ELEMENT
        QML_UNCREATABLE("")
        Q_DECLARE_PRIVATE(VibratoPointDataArray)
        Q_PROPERTY(int size READ size NOTIFY sizeChanged)
        Q_PROPERTY(QList<QPointF> items READ items NOTIFY itemsChanged)
        Q_PROPERTY(VibratoPointRole role READ role CONSTANT)
        Q_PROPERTY(Note *note READ note CONSTANT)
        Q_PRIVATE_PROPERTY(d_func()->jsIterable, QJSValue iterable READ iterable CONSTANT)
    public:
        /**
         * @brief Vibrato point data role.
         */
        enum VibratoPointRole {
            Amplitude,
            Frequency,
        };
        Q_ENUM(VibratoPointRole)

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
         * @brief Converts to OpenDSPX control points.
         */
        std::vector<opendspx::ControlPoint> toOpenDSPX() const;
        /**
         * @brief Converts from OpenDSPX control points.
         * @note Typically, this method SHOULD only be called on a newly created object.
         * @pre note()->model()->document()->transaction() != nullptr && note()->model()->document()->transaction()->state() == dini::TransactionState::Active.
         */
        void fromOpenDSPX(const std::vector<opendspx::ControlPoint> &points);

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
        ~VibratoPointDataArray() override;

        explicit VibratoPointDataArray(Note *note, VibratoPointRole role);

        QScopedPointer<VibratoPointDataArrayPrivate> d_ptr;
    };

}

#endif // DSPXMODEL_VIBRATOPOINTDATAARRAY_H
