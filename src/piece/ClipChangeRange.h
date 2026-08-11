#ifndef DSPXMODEL_CLIPCHANGERANGE_H
#define DSPXMODEL_CLIPCHANGERANGE_H

#include <QMetaType>
#include <QSharedDataPointer>
#include <qqmlintegration.h>

#include <dspxmodelPiece/DSPXModelPieceGlobal.h>

namespace dspx {

    class ClipChangeRangePrivate;

    /**
     * @brief One clip-relative half-open tick interval affected by a change.
     *
     * The represented interval is `[position(), position() + length())`.
     * Positions are not clipped to the visible clip range. A discrete point at
     * tick x is represented by `[x, x + 1)`.
     */
    class DSPXMODEL_PIECE_EXPORT ClipChangeRange {
        Q_GADGET
        QML_VALUE_TYPE(clipChangeRange)
        Q_PROPERTY(int position READ position CONSTANT)
        Q_PROPERTY(int length READ length CONSTANT)

    public:
        /** @brief Creates an empty interval at tick zero. */
        ClipChangeRange();

        /**
         * @brief Creates an interval.
         * @param position Clip-relative start tick.
         * @param length Interval length in ticks; negative values become zero.
         */
        ClipChangeRange(int position, int length);

        /** @brief Copies an implicitly shared interval. */
        ClipChangeRange(const ClipChangeRange &other);

        /** @brief Moves an implicitly shared interval. */
        ClipChangeRange(ClipChangeRange &&other) noexcept;

        /** @brief Destroys this value wrapper. */
        ~ClipChangeRange();

        /** @brief Copy-assigns an implicitly shared interval. */
        ClipChangeRange &operator=(const ClipChangeRange &other);

        /** @brief Move-assigns an implicitly shared interval. */
        ClipChangeRange &operator=(ClipChangeRange &&other) noexcept;

        /** @brief Gets the clip-relative start tick. */
        int position() const;

        /** @brief Gets the interval length in ticks. */
        int length() const;

        /** @brief Tests whether position and length are equal. */
        bool operator==(const ClipChangeRange &other) const;

        /** @brief Tests whether position or length differs. */
        bool operator!=(const ClipChangeRange &other) const { return !(*this == other); }

    private:
        QSharedDataPointer<ClipChangeRangePrivate> d_ptr;
    };

}

Q_DECLARE_METATYPE(dspx::ClipChangeRange)
Q_DECLARE_TYPEINFO(dspx::ClipChangeRange, Q_RELOCATABLE_TYPE);

#endif // DSPXMODEL_CLIPCHANGERANGE_H
