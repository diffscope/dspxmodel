#include "ClipSelectionModel.h"
#include "ClipSelectionModel_p.h"

#include <QList>
#include <QPointer>

#include <dspxmodelORM/Model.h>
#include <dspxmodelORM/Clip.h>
#include <dspxmodelORM/ClipSequence.h>
#include <dspxmodelORM/Track.h>
#include <dspxmodelORM/TrackList.h>

namespace dspx {

    bool ClipSelectionModelPrivate::isValidItem(Clip *item) const {
        if (!item) {
            return false;
        }
        auto clipSeq = item->clipSequence();
        if (!clipSeq) {
            return false;
        }
        auto track = clipSeq->track();
        if (!track) {
            return false;
        }
        auto trackList = track->trackList();
        if (!trackList) {
            return false;
        }
        return trackList == selectionModel->model()->tracks();
    }

    void ClipSelectionModelPrivate::connectTrackForItem(Clip *item) {
        disconnectTrackForItem(item);

        if (!item) {
            return;
        }

        auto *clipSeq = item->clipSequence();
        if (!clipSeq) {
            return;
        }

        auto *track = clipSeq->track();
        if (!track) {
            return;
        }

        QPointer item_ = item;
        trackListConnections[item] = QObject::connect(track, &Track::trackListChanged, q_ptr, [this, item_]() {
            if (!isValidItem(item_)) {
                dropItem(item_);
            }
        });
    }

    void ClipSelectionModelPrivate::disconnectTrackForItem(Clip *item) {
        const auto connection = trackListConnections.take(item);
        if (connection) {
            QObject::disconnect(connection);
        }
    }

    void ClipSelectionModelPrivate::connectItem(Clip *item) {
        if (connectedItems.contains(item)) {
            return;
        }
        QObject::connect(item, &QObject::destroyed, q_ptr, [this](QObject *obj) {
            dropItem(static_cast<Clip *>(obj));
        });
        QObject::connect(item, &Clip::clipSequenceChanged, q_ptr, [this, item]() {
            auto oldClipSequence = clipToClipSequence.value(item, nullptr);
            auto newClipSequence = item->clipSequence();
            
            if (oldClipSequence != newClipSequence) {
                if (newClipSequence) {
                    clipToClipSequence[item] = newClipSequence;
                } else {
                    clipToClipSequence.remove(item);
                }
                
                if (selectedItems.contains(item)) {
                    if (oldClipSequence) {
                        auto &items = clipSequencesWithSelectedItems[oldClipSequence];
                        items.remove(item);
                        if (items.isEmpty()) {
                            clipSequencesWithSelectedItems.remove(oldClipSequence);
                        }
                    }
                    if (newClipSequence) {
                        clipSequencesWithSelectedItems[newClipSequence].insert(item);
                    }
                    Q_EMIT q_ptr->clipSequencesWithSelectedItemsChanged();
                }

                connectTrackForItem(item);
            }
            
            if (!isValidItem(item)) {
                dropItem(item);
            }
        });
        
        auto clipSeq = item->clipSequence();
        if (clipSeq) {
            clipToClipSequence[item] = clipSeq;
        }
        connectTrackForItem(item);
        
        connectedItems.insert(item);
    }

    void ClipSelectionModelPrivate::disconnectItem(Clip *item) {
        QObject::disconnect(item, nullptr, q_ptr, nullptr);
        disconnectTrackForItem(item);
        clipToClipSequence.remove(item);
        connectedItems.remove(item);
    }

    bool ClipSelectionModelPrivate::addToSelection(Clip *item) {
        if (!isValidItem(item) || selectedItems.contains(item)) {
            return false;
        }
        connectItem(item);
        selectedItems.insert(item);
        const auto clipType = item->type();
        selectedClipTypes[item] = clipType;
        switch (clipType) {
            case Clip::Singing:
                ++selectedSingingClipCount;
                break;
            case Clip::Audio:
                ++selectedAudioClipCount;
                break;
        }
        
        auto clipSeq = item->clipSequence();
        if (clipSeq) {
            clipToClipSequence[item] = clipSeq;
            clipSequencesWithSelectedItems[clipSeq].insert(item);
        }
        
        Q_EMIT q_ptr->itemSelected(item, true);
        return true;
    }

    bool ClipSelectionModelPrivate::removeFromSelection(Clip *item) {
        if (!item) {
            return false;
        }
        if (!selectedItems.remove(item)) {
            return false;
        }
        if (selectedClipTypes.contains(item)) {
            const auto clipType = selectedClipTypes.value(item);
            switch (clipType) {
                case Clip::Singing:
                    --selectedSingingClipCount;
                    break;
                case Clip::Audio:
                    --selectedAudioClipCount;
                    break;
            }
        }
        selectedClipTypes.remove(item);
        
        auto clipSeq = clipToClipSequence.value(item);
        if (clipSeq) {
            auto &items = clipSequencesWithSelectedItems[clipSeq];
            items.remove(item);
            if (items.isEmpty()) {
                clipSequencesWithSelectedItems.remove(clipSeq);
            }
        }
        if (item == currentItem && item->clipSequence()) {
            clipToClipSequence[item] = item->clipSequence();
        } else {
            clipToClipSequence.remove(item);
        }
        
        if (item != currentItem) {
            disconnectItem(item);
        }
        Q_EMIT q_ptr->itemSelected(item, false);
        return true;
    }

    bool ClipSelectionModelPrivate::clearSelection() {
        if (selectedItems.isEmpty()) {
            return false;
        }
        const auto items = selectedItems.values();
        bool selectionChanged = false;
        for (auto clip : items) {
            selectionChanged |= removeFromSelection(clip);
        }
        return selectionChanged;
    }

    void ClipSelectionModelPrivate::dropItem(Clip *item) {
        if (!item) {
            return;
        }
        const int oldCount = selectedItems.size();
        const int oldSingingClipCount = selectedSingingClipCount;
        const int oldAudioClipCount = selectedAudioClipCount;
        bool selectionChanged = removeFromSelection(item);
        bool countChanged = selectionChanged && oldCount != selectedItems.size();
        bool singingClipCountChanged = selectionChanged && oldSingingClipCount != selectedSingingClipCount;
        bool audioClipCountChanged = selectionChanged && oldAudioClipCount != selectedAudioClipCount;
        bool currentChanged = false;
        bool clipSequencesChanged = selectionChanged;
        
        if (currentItem == item) {
            if (!selectedItems.contains(item)) {
                disconnectItem(item);
            }
            currentItem = nullptr;
            currentChanged = true;
        }
        if (selectionChanged) {
            Q_EMIT q_ptr->selectedItemsChanged();
            if (countChanged) {
                Q_EMIT q_ptr->selectedCountChanged();
            }
            if (singingClipCountChanged) {
                Q_EMIT q_ptr->selectedSingingClipCountChanged();
            }
            if (audioClipCountChanged) {
                Q_EMIT q_ptr->selectedAudioClipCountChanged();
            }
        }
        if (clipSequencesChanged) {
            Q_EMIT q_ptr->clipSequencesWithSelectedItemsChanged();
        }
        if (currentChanged) {
            Q_EMIT q_ptr->currentItemChanged();
        }
    }

    void ClipSelectionModelPrivate::setCurrentItem(Clip *item) {
        if (!isValidItem(item)) {
            item = nullptr;
        }
        if (currentItem == item) {
            return;
        }
        auto oldItem = currentItem;
        currentItem = item;
        if (oldItem && !selectedItems.contains(oldItem)) {
            disconnectItem(oldItem);
        }
        if (currentItem && !selectedItems.contains(currentItem)) {
            connectItem(currentItem);
        }
        Q_EMIT q_ptr->currentItemChanged();
    }

    ClipSelectionModel::ClipSelectionModel(SelectionModel *parent) : QObject(parent), d_ptr(new ClipSelectionModelPrivate) {
        Q_D(ClipSelectionModel);
        d->q_ptr = this;
        d->selectionModel = parent;
    }

    ClipSelectionModel::~ClipSelectionModel() = default;

    Clip *ClipSelectionModel::currentItem() const {
        Q_D(const ClipSelectionModel);
        return d->currentItem;
    }

    QList<Clip *> ClipSelectionModel::selectedItems() const {
        Q_D(const ClipSelectionModel);
        return d->selectedItems.values();
    }

    int ClipSelectionModel::selectedCount() const {
        Q_D(const ClipSelectionModel);
        return d->selectedItems.size();
    }

    int ClipSelectionModel::selectedSingingClipCount() const {
        Q_D(const ClipSelectionModel);
        return d->selectedSingingClipCount;
    }

    int ClipSelectionModel::selectedAudioClipCount() const {
        Q_D(const ClipSelectionModel);
        return d->selectedAudioClipCount;
    }

    QList<ClipSequence *> ClipSelectionModel::clipSequencesWithSelectedItems() const {
        Q_D(const ClipSelectionModel);
        return d->clipSequencesWithSelectedItems.keys();
    }

    bool ClipSelectionModel::isItemSelected(Clip *item) const {
        Q_D(const ClipSelectionModel);
        return d->selectedItems.contains(item);
    }

    void ClipSelectionModelPrivate::select(Clip *item, SelectionModel::SelectionCommand command) {
        const int oldCount = selectedItems.size();
        const int oldSingingClipCount = selectedSingingClipCount;
        const int oldAudioClipCount = selectedAudioClipCount;
        bool selectionChanged = false;
        
        if (command & SelectionModel::ClearPreviousSelection) {
            selectionChanged |= clearSelection();
        }
        if ((command & SelectionModel::Select) && (command & SelectionModel::Deselect)) {
            if (selectedItems.contains(item)) {
                selectionChanged |= removeFromSelection(item);
            } else {
                selectionChanged |= addToSelection(item);
            }
        } else if (command & SelectionModel::Select) {
            selectionChanged |= addToSelection(item);
        } else if (command & SelectionModel::Deselect) {
            selectionChanged |= removeFromSelection(item);
        }
        if (command & SelectionModel::SetCurrentItem) {
            setCurrentItem(item);
        }

        bool clipSequencesChanged = selectionChanged;
        
        if (selectionChanged) {
            Q_EMIT q_ptr->selectedItemsChanged();
            if (oldCount != selectedItems.size()) {
                Q_EMIT q_ptr->selectedCountChanged();
            }
            if (oldSingingClipCount != selectedSingingClipCount) {
                Q_EMIT q_ptr->selectedSingingClipCountChanged();
            }
            if (oldAudioClipCount != selectedAudioClipCount) {
                Q_EMIT q_ptr->selectedAudioClipCountChanged();
            }
        }
        if (clipSequencesChanged) {
            Q_EMIT q_ptr->clipSequencesWithSelectedItemsChanged();
        }
    }

}

#include "moc_ClipSelectionModel.cpp"
