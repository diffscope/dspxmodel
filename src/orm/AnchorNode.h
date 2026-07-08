#ifndef DSPXMODEL_ANCHORNODE_H
#define DSPXMODEL_ANCHORNODE_H

#include <qqmlintegration.h>

#include <dspxmodelORM/EntityObject.h>

namespace opendspx {
    struct AnchorNode;
}

namespace dspx {

    class AnchorNodeSequence;

    class AnchorNodePrivate;

    /**
     * @brief Anchor node.
     */
    class DSPXMODEL_ORM_EXPORT AnchorNode : public EntityObject {
        Q_OBJECT
        QML_ELEMENT
        QML_UNCREATABLE("")
        Q_DECLARE_PRIVATE(AnchorNode)
        Q_PROPERTY(InterpolationMode interpolationMode READ interpolationMode WRITE setInterpolationMode NOTIFY interpolationModeChanged)
        Q_PROPERTY(int x READ x WRITE setX NOTIFY xChanged)
        Q_PROPERTY(int y READ y WRITE setY NOTIFY yChanged)
        Q_PROPERTY(AnchorNode *previousItem READ previousItem NOTIFY previousItemChanged)
        Q_PROPERTY(AnchorNode *nextItem READ nextItem NOTIFY nextItemChanged)
        Q_PROPERTY(AnchorNodeSequence *anchorNodeSequence READ anchorNodeSequence NOTIFY anchorNodeSequenceChanged)
    public:
        /**
         * @brief Interpolation mode.
         */
        enum InterpolationMode {
            None,
            Linear,
            Hermite,
        };
        Q_ENUM(InterpolationMode)

        /**
         * @brief Gets interpolation mode.
         */
        InterpolationMode interpolationMode() const;
        /**
         * @brief Sets interpolation mode.
         * @post interpolationMode() == interpolationMode.
         */
        void setInterpolationMode(InterpolationMode interpolationMode);

        /**
         * @brief Gets x.
         * @post x() >= 0.
         */
        int x() const;
        /**
         * @brief Sets x.
         * @pre x >= 0.
         * @pre If anchorNodeSequence() != nullptr, no other item in anchorNodeSequence() has x.
         * @post x() == x.
         */
        void setX(int x);

        /**
         * @brief Gets y.
         */
        int y() const;
        /**
         * @brief Sets y.
         * @post y() == y.
         */
        void setY(int y);

        /**
         * @brief Gets previous item.
         */
        AnchorNode *previousItem() const;
        /**
         * @brief Gets next item.
         */
        AnchorNode *nextItem() const;

        /**
         * @brief Gets anchor node sequence.
         */
        AnchorNodeSequence *anchorNodeSequence() const;

        /**
         * @brief Converts to OpenDSPX anchor node.
         */
        opendspx::AnchorNode toOpenDSPX() const;
        /**
         * @brief Converts from OpenDSPX anchor node.
         * @note Typically, this method SHOULD only be called on a newly created object.
         * @pre model()->document()->transaction() != nullptr && model()->document()->transaction()->state() == dini::TransactionState::Active.
         */
        void fromOpenDSPX(const opendspx::AnchorNode &node);

    signals:
        void interpolationModeChanged(InterpolationMode interpolationMode);
        void xChanged(int x);
        void yChanged(int y);
        void previousItemChanged(AnchorNode *previousItem);
        void nextItemChanged(AnchorNode *nextItem);
        void anchorNodeSequenceChanged(AnchorNodeSequence *anchorNodeSequence);

    private:
        ~AnchorNode() override;

        explicit AnchorNode(Handle handle, Model *model);

        QScopedPointer<AnchorNodePrivate> d_ptr;
    };

}

#endif // DSPXMODEL_ANCHORNODE_H
