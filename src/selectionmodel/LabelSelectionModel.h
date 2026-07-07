#ifndef DSPXMODEL_LABELSELECTIONMODEL_H
#define DSPXMODEL_LABELSELECTIONMODEL_H

#include <QList>
#include <QObject>
#include <QScopedPointer>
#include <qqmlintegration.h>

#include <dspxmodelSelectionModel/DSPXModelSelectionModelGlobal.h>

namespace dspx {

    class SelectionModel;
    class Label;
    class LabelSelectionModelPrivate;

    /**
     * @brief Selection model for labels in SelectionModel::model()->labels().
     *
     * currentItem() is independent from selectedItems(); it may be set to a valid
     * label that is not selected. Selected or current labels are dropped when
     * they leave the owning model's label sequence.
     */
    class DSPXMODEL_SELECTIONMODEL_EXPORT LabelSelectionModel : public QObject {
        Q_OBJECT
        QML_ELEMENT
        QML_UNCREATABLE("")
        Q_DECLARE_PRIVATE(LabelSelectionModel)
        Q_PROPERTY(Label *currentItem READ currentItem NOTIFY currentItemChanged)
        Q_PROPERTY(QList<Label *> selectedItems READ selectedItems NOTIFY selectedItemsChanged)
        Q_PROPERTY(int selectedCount READ selectedCount NOTIFY selectedCountChanged)

    public:
        ~LabelSelectionModel() override;

        /**
         * @brief Gets the current label, or nullptr.
         */
        Label *currentItem() const;
        /**
         * @brief Gets selected labels.
         */
        QList<Label *> selectedItems() const;
        /**
         * @brief Gets selected label count.
         * @post selectedCount() >= 0.
         */
        int selectedCount() const;

        /**
         * @brief Gets whether item is selected.
         */
        Q_INVOKABLE bool isItemSelected(Label *item) const;

    signals:
        void currentItemChanged();
        void selectedItemsChanged();
        void selectedCountChanged();
        void itemSelected(Label *item, bool selected);

    private:
        friend class SelectionModel;
        explicit LabelSelectionModel(SelectionModel *parent);
        QScopedPointer<LabelSelectionModelPrivate> d_ptr;
    };

}

#endif // DSPXMODEL_LABELSELECTIONMODEL_H
