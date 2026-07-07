#ifndef DSPXMODEL_TRACKSELECTIONMODEL_H
#define DSPXMODEL_TRACKSELECTIONMODEL_H

#include <QList>
#include <QObject>
#include <QScopedPointer>
#include <qqmlintegration.h>

#include <dspxmodelSelectionModel/DSPXModelSelectionModelGlobal.h>

namespace dspx {

    class Track;
    class SelectionModel;
    class TrackSelectionModelPrivate;

    /**
     * @brief Selection model for tracks in SelectionModel::model()->tracks().
     *
     * currentItem() is independent from selectedItems(); it may be set to a valid
     * track that is not selected. Selected or current tracks are dropped when
     * they leave the owning model's track list.
     */
    class DSPXMODEL_SELECTIONMODEL_EXPORT TrackSelectionModel : public QObject {
        Q_OBJECT
        QML_ELEMENT
        QML_UNCREATABLE("")
        Q_DECLARE_PRIVATE(TrackSelectionModel)
        Q_PROPERTY(Track *currentItem READ currentItem NOTIFY currentItemChanged)
        Q_PROPERTY(QList<Track *> selectedItems READ selectedItems NOTIFY selectedItemsChanged)
        Q_PROPERTY(int selectedCount READ selectedCount NOTIFY selectedCountChanged)

    public:
        ~TrackSelectionModel() override;

        /**
         * @brief Gets the current track, or nullptr.
         */
        Track *currentItem() const;
        /**
         * @brief Gets selected tracks.
         */
        QList<Track *> selectedItems() const;
        /**
         * @brief Gets selected track count.
         * @post selectedCount() >= 0.
         */
        int selectedCount() const;
        /**
         * @brief Gets whether item is selected.
         */
        Q_INVOKABLE bool isItemSelected(Track *item) const;

    signals:
        void currentItemChanged();
        void selectedItemsChanged();
        void selectedCountChanged();
        void itemSelected(Track *item, bool selected);

    private:
        friend class SelectionModel;
        explicit TrackSelectionModel(SelectionModel *parent = nullptr);
        QScopedPointer<TrackSelectionModelPrivate> d_ptr;
    };

}

#endif // DSPXMODEL_TRACKSELECTIONMODEL_H
