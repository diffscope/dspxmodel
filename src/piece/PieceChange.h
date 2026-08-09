#ifndef DSPXMODEL_PIECECHANGE_H
#define DSPXMODEL_PIECECHANGE_H

#include <QFlags>
#include <QSharedDataPointer>
#include <QStringList>
#include <QMetaType>
#include <qqmlintegration.h>

#include <dspxmodelPiece/DSPXModelPieceGlobal.h>

namespace dspx {

    class PieceChangePrivate;
    class PieceDividerPrivate;

    /**
     * @brief Aggregated content changes affecting one Piece in one committed transaction.
     *
     * A PieceChange contains a set of coarse-grained change types. Changes are
     * collected while ORM updates are applied, coalesced at the dini transaction
     * boundary, and delivered only after the PieceDivider has rebuilt its pieces.
     * Parameter changes additionally carry the sorted, unique ParameterMap keys
     * whose edited or transform values may have changed in the piece interval.
     *
     * PieceChange is an implicitly shared value type. It is read-only to library
     * users; instances are produced by PieceDivider and delivered by
     * Piece::updated().
     */
    class DSPXMODEL_PIECE_EXPORT PieceChange {
        Q_GADGET
        QML_VALUE_TYPE(pieceChange)
        Q_PROPERTY(ChangeTypes types READ types CONSTANT)
        Q_PROPERTY(QStringList parameterNames READ parameterNames CONSTANT)

    public:
        /**
         * @brief A category of committed content change.
         */
        enum ChangeType {
            Score = 0x01,         ///< A note entered or left the target clip.
            Lyric = 0x02,         ///< A lyric or language changed.
            Pronunciation = 0x04, ///< An edited pronunciation changed.
            Note = 0x08,          ///< Pitch or absolute note timing changed.
            Phoneme = 0x10,       ///< Edited phoneme content changed.
            Vibrato = 0x20,       ///< Vibrato properties or control points changed.
            Parameter = 0x40,     ///< Edited/transform parameter values may have changed.
        };
        Q_ENUM(ChangeType)
        Q_DECLARE_FLAGS(ChangeTypes, ChangeType)
        Q_FLAG(ChangeTypes)

        /**
         * @brief Creates an empty change set.
         * @post isEmpty() is true.
         */
        PieceChange();

        /**
         * @brief Copies an implicitly shared change set.
         */
        PieceChange(const PieceChange &other);

        /**
         * @brief Moves a change set.
         */
        PieceChange(PieceChange &&other) noexcept;

        /**
         * @brief Destroys this value wrapper.
         */
        ~PieceChange();

        /**
         * @brief Copy-assigns an implicitly shared change set.
         */
        PieceChange &operator=(const PieceChange &other);

        /**
         * @brief Move-assigns a change set.
         */
        PieceChange &operator=(PieceChange &&other) noexcept;

        /**
         * @brief Gets all change categories contained in this value.
         */
        ChangeTypes types() const;

        /**
         * @brief Tests whether a change category is present.
         * @param type Category to test.
         */
        Q_INVOKABLE bool contains(ChangeType type) const;

        /**
         * @brief Tests whether no change category is present.
         */
        Q_INVOKABLE bool isEmpty() const;

        /**
         * @brief Gets sorted, unique names of parameters affected in this piece.
         *
         * The list is empty when Parameter is absent. Parameter renames may
         * contribute both the old and new names.
         */
        QStringList parameterNames() const;

    private:
        explicit PieceChange(ChangeTypes types, QStringList parameterNames);

        QSharedDataPointer<PieceChangePrivate> d_ptr;

        friend class PieceDividerPrivate;
    };

    Q_DECLARE_OPERATORS_FOR_FLAGS(PieceChange::ChangeTypes)

}

Q_DECLARE_METATYPE(dspx::PieceChange)
Q_DECLARE_TYPEINFO(dspx::PieceChange, Q_RELOCATABLE_TYPE);

#endif // DSPXMODEL_PIECECHANGE_H
