#include "AnchorNodeSelectionModel.h"
#include "AnchorNodeSelectionModel_p.h"

#include <dspxmodelORM/AnchorNode.h>
#include <dspxmodelORM/AnchorNodeSequence.h>
#include <dspxmodelORM/Parameter.h>
#include <dspxmodelSelectionModel/SelectionModel.h>

namespace dspx {

    bool AnchorNodeSelectionModelPrivate::isValidItem(AnchorNode *item) const {
        return item && item->model() == selectionModel->model() && item->anchorNodeSequence();
    }

    bool AnchorNodeSelectionModelPrivate::isValidAnchorNodeSequence(AnchorNodeSequence *sequence) const {
        return sequence && sequence->parameter() && sequence->parameter()->model() == selectionModel->model();
    }

    void AnchorNodeSelectionModelPrivate::connectItem(AnchorNode *item) {
        if (connectedItems.contains(item)) {
            return;
        }
        QObject::connect(item, &QObject::destroyed, q_ptr, [this](QObject *obj) {
            dropItem(static_cast<AnchorNode *>(obj));
        });
        QObject::connect(item, &AnchorNode::anchorNodeSequenceChanged, q_ptr, [this, item]() {
            const auto newSequence = item->anchorNodeSequence();
            if (newSequence != anchorNodeSequenceWithSelectedItems) {
                dropItem(item);
            }
        });
        connectedItems.insert(item);
    }

    void AnchorNodeSelectionModelPrivate::disconnectItem(AnchorNode *item) {
        QObject::disconnect(item, nullptr, q_ptr, nullptr);
        connectedItems.remove(item);
    }

    bool AnchorNodeSelectionModelPrivate::addToSelection(AnchorNode *item) {
        if (!isValidItem(item) || selectedItems.contains(item)) {
            return false;
        }
        connectItem(item);
        selectedItems.insert(item);
        Q_EMIT q_ptr->itemSelected(item, true);
        return true;
    }

    bool AnchorNodeSelectionModelPrivate::removeFromSelection(AnchorNode *item) {
        if (!item) {
            return false;
        }
        if (!selectedItems.remove(item)) {
            return false;
        }
        if (item != currentItem) {
            disconnectItem(item);
        }
        Q_EMIT q_ptr->itemSelected(item, false);
        return true;
    }

    bool AnchorNodeSelectionModelPrivate::clearSelection() {
        if (selectedItems.isEmpty()) {
            return false;
        }
        const auto items = selectedItems.values();
        bool selectionChanged = false;
        for (auto *anchorNode : items) {
            selectionChanged |= removeFromSelection(anchorNode);
        }
        return selectionChanged;
    }

    void AnchorNodeSelectionModelPrivate::dropItem(AnchorNode *item) {
        if (!item) {
            return;
        }
        const int oldCount = selectedItems.size();
        const bool selectionChanged = removeFromSelection(item);
        const bool countChanged = selectionChanged && oldCount != selectedItems.size();
        bool currentChanged = false;
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
        }
        if (currentChanged) {
            Q_EMIT q_ptr->currentItemChanged();
        }
    }

    void AnchorNodeSelectionModelPrivate::setCurrentItem(AnchorNode *item) {
        if (!isValidItem(item)) {
            item = nullptr;
        }
        if (currentItem == item) {
            return;
        }
        auto *oldItem = currentItem;
        currentItem = item;
        if (oldItem && !selectedItems.contains(oldItem)) {
            disconnectItem(oldItem);
        }
        if (currentItem && !selectedItems.contains(currentItem)) {
            connectItem(currentItem);
        }
        Q_EMIT q_ptr->currentItemChanged();
    }

    void AnchorNodeSelectionModelPrivate::connectAnchorNodeSequence(AnchorNodeSequence *sequence) {
        if (!sequence) {
            return;
        }

        QObject::disconnect(anchorNodeSequenceDestroyedConnection);
        anchorNodeSequenceDestroyedConnection = QObject::connect(sequence, &QObject::destroyed, q_ptr, [this]() {
            clearAllAndResetAnchorNodeSequence();
        });
    }

    void AnchorNodeSelectionModelPrivate::disconnectAnchorNodeSequence() {
        QObject::disconnect(anchorNodeSequenceDestroyedConnection);
        anchorNodeSequenceDestroyedConnection = {};

        if (anchorNodeSequenceWithSelectedItems) {
            QObject::disconnect(anchorNodeSequenceWithSelectedItems, nullptr, q_ptr, nullptr);
        }
    }

    void AnchorNodeSelectionModelPrivate::clearAllAndResetAnchorNodeSequence() {
        const int oldCount = selectedItems.size();
        const bool selectionChanged = clearSelection();
        setCurrentItem(nullptr);
        const bool sequenceChanged = anchorNodeSequenceWithSelectedItems != nullptr;
        disconnectAnchorNodeSequence();
        anchorNodeSequenceWithSelectedItems = nullptr;

        if (selectionChanged) {
            Q_EMIT q_ptr->selectedItemsChanged();
            if (oldCount != selectedItems.size()) {
                Q_EMIT q_ptr->selectedCountChanged();
            }
        }
        if (sequenceChanged) {
            Q_EMIT q_ptr->anchorNodeSequenceWithSelectedItemsChanged();
        }
    }

    void AnchorNodeSelectionModelPrivate::select(AnchorNode *item, SelectionModel::SelectionCommand command, AnchorNodeSequence *containerItemHint) {
        const int oldCount = selectedItems.size();
        bool selectionChanged = false;
        bool sequenceChanged = false;

        if (command & SelectionModel::ClearPreviousSelection) {
            selectionChanged |= clearSelection();
        }

        if (!item && containerItemHint && isValidAnchorNodeSequence(containerItemHint) &&
            anchorNodeSequenceWithSelectedItems != containerItemHint) {
            selectionChanged |= clearSelection();
            if (currentItem && currentItem->anchorNodeSequence() != containerItemHint) {
                setCurrentItem(nullptr);
            }
            disconnectAnchorNodeSequence();
            anchorNodeSequenceWithSelectedItems = containerItemHint;
            connectAnchorNodeSequence(anchorNodeSequenceWithSelectedItems);
            sequenceChanged = true;
        }

        if (item && isValidItem(item)) {
            auto *itemSequence = item->anchorNodeSequence();

            if (!anchorNodeSequenceWithSelectedItems) {
                anchorNodeSequenceWithSelectedItems = itemSequence;
                connectAnchorNodeSequence(anchorNodeSequenceWithSelectedItems);
                sequenceChanged = true;
            } else if (anchorNodeSequenceWithSelectedItems != itemSequence) {
                selectionChanged |= clearSelection();
                if (currentItem) {
                    setCurrentItem(nullptr);
                }
                disconnectAnchorNodeSequence();
                anchorNodeSequenceWithSelectedItems = itemSequence;
                connectAnchorNodeSequence(anchorNodeSequenceWithSelectedItems);
                sequenceChanged = true;
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
        } else if (command & SelectionModel::SetCurrentItem) {
            setCurrentItem(nullptr);
        }

        if (selectionChanged) {
            Q_EMIT q_ptr->selectedItemsChanged();
            if (oldCount != selectedItems.size()) {
                Q_EMIT q_ptr->selectedCountChanged();
            }
        }
        if (sequenceChanged) {
            Q_EMIT q_ptr->anchorNodeSequenceWithSelectedItemsChanged();
        }
    }

    AnchorNodeSelectionModel::AnchorNodeSelectionModel(QObject *parent) : QObject(parent), d_ptr(new AnchorNodeSelectionModelPrivate) {
        Q_D(AnchorNodeSelectionModel);
        d->q_ptr = this;
        d->selectionModel = qobject_cast<SelectionModel *>(parent);
    }

    AnchorNodeSelectionModel::~AnchorNodeSelectionModel() = default;

    AnchorNode *AnchorNodeSelectionModel::currentItem() const {
        Q_D(const AnchorNodeSelectionModel);
        return d->currentItem;
    }

    QList<AnchorNode *> AnchorNodeSelectionModel::selectedItems() const {
        Q_D(const AnchorNodeSelectionModel);
        return d->selectedItems.values();
    }

    int AnchorNodeSelectionModel::selectedCount() const {
        Q_D(const AnchorNodeSelectionModel);
        return d->selectedItems.size();
    }

    AnchorNodeSequence *AnchorNodeSelectionModel::anchorNodeSequenceWithSelectedItems() const {
        Q_D(const AnchorNodeSelectionModel);
        return d->anchorNodeSequenceWithSelectedItems;
    }

    bool AnchorNodeSelectionModel::isItemSelected(AnchorNode *item) const {
        Q_D(const AnchorNodeSelectionModel);
        return d->selectedItems.contains(item);
    }

}

#include "moc_AnchorNodeSelectionModel.cpp"
