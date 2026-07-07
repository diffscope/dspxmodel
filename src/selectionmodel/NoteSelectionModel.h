#ifndef DSPXMODEL_NOTESELECTIONMODEL_H
#define DSPXMODEL_NOTESELECTIONMODEL_H

#include <QList>
#include <QObject>
#include <QScopedPointer>
#include <qqmlintegration.h>

#include <dspxmodelSelectionModel/DSPXModelSelectionModelGlobal.h>

namespace dspx {

    class SelectionModel;
    class Note;
    class NoteSequence;
    class NoteSelectionModelPrivate;

    /**
     * @brief Selection model for notes in the owning SelectionModel's tracks.
     *
     * A selectable note must belong to a NoteSequence whose SingingClip is in a
     * ClipSequence under SelectionModel::model()->tracks(). All selected notes
     * are constrained to one NoteSequence. Selecting a note from another valid
     * NoteSequence clears the previous note selection and switches the context.
     *
     * noteSequenceWithSelectedItems() stores the note selection context. It may
     * remain set when selectedCount() is 0 so empty selections can still be tied
     * to a NoteSequence. It is reset when the context becomes invalid or switches.
     * currentItem() is independent from selectedItems().
     */
    class DSPXMODEL_SELECTIONMODEL_EXPORT NoteSelectionModel : public QObject {
        Q_OBJECT
        QML_ELEMENT
        QML_UNCREATABLE("")
        Q_DECLARE_PRIVATE(NoteSelectionModel)
        Q_PROPERTY(Note *currentItem READ currentItem NOTIFY currentItemChanged)
        Q_PROPERTY(QList<Note *> selectedItems READ selectedItems NOTIFY selectedItemsChanged)
        Q_PROPERTY(int selectedCount READ selectedCount NOTIFY selectedCountChanged)
        Q_PROPERTY(NoteSequence *noteSequenceWithSelectedItems READ noteSequenceWithSelectedItems NOTIFY
                       noteSequenceWithSelectedItemsChanged)

    public:
        ~NoteSelectionModel() override;

        /**
         * @brief Gets the current note, or nullptr.
         */
        Note *currentItem() const;
        /**
         * @brief Gets selected notes.
         */
        QList<Note *> selectedItems() const;
        /**
         * @brief Gets selected note count.
         * @post selectedCount() >= 0.
         */
        int selectedCount() const;
        /**
         * @brief Gets the note sequence context for selected or empty note selection.
         */
        NoteSequence *noteSequenceWithSelectedItems() const;

        /**
         * @brief Gets whether item is selected.
         */
        Q_INVOKABLE bool isItemSelected(Note *item) const;

    signals:
        void currentItemChanged();
        void selectedItemsChanged();
        void selectedCountChanged();
        void noteSequenceWithSelectedItemsChanged();
        void itemSelected(Note *item, bool selected);

    private:
        friend class SelectionModel;
        explicit NoteSelectionModel(QObject *parent = nullptr);
        QScopedPointer<NoteSelectionModelPrivate> d_ptr;
    };

}

#endif // DSPXMODEL_NOTESELECTIONMODEL_H
