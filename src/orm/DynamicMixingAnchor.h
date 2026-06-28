#ifndef DSPXMODEL_DYNAMICMIXINGANCHOR_H
#define DSPXMODEL_DYNAMICMIXINGANCHOR_H

#include <QList>

#include <dspxmodelORM/EntityObject.h>

namespace dspx {

    class DynamicMixingAnchorSequence;

    class DynamicMixingAnchorPrivate;

    /**
     * @brief Dynamic mixing anchor.
     */
    class DSPXMODEL_ORM_EXPORT DynamicMixingAnchor : public EntityObject {
        Q_OBJECT
        Q_PROPERTY(int pos READ pos WRITE setPos NOTIFY posChanged)
        Q_PROPERTY(QList<double> ratio READ ratio WRITE setRatio NOTIFY ratioChanged)
        Q_PROPERTY(DynamicMixingAnchor *previousItem READ previousItem NOTIFY previousItemChanged)
        Q_PROPERTY(DynamicMixingAnchor *nextItem READ nextItem NOTIFY nextItemChanged)
        Q_PROPERTY(DynamicMixingAnchorSequence *dynamicMixingAnchorSequence READ dynamicMixingAnchorSequence NOTIFY dynamicMixingAnchorSequenceChanged)
    public:
        ~DynamicMixingAnchor() override;

        /**
         * @brief Gets pos.
         *
         * This property is the position index of the dynamic mixing anchor in the dynamic mixing anchor sequence.
         *
         * @post pos() >= 0.
         */
        int pos() const;
        /**
         * @brief Sets pos.
         * @pre pos >= 0.
         * @post pos() == pos.
         */
        void setPos(int pos);

        /**
         * @brief Gets ratio.
         *
         * The size of ratio() plus 1 should be equal to the size of dynamicMixingAnchorSequence()->sources()->singers().
         * However, this constraint is permitted to be temporarily violated during intermediate states.
         * In this case:
         *   - if the size is bigger, the extra items are ignored;
         *   - if the size is smaller, the missing items are filled with 0.0.
         *
         * @post Each item in ratio() is in the range [0.0, 1.0].
         * @post The sum of items in ratio() is less than or equal to 1.0.
         */
        QList<double> ratio() const;
        /**
         * @brief Sets ratio.
         * @pre Each item in ratio is in the range [0.0, 1.0].
         * @pre The sum of items in ratio is less than or equal to 1.0.
         * @post ratio() == ratio.
         */
        void setRatio(const QList<double> &ratio);

        /**
         * @brief Gets previous item.
         */
        DynamicMixingAnchor *previousItem() const;
        /**
         * @brief Gets next item.
         */
        DynamicMixingAnchor *nextItem() const;

        /**
         * @brief Gets dynamic mixing anchor sequence.
         */
        DynamicMixingAnchorSequence *dynamicMixingAnchorSequence() const;

    signals:
        void posChanged(int pos);
        void ratioChanged(const QList<double> &ratio);
        void previousItemChanged(DynamicMixingAnchor *previousItem);
        void nextItemChanged(DynamicMixingAnchor *nextItem);
        void dynamicMixingAnchorSequenceChanged(DynamicMixingAnchorSequence *dynamicMixingAnchorSequence);

    private:
        explicit DynamicMixingAnchor(Handle handle, Model *model);

        QScopedPointer<DynamicMixingAnchorPrivate> d_ptr;
    };

}

#endif // DSPXMODEL_DYNAMICMIXINGANCHOR_H
