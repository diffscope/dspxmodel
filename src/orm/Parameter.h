#ifndef DSPXMODEL_PARAMETER_H
#define DSPXMODEL_PARAMETER_H

#include <qqmlintegration.h>

#include <dspxmodelORM/EntityObject.h>

namespace opendspx {
    struct Param;
}

namespace dspx {

    class AnchorNodeSequence;
    class FreeValueDataArray;
    class ParameterMap;

    class ParameterPrivate;

    /**
     * @brief Parameter.
     */
    class DSPXMODEL_ORM_EXPORT Parameter : public EntityObject {
        Q_OBJECT
        QML_ELEMENT
        QML_UNCREATABLE("")
        Q_DECLARE_PRIVATE(Parameter)
        Q_PROPERTY(FreeValueDataArray *original READ original CONSTANT)
        Q_PROPERTY(FreeValueDataArray *freeTransform READ freeTransform CONSTANT)
        Q_PROPERTY(FreeValueDataArray *freeEdited READ freeEdited CONSTANT)
        Q_PROPERTY(AnchorNodeSequence *anchorTransform READ anchorTransform CONSTANT)
        Q_PROPERTY(AnchorNodeSequence *anchorEdited READ anchorEdited CONSTANT)
        Q_PROPERTY(ParameterMap *parameterMap READ parameterMap NOTIFY parameterMapChanged)
    public:
        /**
         * @brief Gets original free value data array.
         * @post original() != nullptr.
         */
        FreeValueDataArray *original() const;
        /**
         * @brief Gets free transform value data array.
         * @post freeTransform() != nullptr.
         */
        FreeValueDataArray *freeTransform() const;
        /**
         * @brief Gets free edited value data array.
         * @post freeEdited() != nullptr.
         */
        FreeValueDataArray *freeEdited() const;
        /**
         * @brief Gets anchor transform node sequence.
         * @post anchorTransform() != nullptr.
         */
        AnchorNodeSequence *anchorTransform() const;
        /**
         * @brief Gets anchor edited node sequence.
         * @post anchorEdited() != nullptr.
         */
        AnchorNodeSequence *anchorEdited() const;

        /**
         * @brief Gets parameter map.
         */
        ParameterMap *parameterMap() const;

        /**
         * @brief Converts to OpenDSPX parameter.
         */
        opendspx::Param toOpenDSPX() const;
        /**
         * @brief Converts from OpenDSPX parameter.
         * @note Typically, this method SHOULD only be called on a newly created object.
         * @pre model()->document()->transaction() != nullptr && model()->document()->transaction()->state() == dini::TransactionState::Active.
         */
        void fromOpenDSPX(const opendspx::Param &parameter);

    signals:
        void parameterMapChanged(ParameterMap *parameterMap);
        void parameterMapChangedAfterCommit(ParameterMap *parameterMap);

    private:
        ~Parameter() override;

        explicit Parameter(Handle handle, Model *model);

        QScopedPointer<ParameterPrivate> d_ptr;
    };

}

#endif // DSPXMODEL_PARAMETER_H
