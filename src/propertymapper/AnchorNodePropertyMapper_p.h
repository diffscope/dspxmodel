#ifndef DSPXMODEL_ANCHORNODEPROPERTYMAPPER_P_H
#define DSPXMODEL_ANCHORNODEPROPERTYMAPPER_P_H

#include <dspxmodelPropertyMapper/AnchorNodePropertyMapper.h>

#include <dspxmodelORM/AnchorNode.h>

#include <PropertyMapperData_p.h>

namespace dspx {
    class AnchorNodeSelectionModel;

    class AnchorNodePropertyMapperPrivate : public PropertyMapperData<
        AnchorNodePropertyMapper,
        AnchorNodePropertyMapperPrivate,
        dspx::AnchorNode,
        PropertyMetadata<dspx::AnchorNode, &dspx::AnchorNode::x, nullptr, decltype(&dspx::AnchorNode::xChanged)>,
        PropertyMetadata<dspx::AnchorNode, &dspx::AnchorNode::y, &dspx::AnchorNode::setY, decltype(&dspx::AnchorNode::yChanged)>,
        PropertyMetadata<dspx::AnchorNode, &dspx::AnchorNode::interpolationMode, &dspx::AnchorNode::setInterpolationMode, decltype(&dspx::AnchorNode::interpolationModeChanged)>
    > {
        Q_DECLARE_PUBLIC(AnchorNodePropertyMapper)

    public:
        AnchorNodePropertyMapperPrivate() : PropertyMapperData(
            {&dspx::AnchorNode::xChanged},
            {&dspx::AnchorNode::yChanged},
            {&dspx::AnchorNode::interpolationModeChanged}
        ) {
        }

        enum {
            PositionProperty,
            ValueProperty,
            InterpolationModeProperty,
        };

        SelectionModel *selectionModel{};
        AnchorNodeSelectionModel *anchorNodeSelectionModel{};

        void setSelectionModel(SelectionModel *selectionModel);
        void attachSelectionModel();
        void detachSelectionModel();

        template<int i>
        void notifyValueChange() {
            Q_Q(AnchorNodePropertyMapper);
            if constexpr (i == PositionProperty) {
                Q_EMIT q->positionChanged();
            } else if constexpr (i == ValueProperty) {
                Q_EMIT q->valueChanged();
            } else if constexpr (i == InterpolationModeProperty) {
                Q_EMIT q->interpolationModeChanged();
            }
        }
    };
}

#endif // DSPXMODEL_ANCHORNODEPROPERTYMAPPER_P_H
