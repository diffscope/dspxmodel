#ifndef DSPXMODEL_ANCHORNODESELECTIONMODEL_H
#define DSPXMODEL_ANCHORNODESELECTIONMODEL_H

#include <QList>
#include <QObject>
#include <QScopedPointer>
#include <qqmlintegration.h>

#include <dspxmodelSelectionModel/DSPXModelSelectionModelGlobal.h>

namespace dspx {

    class AnchorNode;
    class AnchorNodeSequence;
    class AnchorNodeSelectionModelPrivate;
    class SelectionModel;

    /**
     * @brief Selection model for anchor nodes in one AnchorNodeSequence.
     *
     * A selectable anchor node must belong to the owning SelectionModel's model
     * and must be contained in an AnchorNodeSequence. All selected anchor nodes
     * are constrained to one AnchorNodeSequence. Selecting an anchor node from
     * another valid sequence clears the previous selection and switches the
     * sequence context.
     *
     * anchorNodeSequenceWithSelectedItems() stores the anchor node selection
     * context. It may remain set when selectedCount() is 0 so empty selections
     * can still be tied to a sequence. currentItem() is independent from
     * selectedItems().
     */
    class DSPXMODEL_SELECTIONMODEL_EXPORT AnchorNodeSelectionModel : public QObject {
        Q_OBJECT
        QML_ELEMENT
        QML_UNCREATABLE("")
        Q_DECLARE_PRIVATE(AnchorNodeSelectionModel)
        Q_PROPERTY(AnchorNode *currentItem READ currentItem NOTIFY currentItemChanged)
        Q_PROPERTY(QList<AnchorNode *> selectedItems READ selectedItems NOTIFY selectedItemsChanged)
        Q_PROPERTY(int selectedCount READ selectedCount NOTIFY selectedCountChanged)
        Q_PROPERTY(AnchorNodeSequence *anchorNodeSequenceWithSelectedItems READ anchorNodeSequenceWithSelectedItems NOTIFY anchorNodeSequenceWithSelectedItemsChanged)

    public:
        ~AnchorNodeSelectionModel() override;

        /**
         * @brief Gets the current anchor node, or nullptr.
         */
        AnchorNode *currentItem() const;
        /**
         * @brief Gets selected anchor nodes.
         */
        QList<AnchorNode *> selectedItems() const;
        /**
         * @brief Gets selected anchor node count.
         * @post selectedCount() >= 0.
         */
        int selectedCount() const;
        /**
         * @brief Gets the anchor node sequence context for selected or empty selection.
         */
        AnchorNodeSequence *anchorNodeSequenceWithSelectedItems() const;

        /**
         * @brief Gets whether item is selected.
         */
        Q_INVOKABLE bool isItemSelected(AnchorNode *item) const;

    signals:
        void currentItemChanged();
        void selectedItemsChanged();
        void selectedCountChanged();
        void anchorNodeSequenceWithSelectedItemsChanged();
        void itemSelected(AnchorNode *item, bool selected);

    private:
        friend class SelectionModel;
        explicit AnchorNodeSelectionModel(QObject *parent = nullptr);
        QScopedPointer<AnchorNodeSelectionModelPrivate> d_ptr;
    };

}

#endif // DSPXMODEL_ANCHORNODESELECTIONMODEL_H
