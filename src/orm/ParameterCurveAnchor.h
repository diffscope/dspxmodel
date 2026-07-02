#ifndef DSPXMODEL_PARAMETERCURVEANCHOR_H
#define DSPXMODEL_PARAMETERCURVEANCHOR_H

#include <dspxmodelORM/EntityObject.h>

namespace dspx {

    class AnchorNodeSequence;
    class Parameter;

    class ParameterCurveAnchorPrivate;

    /**
     * @brief Anchor parameter curve.
     */
    class DSPXMODEL_ORM_EXPORT ParameterCurveAnchor : public QObject {
        Q_OBJECT
        Q_PROPERTY(ParameterCurveRole role READ role CONSTANT)
        Q_PROPERTY(AnchorNodeSequence *nodes READ nodes CONSTANT)
        Q_PROPERTY(Parameter *parameter READ parameter CONSTANT)
    public:
        /**
         * @brief Parameter curve role.
         */
        enum ParameterCurveRole {
            Transform,
            Edited,
        };
        Q_ENUM(ParameterCurveRole)

        ~ParameterCurveAnchor() override;

        /**
         * @brief Gets role.
         */
        ParameterCurveRole role() const;

        /**
         * @brief Gets anchor node sequence.
         * @post nodes() != nullptr.
         */
        AnchorNodeSequence *nodes() const;

        /**
         * @brief Gets parameter.
         * @post parameter() != nullptr.
         */
        Parameter *parameter() const;

    private:
        explicit ParameterCurveAnchor(Parameter *parameter, ParameterCurveRole role);

        QScopedPointer<ParameterCurveAnchorPrivate> d_ptr;
    };

}

#endif // DSPXMODEL_PARAMETERCURVEANCHOR_H
