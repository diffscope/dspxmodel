#include "ClipChangeUtils_p.h"

#include <algorithm>
#include <utility>

namespace dspx {

    QList<ClipChangeRange> normalizeClipChangeRanges(QList<ClipChangeRange> ranges) {
        ranges.erase(std::remove_if(ranges.begin(), ranges.end(), [](const ClipChangeRange &range) {
            return range.length() <= 0;
        }), ranges.end());
        std::sort(ranges.begin(), ranges.end(), [](const ClipChangeRange &left,
                                                   const ClipChangeRange &right) {
            return left.position() < right.position() ||
                   (left.position() == right.position() && left.length() < right.length());
        });

        QList<ClipChangeRange> result;
        for (const auto &range : std::as_const(ranges)) {
            if (result.isEmpty()) {
                result.append(range);
                continue;
            }
            const auto &last = result.constLast();
            const qint64 lastEnd = static_cast<qint64>(last.position()) + last.length();
            const qint64 rangeEnd = static_cast<qint64>(range.position()) + range.length();
            if (range.position() > lastEnd) {
                result.append(range);
                continue;
            }
            const qint64 mergedEnd = std::max(lastEnd, rangeEnd);
            result.last() = ClipChangeRange(last.position(),
                                            static_cast<int>(mergedEnd - last.position()));
        }
        return result;
    }

}
