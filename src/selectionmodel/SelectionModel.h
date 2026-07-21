#ifndef DSPXMODEL_SELECTIONMODEL_H
#define DSPXMODEL_SELECTIONMODEL_H

#include <QObject>
#include <QScopedPointer>
#include <qqmlintegration.h>

#include <dspxmodelSelectionModel/DSPXModelSelectionModelGlobal.h>

namespace dspx {

    class Model;

    class AnchorNodeSelectionModel;
    class ClipSelectionModel;
    class DynamicMixingAnchorSelectionModel;
    class KeySignatureSelectionModel;
    class LabelSelectionModel;
    class NoteSelectionModel;
    class TempoSelectionModel;
    class TrackSelectionModel;
    class SelectionModelPrivate;

    /**
     * @brief Coordinates selection across model item types.
     *
     * SelectionModel owns one typed selection model per supported item type and
     * exposes the active one through selectionType(), currentItemSelectionModel(),
     * currentItem(), and selectedCount(). At most one item type is active at a
     * time; selecting an item of another type clears the previously active typed
     * selection.
     *
     * The typed models only accept items that belong to this object's model().
     * currentItem() is a focus item and does not have to be part of the selected
     * item set.
     *
     * @see AnchorNodeSelectionModel, ClipSelectionModel, DynamicMixingAnchorSelectionModel, KeySignatureSelectionModel, LabelSelectionModel, NoteSelectionModel, TempoSelectionModel, TrackSelectionModel
     */
    class DSPXMODEL_SELECTIONMODEL_EXPORT SelectionModel : public QObject {
        Q_OBJECT
        QML_ELEMENT
        QML_UNCREATABLE("")
        Q_DECLARE_PRIVATE(SelectionModel)
        Q_PROPERTY(Model *model READ model CONSTANT)
        Q_PROPERTY(SelectionType selectionType READ selectionType NOTIFY selectionTypeChanged)
        Q_PROPERTY(AnchorNodeSelectionModel *anchorNodeSelectionModel READ anchorNodeSelectionModel CONSTANT)
        Q_PROPERTY(ClipSelectionModel *clipSelectionModel READ clipSelectionModel CONSTANT)
        Q_PROPERTY(DynamicMixingAnchorSelectionModel *dynamicMixingAnchorSelectionModel READ dynamicMixingAnchorSelectionModel CONSTANT)
        Q_PROPERTY(KeySignatureSelectionModel *keySignatureSelectionModel READ keySignatureSelectionModel CONSTANT)
        Q_PROPERTY(LabelSelectionModel *labelSelectionModel READ labelSelectionModel CONSTANT)
        Q_PROPERTY(NoteSelectionModel *noteSelectionModel READ noteSelectionModel CONSTANT)
        Q_PROPERTY(TempoSelectionModel *tempoSelectionModel READ tempoSelectionModel CONSTANT)
        Q_PROPERTY(TrackSelectionModel *trackSelectionModel READ trackSelectionModel CONSTANT)
        Q_PROPERTY(QObject *currentItemSelectionModel READ currentItemSelectionModel NOTIFY currentItemSelectionModelChanged)
        Q_PROPERTY(QObject *currentItem READ currentItem NOTIFY currentItemChanged)
        Q_PROPERTY(int selectedCount READ selectedCount NOTIFY selectedCountChanged)

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
            ST_DynamicMixingAnchor
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
        KeySignatureSelectionModel *keySignatureSelectionModel() const;
        LabelSelectionModel *labelSelectionModel() const;
        NoteSelectionModel *noteSelectionModel() const;
        TempoSelectionModel *tempoSelectionModel() const;
        TrackSelectionModel *trackSelectionModel() const;

        /**
         * @brief Gets the typed selection model for selectionType(), or nullptr.
         */
        QObject *currentItemSelectionModel() const;
        /**
         * @brief Gets the current item in the active typed selection model.
         */
        QObject *currentItem() const;
        /**
         * @brief Gets selected item count in the active typed selection model.
         * @post selectedCount() >= 0.
         */
        int selectedCount() const;
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
         */
        Q_INVOKABLE void select(QObject *item, SelectionCommand command, SelectionType emptySelectionType = {}, QObject *containerItemHint = {});

    signals:
        void selectionTypeChanged();
        void currentItemSelectionModelChanged();
        void currentItemChanged();
        void selectedCountChanged();
        void itemSelected(QObject *item, bool selected);

    private:
        QScopedPointer<SelectionModelPrivate> d_ptr;
    };

    Q_DECLARE_OPERATORS_FOR_FLAGS(SelectionModel::SelectionCommand)

}

#endif // DSPXMODEL_SELECTIONMODEL_H
