#ifndef DSPXMODEL_FREEVALUEDATAARRAY_H
#define DSPXMODEL_FREEVALUEDATAARRAY_H

#include <QList>
#include <QObject>
#include <QVariant>

#include <dspxmodelORM/DSPXModelORMGlobal.h>

namespace dspx {

    class ParameterCurveFree;

    class FreeValueDataArrayPrivate;

    /**
     * @brief Free value data array.
     */
    class DSPXMODEL_ORM_EXPORT FreeValueDataArray : public QObject {
        Q_OBJECT
        Q_PROPERTY(int size READ size NOTIFY sizeChanged)
        Q_PROPERTY(QList<QVariant> items READ items NOTIFY itemsChanged)
        Q_PROPERTY(ParameterCurveFree *parameterCurve READ parameterCurve CONSTANT)
    public:
        ~FreeValueDataArray() override;

        /**
         * @brief Gets size.
         * @post size() >= 0.
         */
        int size() const;
        /**
         * @brief Gets items.
         * @post items()[i] is either QVariant::Invalid or QVariant::Int.
         */
        QList<QVariant> items() const;

        /**
         * @brief Gets slice.
         * @pre index >= 0.
         * @pre length >= 0.
         * @post slice(index, length)[i] is either QVariant::Invalid or QVariant::Int.
         * @returns Items in the range [index, index + length).
         */
        Q_INVOKABLE QList<QVariant> slice(int index, int length) const;
        /**
         * @brief Splices items.
         * @pre index >= 0.
         * @pre length >= 0.
         * @pre values[i] is either QVariant::Invalid or QVariant::Int.
         * @post If successful, items are spliced.
         */
        Q_INVOKABLE bool splice(int index, int length, const QList<QVariant> &values);
        /**
         * @brief Rotates items.
         * @pre leftIndex >= 0.
         * @pre middleIndex >= leftIndex.
         * @pre rightIndex >= middleIndex.
         * @post If successful, items are rotated.
         */
        Q_INVOKABLE bool rotate(int leftIndex, int middleIndex, int rightIndex);

        /**
         * @brief Gets parameter curve.
         * @post parameterCurve() != nullptr.
         */
        ParameterCurveFree *parameterCurve() const;

    signals:
        void sizeChanged(int size);
        void itemsChanged();
        void aboutToSplice(int index, int length, const QList<QVariant> &values);
        void spliced(int index, int length, const QList<QVariant> &values);
        void aboutToRotate(int leftIndex, int middleIndex, int rightIndex);
        void rotated(int leftIndex, int middleIndex, int rightIndex);

    private:
        explicit FreeValueDataArray(ParameterCurveFree *parameterCurve);

        QScopedPointer<FreeValueDataArrayPrivate> d_ptr;
    };

}

#endif // DSPXMODEL_FREEVALUEDATAARRAY_H
