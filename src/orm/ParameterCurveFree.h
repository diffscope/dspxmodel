#ifndef DSPXMODEL_PARAMETERCURVEFREE_H
#define DSPXMODEL_PARAMETERCURVEFREE_H

#include <dspxmodelORM/EntityObject.h>

namespace dspx {

    class FreeValueDataArray;
    class Parameter;

    class ParameterCurveFreePrivate;

    /**
     * @brief Free parameter curve.
     */
    class DSPXMODEL_ORM_EXPORT ParameterCurveFree : public QObject {
        Q_OBJECT
        Q_PROPERTY(ParameterCurveRole role READ role CONSTANT)
        Q_PROPERTY(int step READ step CONSTANT)
        Q_PROPERTY(FreeValueDataArray *values READ values CONSTANT)
        Q_PROPERTY(Parameter *parameter READ parameter CONSTANT)
    public:
        /**
         * @brief Parameter curve role.
         */
        enum ParameterCurveRole {
            Original,
            Transform,
            Edited,
        };
        Q_ENUM(ParameterCurveRole)

        ~ParameterCurveFree() override;

        /**
         * @brief Gets role.
         */
        ParameterCurveRole role() const;

        /**
         * @brief Gets step.
         * @post step() == 5.
         */
        int step() const;

        /**
         * @brief Gets free value data array.
         * @post values() != nullptr.
         */
        FreeValueDataArray *values() const;

        /**
         * @brief Gets parameter.
         * @post parameter() != nullptr.
         */
        Parameter *parameter() const;

    private:
        explicit ParameterCurveFree(Parameter *parameter, ParameterCurveRole role);

        QScopedPointer<ParameterCurveFreePrivate> d_ptr;
    };

}

#endif // DSPXMODEL_PARAMETERCURVEFREE_H
