#ifndef DSPXMODEL_PARAMETER_H
#define DSPXMODEL_PARAMETER_H

#include <dspxmodelORM/EntityObject.h>

namespace dspx {

    class ParameterCurveAnchor;
    class ParameterCurveFree;
    class ParameterMap;

    class ParameterPrivate;

    /**
     * @brief Parameter.
     */
    class DSPXMODEL_ORM_EXPORT Parameter : public EntityObject {
        Q_OBJECT
        Q_PROPERTY(ParameterCurveFree *original READ original CONSTANT)
        Q_PROPERTY(ParameterCurveFree *freeTransform READ freeTransform CONSTANT)
        Q_PROPERTY(ParameterCurveFree *freeEdited READ freeEdited CONSTANT)
        Q_PROPERTY(ParameterCurveAnchor *anchorTransform READ anchorTransform CONSTANT)
        Q_PROPERTY(ParameterCurveAnchor *anchorEdited READ anchorEdited CONSTANT)
        Q_PROPERTY(ParameterMap *parameterMap READ parameterMap NOTIFY parameterMapChanged)
    public:
        ~Parameter() override;

        /**
         * @brief Gets original parameter curve.
         * @post original() != nullptr.
         */
        ParameterCurveFree *original() const;
        /**
         * @brief Gets free transform parameter curve.
         * @post freeTransform() != nullptr.
         */
        ParameterCurveFree *freeTransform() const;
        /**
         * @brief Gets free edited parameter curve.
         * @post freeEdited() != nullptr.
         */
        ParameterCurveFree *freeEdited() const;
        /**
         * @brief Gets anchor transform parameter curve.
         * @post anchorTransform() != nullptr.
         */
        ParameterCurveAnchor *anchorTransform() const;
        /**
         * @brief Gets anchor edited parameter curve.
         * @post anchorEdited() != nullptr.
         */
        ParameterCurveAnchor *anchorEdited() const;

        /**
         * @brief Gets parameter map.
         */
        ParameterMap *parameterMap() const;

    signals:
        void parameterMapChanged(ParameterMap *parameterMap);

    private:
        explicit Parameter(Handle handle, Model *model);

        QScopedPointer<ParameterPrivate> d_ptr;
    };

}

#endif // DSPXMODEL_PARAMETER_H
