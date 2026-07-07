#ifndef DSPXMODEL_TEMPOPROPERTYMAPPER_P_H
#define DSPXMODEL_TEMPOPROPERTYMAPPER_P_H

#include "TempoPropertyMapper.h"

#include <dspxmodelORM/Tempo.h>

#include "PropertyMapperData_p.h"

namespace dspx {
    class TempoSelectionModel;
    class TempoPropertyMapperPrivate : public PropertyMapperData<
        TempoPropertyMapper,
        TempoPropertyMapperPrivate,
        dspx::Tempo,
        PropertyMetadata<dspx::Tempo, &dspx::Tempo::position, &dspx::Tempo::setPosition, decltype(&dspx::Tempo::positionChanged)>,
        PropertyMetadata<dspx::Tempo, &dspx::Tempo::value, &dspx::Tempo::setValue, decltype(&dspx::Tempo::valueChanged)>
    > {
        Q_DECLARE_PUBLIC(TempoPropertyMapper)
    public:
        TempoPropertyMapperPrivate() : PropertyMapperData(
            {&dspx::Tempo::positionChanged},
            {&dspx::Tempo::valueChanged}
        ) {}

        dspx::SelectionModel *selectionModel = nullptr;
        dspx::TempoSelectionModel *tempoSelectionModel = nullptr;

        void setSelectionModel(dspx::SelectionModel *selectionModel_);
        void attachSelectionModel();
        void detachSelectionModel();

        enum {
            PositionProperty = 0,
            ValueProperty = 1
        };

        template<int i>
        void notifyValueChange() {
            Q_Q(TempoPropertyMapper);
            if constexpr (i == PositionProperty) {
                q->positionChanged();
            } else if constexpr (i == ValueProperty) {
                q->valueChanged();
            }
        }
    };
}

#endif // DSPXMODEL_TEMPOPROPERTYMAPPER_P_H
