#ifndef DSPXMODEL_ANCHORNODE_H
#define DSPXMODEL_ANCHORNODE_H

#include <dspxmodelORM/EntityObject.h>

namespace dspx {

    class AnchorNodeSequence;

    class AnchorNodePrivate;

    /**
     * @brief Anchor node.
     */
    class DSPXMODEL_ORM_EXPORT AnchorNode : public EntityObject {
        Q_OBJECT
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

        ~AnchorNode() override;

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

    signals:
        void interpolationModeChanged(InterpolationMode interpolationMode);
        void xChanged(int x);
        void yChanged(int y);
        void previousItemChanged(AnchorNode *previousItem);
        void nextItemChanged(AnchorNode *nextItem);
        void anchorNodeSequenceChanged(AnchorNodeSequence *anchorNodeSequence);

    private:
        explicit AnchorNode(Handle handle, Model *model);

        QScopedPointer<AnchorNodePrivate> d_ptr;
    };

}

#endif // DSPXMODEL_ANCHORNODE_H
