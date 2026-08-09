#include "PieceChange.h"

#include "PieceChange_p.h"

#include <algorithm>

namespace dspx {

    PieceChange::PieceChange()
        : d_ptr(new PieceChangePrivate) {
    }

    PieceChange::PieceChange(const PieceChange &other) = default;

    PieceChange::PieceChange(PieceChange &&other) noexcept = default;

    PieceChange::~PieceChange() = default;

    PieceChange &PieceChange::operator=(const PieceChange &other) = default;

    PieceChange &PieceChange::operator=(PieceChange &&other) noexcept = default;

    PieceChange::PieceChange(ChangeTypes types, QStringList parameterNames)
        : d_ptr(new PieceChangePrivate) {
        d_ptr->types = types;
        if (types.testFlag(Parameter)) {
            std::sort(parameterNames.begin(), parameterNames.end());
            parameterNames.removeDuplicates();
            d_ptr->parameterNames = std::move(parameterNames);
        }
    }

    PieceChange::ChangeTypes PieceChange::types() const {
        return d_ptr ? d_ptr->types : ChangeTypes {};
    }

    bool PieceChange::contains(ChangeType type) const {
        return types().testFlag(type);
    }

    bool PieceChange::isEmpty() const {
        return !types();
    }

    QStringList PieceChange::parameterNames() const {
        return d_ptr ? d_ptr->parameterNames : QStringList {};
    }

}
