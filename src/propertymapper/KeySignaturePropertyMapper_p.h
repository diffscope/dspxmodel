#ifndef DSPXMODEL_KEYSIGNATUREPROPERTYMAPPER_P_H
#define DSPXMODEL_KEYSIGNATUREPROPERTYMAPPER_P_H

#include "KeySignaturePropertyMapper.h"

#include <dspxmodelORM/KeySignature.h>

#include "PropertyMapperData_p.h"

namespace dspx {
    class KeySignatureSelectionModel;
    class KeySignaturePropertyMapperPrivate : public PropertyMapperData<
        KeySignaturePropertyMapper,
        KeySignaturePropertyMapperPrivate,
        dspx::KeySignature,
        PropertyMetadata<dspx::KeySignature, &dspx::KeySignature::position, &dspx::KeySignature::setPosition, decltype(&dspx::KeySignature::positionChanged)>,
        PropertyMetadata<dspx::KeySignature, &dspx::KeySignature::mode, &dspx::KeySignature::setMode, decltype(&dspx::KeySignature::modeChanged)>,
        PropertyMetadata<dspx::KeySignature, &dspx::KeySignature::tonality, &dspx::KeySignature::setTonality, decltype(&dspx::KeySignature::tonalityChanged)>,
        PropertyMetadata<dspx::KeySignature, &dspx::KeySignature::accidentalType, &dspx::KeySignature::setAccidentalType, decltype(&dspx::KeySignature::accidentalTypeChanged)>
    > {
        Q_DECLARE_PUBLIC(KeySignaturePropertyMapper)
    public:
        KeySignaturePropertyMapperPrivate() : PropertyMapperData(
            {&dspx::KeySignature::positionChanged},
            {&dspx::KeySignature::modeChanged},
            {&dspx::KeySignature::tonalityChanged},
            {&dspx::KeySignature::accidentalTypeChanged}
        ) {}

        dspx::SelectionModel *selectionModel = nullptr;
        dspx::KeySignatureSelectionModel *keySignatureSelectionModel = nullptr;

        void setSelectionModel(dspx::SelectionModel *selectionModel_);
        void attachSelectionModel();
        void detachSelectionModel();

        enum {
            PositionProperty = 0,
            ModeProperty = 1,
            TonalityProperty = 2,
            AccidentalTypeProperty = 3
        };

        template<int i>
        void notifyValueChange() {
            Q_Q(KeySignaturePropertyMapper);
            if constexpr (i == PositionProperty) {
                q->positionChanged();
            } else if constexpr (i == ModeProperty) {
                q->modeChanged();
            } else if constexpr (i == TonalityProperty) {
                q->tonalityChanged();
            } else if constexpr (i == AccidentalTypeProperty) {
                q->accidentalTypeChanged();
            }
        }
    };
}

#endif // DSPXMODEL_KEYSIGNATUREPROPERTYMAPPER_P_H
