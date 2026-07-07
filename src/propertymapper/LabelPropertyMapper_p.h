#ifndef DSPXMODEL_LABELPROPERTYMAPPER_P_H
#define DSPXMODEL_LABELPROPERTYMAPPER_P_H

#include "LabelPropertyMapper.h"

#include <dspxmodelORM/Label.h>

#include "PropertyMapperData_p.h"

namespace dspx {
    class LabelSelectionModel;
    class LabelPropertyMapperPrivate : public PropertyMapperData<
        LabelPropertyMapper,
        LabelPropertyMapperPrivate,
        dspx::Label,
        PropertyMetadata<dspx::Label, &dspx::Label::position, &dspx::Label::setPosition, decltype(&dspx::Label::positionChanged)>,
        PropertyMetadata<dspx::Label, &dspx::Label::text, &dspx::Label::setText, decltype(&dspx::Label::textChanged)>
    > {
        Q_DECLARE_PUBLIC(LabelPropertyMapper)
    public:
        LabelPropertyMapperPrivate() : PropertyMapperData(
            {&dspx::Label::positionChanged},
            {&dspx::Label::textChanged}
        ) {}

        dspx::SelectionModel *selectionModel = nullptr;
        dspx::LabelSelectionModel *labelSelectionModel = nullptr;

        void setSelectionModel(dspx::SelectionModel *selectionModel_);
        void attachSelectionModel();
        void detachSelectionModel();

        enum {
            PositionProperty = 0,
            TextProperty = 1
        };

        template<int i>
        void notifyValueChange() {
            Q_Q(LabelPropertyMapper);
            if constexpr (i == PositionProperty) {
                q->positionChanged();
            } else if constexpr (i == TextProperty) {
                q->textChanged();
            }
        }
    };
}

#endif // DSPXMODEL_LABELPROPERTYMAPPER_P_H
