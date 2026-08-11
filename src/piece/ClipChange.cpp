#include "ClipChange.h"

#include "ClipChange_p.h"
#include "ClipChangeUtils_p.h"

#include <algorithm>
#include <utility>

namespace dspx {

    ClipChange::ClipChange()
        : d_ptr(new ClipChangePrivate) {
    }

    ClipChange::ClipChange(const ClipChange &other) = default;
    ClipChange::ClipChange(ClipChange &&other) noexcept = default;
    ClipChange::~ClipChange() = default;
    ClipChange &ClipChange::operator=(const ClipChange &other) = default;
    ClipChange &ClipChange::operator=(ClipChange &&other) noexcept = default;

    ClipChange::ClipChange(ChangeTypes types,
                           QStringList parameterNames,
                           QList<ClipChangeRange> ranges)
        : d_ptr(new ClipChangePrivate) {
        d_ptr->types = types;
        if (types.testFlag(Parameter)) {
            std::sort(parameterNames.begin(), parameterNames.end());
            parameterNames.removeDuplicates();
            d_ptr->parameterNames = std::move(parameterNames);
        }

        d_ptr->ranges = normalizeClipChangeRanges(std::move(ranges));
    }

    ClipChange::ChangeTypes ClipChange::types() const {
        return d_ptr ? d_ptr->types : ChangeTypes {};
    }

    bool ClipChange::contains(ChangeType type) const {
        return types().testFlag(type);
    }

    bool ClipChange::isEmpty() const {
        return !types();
    }

    QStringList ClipChange::parameterNames() const {
        return d_ptr ? d_ptr->parameterNames : QStringList {};
    }

    QList<ClipChangeRange> ClipChange::ranges() const {
        return d_ptr ? d_ptr->ranges : QList<ClipChangeRange> {};
    }

}
