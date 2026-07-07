#ifndef DSPXMODEL_TEMPOSELECTIONMODEL_H
#define DSPXMODEL_TEMPOSELECTIONMODEL_H

#include <QList>
#include <QObject>
#include <QScopedPointer>
#include <qqmlintegration.h>

#include <dspxmodelSelectionModel/DSPXModelSelectionModelGlobal.h>

namespace dspx {

    class SelectionModel;
    class Tempo;
    class TempoSelectionModelPrivate;

    /**
     * @brief Selection model for tempos in SelectionModel::model()->tempos().
     *
     * currentItem() is independent from selectedItems(); it may be set to a valid
     * tempo that is not selected. Selected or current tempos are dropped when
     * they leave the owning model's tempo sequence.
     */
    class DSPXMODEL_SELECTIONMODEL_EXPORT TempoSelectionModel : public QObject {
        Q_OBJECT
        QML_ELEMENT
        QML_UNCREATABLE("")
        Q_DECLARE_PRIVATE(TempoSelectionModel)
        Q_PROPERTY(Tempo *currentItem READ currentItem NOTIFY currentItemChanged)
        Q_PROPERTY(QList<Tempo *> selectedItems READ selectedItems NOTIFY selectedItemsChanged)
        Q_PROPERTY(int selectedCount READ selectedCount NOTIFY selectedCountChanged)

    public:
        ~TempoSelectionModel() override;

        /**
         * @brief Gets the current tempo, or nullptr.
         */
        Tempo *currentItem() const;
        /**
         * @brief Gets selected tempos.
         */
        QList<Tempo *> selectedItems() const;
        /**
         * @brief Gets selected tempo count.
         * @post selectedCount() >= 0.
         */
        int selectedCount() const;

        /**
         * @brief Gets whether item is selected.
         */
        Q_INVOKABLE bool isItemSelected(Tempo *item) const;

    signals:
        void currentItemChanged();
        void selectedItemsChanged();
        void selectedCountChanged();
        void itemSelected(Tempo *item, bool selected);

    private:
        friend class SelectionModel;
        explicit TempoSelectionModel(SelectionModel *parent = nullptr);
        QScopedPointer<TempoSelectionModelPrivate> d_ptr;
    };

}

#endif // DSPXMODEL_TEMPOSELECTIONMODEL_H
