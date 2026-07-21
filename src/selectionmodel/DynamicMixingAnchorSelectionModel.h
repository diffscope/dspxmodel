#ifndef DSPXMODEL_DYNAMICMIXINGANCHORSELECTIONMODEL_H
#define DSPXMODEL_DYNAMICMIXINGANCHORSELECTIONMODEL_H

#include <QList>
#include <QObject>
#include <QScopedPointer>
#include <qqmlintegration.h>

#include <dspxmodelSelectionModel/DSPXModelSelectionModelGlobal.h>

namespace dspx {

    class DynamicMixingAnchor;
    class DynamicMixingAnchorSequence;
    class DynamicMixingAnchorSelectionModelPrivate;
    class SelectionModel;

    /**
     * @brief Selection model for anchors in one DynamicMixingAnchorSequence.
     *
     * The sequence context may remain set for an empty selection so editing and
     * paste operations still have an explicit target. Selecting an anchor from
     * another sequence clears the previous selection and switches the context.
     */
    class DSPXMODEL_SELECTIONMODEL_EXPORT DynamicMixingAnchorSelectionModel : public QObject {
        Q_OBJECT
        QML_ELEMENT
        QML_UNCREATABLE("")
        Q_DECLARE_PRIVATE(DynamicMixingAnchorSelectionModel)
        Q_PROPERTY(DynamicMixingAnchor *currentItem READ currentItem NOTIFY currentItemChanged)
        Q_PROPERTY(QList<DynamicMixingAnchor *> selectedItems READ selectedItems NOTIFY selectedItemsChanged)
        Q_PROPERTY(int selectedCount READ selectedCount NOTIFY selectedCountChanged)
        Q_PROPERTY(DynamicMixingAnchorSequence *dynamicMixingAnchorSequenceWithSelectedItems READ dynamicMixingAnchorSequenceWithSelectedItems NOTIFY dynamicMixingAnchorSequenceWithSelectedItemsChanged)

    public:
        ~DynamicMixingAnchorSelectionModel() override;

        /**
         * @brief Gets the current dynamic mixing anchor, or nullptr.
         */
        DynamicMixingAnchor *currentItem() const;
        /**
         * @brief Gets selected dynamic mixing anchors.
         */
        QList<DynamicMixingAnchor *> selectedItems() const;
        /**
         * @brief Gets selected dynamic mixing anchor count.
         */
        int selectedCount() const;
        /**
         * @brief Gets the sequence context for selected or empty selection.
         */
        DynamicMixingAnchorSequence *dynamicMixingAnchorSequenceWithSelectedItems() const;

        /**
         * @brief Gets whether item is selected.
         */
        Q_INVOKABLE bool isItemSelected(DynamicMixingAnchor *item) const;

    Q_SIGNALS:
        void currentItemChanged();
        void selectedItemsChanged();
        void selectedCountChanged();
        void dynamicMixingAnchorSequenceWithSelectedItemsChanged();
        void itemSelected(DynamicMixingAnchor *item, bool selected);

    private:
        friend class SelectionModel;
        explicit DynamicMixingAnchorSelectionModel(QObject *parent = nullptr);
        QScopedPointer<DynamicMixingAnchorSelectionModelPrivate> d_ptr;
    };

}

#endif // DSPXMODEL_DYNAMICMIXINGANCHORSELECTIONMODEL_H
