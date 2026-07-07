#ifndef DSPXMODEL_CLIPSELECTIONMODEL_H
#define DSPXMODEL_CLIPSELECTIONMODEL_H

#include <QList>
#include <QObject>
#include <QScopedPointer>
#include <qqmlintegration.h>

#include <dspxmodelSelectionModel/DSPXModelSelectionModelGlobal.h>

namespace dspx {

    class SelectionModel;
    class Clip;
    class ClipSequence;
    class ClipSelectionModelPrivate;

    /**
     * @brief Selection model for clips in the owning SelectionModel's tracks.
     *
     * A selectable clip must belong to a ClipSequence whose track belongs to
     * SelectionModel::model()->tracks(). Clips from multiple ClipSequence objects
     * may be selected at the same time. The model tracks the selected clip type
     * counts and the set of ClipSequence objects that contain selected clips.
     *
     * currentItem() is independent from selectedItems(); it may be set to a valid
     * clip that is not selected.
     */
    class DSPXMODEL_SELECTIONMODEL_EXPORT ClipSelectionModel : public QObject {
        Q_OBJECT
        QML_ELEMENT
        QML_UNCREATABLE("")
        Q_DECLARE_PRIVATE(ClipSelectionModel)
        Q_PROPERTY(Clip *currentItem READ currentItem NOTIFY currentItemChanged)
        Q_PROPERTY(QList<Clip *> selectedItems READ selectedItems NOTIFY selectedItemsChanged)
        Q_PROPERTY(int selectedCount READ selectedCount NOTIFY selectedCountChanged)
        Q_PROPERTY(int selectedSingingClipCount READ selectedSingingClipCount NOTIFY selectedSingingClipCountChanged)
        Q_PROPERTY(int selectedAudioClipCount READ selectedAudioClipCount NOTIFY selectedAudioClipCountChanged)
        Q_PROPERTY(QList<ClipSequence *> clipSequencesWithSelectedItems READ clipSequencesWithSelectedItems NOTIFY clipSequencesWithSelectedItemsChanged)

    public:
        ~ClipSelectionModel() override;

        /**
         * @brief Gets the current clip, or nullptr.
         */
        Clip *currentItem() const;
        /**
         * @brief Gets selected clips.
         */
        QList<Clip *> selectedItems() const;
        /**
         * @brief Gets selected clip count.
         * @post selectedCount() >= 0.
         */
        int selectedCount() const;
        /**
         * @brief Gets selected singing clip count.
         * @post selectedSingingClipCount() >= 0.
         */
        int selectedSingingClipCount() const;
        /**
         * @brief Gets selected audio clip count.
         * @post selectedAudioClipCount() >= 0.
         */
        int selectedAudioClipCount() const;
        /**
         * @brief Gets clip sequences that contain selected clips.
         */
        QList<ClipSequence *> clipSequencesWithSelectedItems() const;

        /**
         * @brief Gets whether item is selected.
         */
        Q_INVOKABLE bool isItemSelected(Clip *item) const;

    signals:
        void currentItemChanged();
        void selectedItemsChanged();
        void selectedCountChanged();
        void selectedSingingClipCountChanged();
        void selectedAudioClipCountChanged();
        void clipSequencesWithSelectedItemsChanged();
        void itemSelected(Clip *item, bool selected);

    private:
        friend class SelectionModel;
        explicit ClipSelectionModel(SelectionModel *parent = nullptr);
        QScopedPointer<ClipSelectionModelPrivate> d_ptr;
    };

}

#endif // DSPXMODEL_CLIPSELECTIONMODEL_H
