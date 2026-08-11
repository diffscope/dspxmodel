#include "ClipChangeRange.h"

#include "ClipChangeRange_p.h"

#include <algorithm>

namespace dspx {

    ClipChangeRange::ClipChangeRange()
        : d_ptr(new ClipChangeRangePrivate) {
    }

    ClipChangeRange::ClipChangeRange(int position, int length)
        : d_ptr(new ClipChangeRangePrivate) {
        d_ptr->position = position;
        d_ptr->length = std::max(0, length);
    }

    ClipChangeRange::ClipChangeRange(const ClipChangeRange &other) = default;
    ClipChangeRange::ClipChangeRange(ClipChangeRange &&other) noexcept = default;
    ClipChangeRange::~ClipChangeRange() = default;
    ClipChangeRange &ClipChangeRange::operator=(const ClipChangeRange &other) = default;
    ClipChangeRange &ClipChangeRange::operator=(ClipChangeRange &&other) noexcept = default;

    int ClipChangeRange::position() const {
        return d_ptr ? d_ptr->position : 0;
    }

    int ClipChangeRange::length() const {
        return d_ptr ? d_ptr->length : 0;
    }

    bool ClipChangeRange::operator==(const ClipChangeRange &other) const {
        return position() == other.position() && length() == other.length();
    }

}
