#ifndef DSPXMODEL_KEYSIGNATURESELECTIONMODEL_H
#define DSPXMODEL_KEYSIGNATURESELECTIONMODEL_H

#include <QList>
#include <QObject>
#include <QScopedPointer>
#include <qqmlintegration.h>

#include <dspxmodelSelectionModel/DSPXModelSelectionModelGlobal.h>

namespace dspx {

    class SelectionModel;
    class KeySignature;
    class KeySignatureSelectionModelPrivate;

    /**
     * @brief Selection model for key signatures in SelectionModel::model()->keySignatures().
     *
     * currentItem() is independent from selectedItems(); it may be set to a valid
     * key signature that is not selected. Selected or current key signatures are
     * dropped when they leave the owning model's key signature sequence.
     */
    class DSPXMODEL_SELECTIONMODEL_EXPORT KeySignatureSelectionModel : public QObject {
        Q_OBJECT
        QML_ELEMENT
        QML_UNCREATABLE("")
        Q_DECLARE_PRIVATE(KeySignatureSelectionModel)
        Q_PROPERTY(KeySignature *currentItem READ currentItem NOTIFY currentItemChanged)
        Q_PROPERTY(QList<KeySignature *> selectedItems READ selectedItems NOTIFY selectedItemsChanged)
        Q_PROPERTY(int selectedCount READ selectedCount NOTIFY selectedCountChanged)

    public:
        ~KeySignatureSelectionModel() override;

        /**
         * @brief Gets the current key signature, or nullptr.
         */
        KeySignature *currentItem() const;
        /**
         * @brief Gets selected key signatures.
         */
        QList<KeySignature *> selectedItems() const;
        /**
         * @brief Gets selected key signature count.
         * @post selectedCount() >= 0.
         */
        int selectedCount() const;

        /**
         * @brief Gets whether item is selected.
         */
        Q_INVOKABLE bool isItemSelected(KeySignature *item) const;

    signals:
        void currentItemChanged();
        void selectedItemsChanged();
        void selectedCountChanged();
        void itemSelected(KeySignature *item, bool selected);

    private:
        friend class SelectionModel;
        explicit KeySignatureSelectionModel(SelectionModel *parent = nullptr);
        QScopedPointer<KeySignatureSelectionModelPrivate> d_ptr;
    };

}

#endif // DSPXMODEL_KEYSIGNATURESELECTIONMODEL_H
