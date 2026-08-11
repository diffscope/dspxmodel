#ifndef DSPXMODEL_CLIPCHANGE_H
#define DSPXMODEL_CLIPCHANGE_H

#include <qqmlintegration.h>

#include <QFlags>
#include <QList>
#include <QMetaType>
#include <QSharedDataPointer>
#include <QStringList>

#include <dspxmodelPiece/ClipChangeRange.h>
#include <dspxmodelPiece/DSPXModelPieceGlobal.h>

namespace dspx {

    class ClipChangePrivate;
    class ClipWatcherPrivate;

    /**
     * @brief Aggregated net changes observed in one SingingClip.
     *
     * Instances are produced by ClipWatcher::takeChanges(). Categories describe
     * score, lyric, pronunciation, note, phoneme, vibrato and parameter changes,
     * while ranges() reports the normalized union of every affected old and new
     * clip-relative tick interval. Parameter
     * changes additionally contain sorted, unique edited/transform parameter
     * names.
     */
    class DSPXMODEL_PIECE_EXPORT ClipChange {
        Q_GADGET
        QML_VALUE_TYPE(clipChange)
        Q_PROPERTY(ChangeTypes types READ types CONSTANT)
        Q_PROPERTY(QStringList parameterNames READ parameterNames CONSTANT)
        Q_PROPERTY(QList<ClipChangeRange> ranges READ ranges CONSTANT)

    public:
        /** @brief A category of clip content change. */
        enum ChangeType {
            Score = 0x01, ///< A note entered or left the clip.
            Lyric = 0x02, ///< A note lyric or language changed.
            Pronunciation = 0x04, ///< An edited pronunciation changed.
            Note = 0x08, ///< Pitch or absolute note timing changed.
            Phoneme = 0x10, ///< Edited phoneme content changed.
            Vibrato = 0x20, ///< Vibrato properties or points changed.
            Parameter = 0x40, ///< Edited/transform parameter output changed.
            Sources = 0x80, ///< Architecture, singers, extras or mixing changed.
            ClipTiming = 0x100, ///< Clip placement or visible/content range changed.
        };
        Q_ENUM(ChangeType)
        Q_DECLARE_FLAGS(ChangeTypes, ChangeType)
        Q_FLAG(ChangeTypes)

        /** @brief Creates an empty change set. */
        ClipChange();
        /** @brief Copies an implicitly shared change set. */
        ClipChange(const ClipChange &other);

        /** @brief Moves an implicitly shared change set. */
        ClipChange(ClipChange &&other) noexcept;

        /** @brief Destroys this value wrapper. */
        ~ClipChange();

        /** @brief Copy-assigns an implicitly shared change set. */
        ClipChange &operator=(const ClipChange &other);

        /** @brief Move-assigns an implicitly shared change set. */
        ClipChange &operator=(ClipChange &&other) noexcept;

        /** @brief Gets every contained change category. */
        ChangeTypes types() const;

        /** @brief Tests whether a category is present. */
        Q_INVOKABLE bool contains(ChangeType type) const;

        /**
         * @brief Tests whether no change category is present.
         *
         * This depends on types(), not ranges(). An empty parameter being added,
         * removed or renamed can therefore be non-empty while ranges() is empty.
         */
        Q_INVOKABLE bool isEmpty() const;

        /**
         * @brief Gets sorted, unique affected edited/transform parameter names.
         *
         * Renaming contributes both the old and new names.
         */
        QStringList parameterNames() const;

        /**
         * @brief Gets the normalized union of affected half-open tick intervals.
         *
         * Empty intervals are removed; overlapping or touching intervals are
         * merged and the result is sorted by position. Coordinates are relative
         * to the clip and are not clipped to its visible length.
         */
        QList<ClipChangeRange> ranges() const;

    private:
        explicit ClipChange(ChangeTypes types, QStringList parameterNames, QList<ClipChangeRange> ranges);

        QSharedDataPointer<ClipChangePrivate> d_ptr;

        friend class ClipWatcherPrivate;
    };

    Q_DECLARE_OPERATORS_FOR_FLAGS(ClipChange::ChangeTypes)

}

Q_DECLARE_METATYPE(dspx::ClipChange)
Q_DECLARE_TYPEINFO(dspx::ClipChange, Q_RELOCATABLE_TYPE);

#endif // DSPXMODEL_CLIPCHANGE_H
