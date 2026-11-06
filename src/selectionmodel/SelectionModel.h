#ifndef DSPXMODEL_SELECTIONMODEL_H
#define DSPXMODEL_SELECTIONMODEL_H

#include <QObject>
#include <QScopedPointer>
#include <QString>
#include <qqmlintegration.h>

#include <dspxmodelORM/FreeValueDataArray.h>
#include <dspxmodelSelectionModel/DSPXModelSelectionModelGlobal.h>

namespace dspx {

    class Model;
    class SingingClip;

    class AnchorNodeSelectionModel;
    class ClipSelectionModel;
    class DynamicMixingAnchorSelectionModel;
    class FreeParameterSelectionModel;
    class KeySignatureSelectionModel;
    class LabelSelectionModel;
    class NoteSelectionModel;
    class TempoSelectionModel;
    class TrackSelectionModel;
    class SelectionModelPrivate;

    /**
     * @brief Coordinates item and free parameter range selections.
     *
     * SelectionModel owns one model per selection type. At most one type is
     * active at a time. Changing the type clears the previous selection and
     * releases any active free parameter context.
     *
     * The typed models only accept items that belong to this object's model().
     * currentItem() is a focus item and does not have to be part of the selected
     * item set.
     *
     * @see AnchorNodeSelectionModel, ClipSelectionModel, DynamicMixingAnchorSelectionModel, FreeParameterSelectionModel, KeySignatureSelectionModel, LabelSelectionModel, NoteSelectionModel, TempoSelectionModel, TrackSelectionModel
     */
    class DSPXMODEL_SELECTIONMODEL_EXPORT SelectionModel : public QObject {
        Q_OBJECT
        QML_ELEMENT
        QML_UNCREATABLE("")
        Q_DECLARE_PRIVATE(SelectionModel)
        Q_MOC_INCLUDE(<dspxmodelORM/SingingClip.h>)
        Q_MOC_INCLUDE(<dspxmodelSelectionModel/FreeParameterSelectionModel.h>)
        Q_PROPERTY(Model *model READ model CONSTANT)
        Q_PROPERTY(SelectionType selectionType READ selectionType NOTIFY selectionTypeChanged)
        Q_PROPERTY(AnchorNodeSelectionModel *anchorNodeSelectionModel READ anchorNodeSelectionModel CONSTANT)
        Q_PROPERTY(ClipSelectionModel *clipSelectionModel READ clipSelectionModel CONSTANT)
        Q_PROPERTY(DynamicMixingAnchorSelectionModel *dynamicMixingAnchorSelectionModel READ dynamicMixingAnchorSelectionModel CONSTANT)
        Q_PROPERTY(FreeParameterSelectionModel *freeParameterSelectionModel READ freeParameterSelectionModel CONSTANT)
        Q_PROPERTY(KeySignatureSelectionModel *keySignatureSelectionModel READ keySignatureSelectionModel CONSTANT)
        Q_PROPERTY(LabelSelectionModel *labelSelectionModel READ labelSelectionModel CONSTANT)
        Q_PROPERTY(NoteSelectionModel *noteSelectionModel READ noteSelectionModel CONSTANT)
        Q_PROPERTY(TempoSelectionModel *tempoSelectionModel READ tempoSelectionModel CONSTANT)
        Q_PROPERTY(TrackSelectionModel *trackSelectionModel READ trackSelectionModel CONSTANT)
        Q_PROPERTY(QObject *currentItemSelectionModel READ currentItemSelectionModel NOTIFY currentItemSelectionModelChanged)
        Q_PROPERTY(QObject *currentItem READ currentItem NOTIFY currentItemChanged)
        Q_PROPERTY(int selectedCount READ selectedCount NOTIFY selectedCountChanged)
        Q_PROPERTY(bool hasSelection READ hasSelection NOTIFY hasSelectionChanged)

    public:
        /**
         * @brief Active typed selection model.
         */
        enum SelectionType {
            ST_None,
            ST_AnchorNode,
            ST_Clip,
            ST_Label,
            ST_Note,
            ST_Tempo,
            ST_Track,
            ST_KeySignature,
            ST_DynamicMixingAnchor,
            ST_FreeParameter,
        };
        Q_ENUM(SelectionType)

        explicit SelectionModel(Model *model, QObject *parent = nullptr);
        ~SelectionModel() override;

        /**
         * @brief Gets the model that selected items must belong to.
         * @post model() != nullptr.
         */
        Model *model() const;

        /**
         * @brief Gets the active selection type.
         */
        SelectionType selectionType() const;

        /**
         * @brief Gets typed selection models.
         */
        AnchorNodeSelectionModel *anchorNodeSelectionModel() const;
        ClipSelectionModel *clipSelectionModel() const;
        DynamicMixingAnchorSelectionModel *dynamicMixingAnchorSelectionModel() const;
        FreeParameterSelectionModel *freeParameterSelectionModel() const;
        KeySignatureSelectionModel *keySignatureSelectionModel() const;
        LabelSelectionModel *labelSelectionModel() const;
        NoteSelectionModel *noteSelectionModel() const;
        TempoSelectionModel *tempoSelectionModel() const;
        TrackSelectionModel *trackSelectionModel() const;

        /**
         * @brief Gets the active item selection model, or nullptr.
         */
        QObject *currentItemSelectionModel() const;
        /**
         * @brief Gets the current item in the active item selection model, or nullptr.
         */
        QObject *currentItem() const;
        /**
         * @brief Gets the selected item count. Returns zero for a range selection.
         * @post selectedCount() >= 0.
         */
        int selectedCount() const;
        /// Returns whether the active selection contains items or a nonempty range.
        bool hasSelection() const;
        /**
         * @brief Gets whether item is selected by the active typed model.
         */
        Q_INVOKABLE bool isItemSelected(QObject *item) const;

        /**
         * @brief Selection operation flags.
         *
         * Select adds an item, Deselect removes it, Toggle switches its selected
         * state, ClearPreviousSelection clears the active typed selection first,
         * and SetCurrentItem updates the typed current item. Flags may be
         * combined.
         */
        enum SelectionCommandFlag {
            Select = 0x1,
            Deselect = 0x2,
            Toggle = Select | Deselect,
            ClearPreviousSelection = 0x4,
            SetCurrentItem = 0x8,
        };
        Q_ENUM(SelectionCommandFlag)
        Q_DECLARE_FLAGS(SelectionCommand, SelectionCommandFlag)

        /**
         * @brief Gets the selection type represented by item.
         */
        Q_INVOKABLE static SelectionType selectionTypeFromItem(QObject *item);
        /**
         * @brief Applies a selection command.
         *
         * If item is nullptr, emptySelectionType may select the typed model to
         * operate on; this is useful for clearing a type or setting an empty
         * selection context through containerItemHint. If item is not nullptr and
         * does not map to a supported type, the command is treated as targeting
         * ST_None.
         *
         * For anchor node, dynamic mixing anchor, and note selection,
         * containerItemHint may be the corresponding sequence used as the empty
         * selection context.
         * Invalid hints are ignored.
         * For ST_FreeParameter, ClearPreviousSelection clears the range if the
         * context is active and retains the context.
         */
        Q_INVOKABLE void select(QObject *item, SelectionCommand command, SelectionType emptySelectionType = {}, QObject *containerItemHint = {});

        /**
         * @brief Activates an editable free parameter context.
         * @returns True if the clip belongs to the model's tracks, the ID is
         * nonempty, and the role is Edited or Transform. Returns false for an
         * invalid context. Repeated activation of the same context retains its range.
         */
        Q_INVOKABLE bool setFreeParameterContext(SingingClip *clip, const QString &parameterId, FreeValueDataArray::FreeValueRole role = FreeValueDataArray::Edited);
        /**
         * @brief Sets the range in clip content ticks if a free parameter context is active.
         *
         * Clamps each endpoint to zero or greater and orders the endpoints.
         * Equal endpoints represent an empty range.
         */
        Q_INVOKABLE void setFreeParameterRange(int start, int end);
        /// Clears the active free parameter context and its range.
        Q_INVOKABLE void clearFreeParameterContext();

    Q_SIGNALS:
        void selectionTypeChanged();
        void currentItemSelectionModelChanged();
        void currentItemChanged();
        void selectedCountChanged();
        void hasSelectionChanged();
        void itemSelected(QObject *item, bool selected);

    private:
        void clearItemSelection();
        QScopedPointer<SelectionModelPrivate> d_ptr;
    };

    Q_DECLARE_OPERATORS_FOR_FLAGS(SelectionModel::SelectionCommand)

}

#endif // DSPXMODEL_SELECTIONMODEL_H
