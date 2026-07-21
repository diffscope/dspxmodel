#include "DynamicMixingAnchorSelectionModel.h"
#include "DynamicMixingAnchorSelectionModel_p.h"

#include <dspxmodelORM/DynamicMixingAnchor.h>
#include <dspxmodelORM/DynamicMixingAnchorSequence.h>
#include <dspxmodelORM/Sources.h>
#include <dspxmodelSelectionModel/SelectionModel.h>

namespace dspx {

    bool DynamicMixingAnchorSelectionModelPrivate::isValidItem(DynamicMixingAnchor *item) const {
        return item && item->model() == selectionModel->model() && item->dynamicMixingAnchorSequence();
    }

    bool DynamicMixingAnchorSelectionModelPrivate::isValidSequence(DynamicMixingAnchorSequence *sequence) const {
        return sequence && sequence->sources() && sequence->sources()->model() == selectionModel->model();
    }

    void DynamicMixingAnchorSelectionModelPrivate::connectItem(DynamicMixingAnchor *item) {
        if (connectedItems.contains(item))
            return;
        QObject::connect(item, &QObject::destroyed, q_ptr, [this](QObject *object) {
            dropItem(static_cast<DynamicMixingAnchor *>(object));
        });
        QObject::connect(item, &DynamicMixingAnchor::dynamicMixingAnchorSequenceChanged, q_ptr, [this, item] {
            if (item->dynamicMixingAnchorSequence() != dynamicMixingAnchorSequenceWithSelectedItems)
                dropItem(item);
        });
        connectedItems.insert(item);
    }

    void DynamicMixingAnchorSelectionModelPrivate::disconnectItem(DynamicMixingAnchor *item) {
        QObject::disconnect(item, nullptr, q_ptr, nullptr);
        connectedItems.remove(item);
    }

    bool DynamicMixingAnchorSelectionModelPrivate::addToSelection(DynamicMixingAnchor *item) {
        if (!isValidItem(item) || selectedItems.contains(item))
            return false;
        connectItem(item);
        selectedItems.insert(item);
        Q_EMIT q_ptr->itemSelected(item, true);
        return true;
    }

    bool DynamicMixingAnchorSelectionModelPrivate::removeFromSelection(DynamicMixingAnchor *item) {
        if (!item || !selectedItems.remove(item))
            return false;
        if (item != currentItem)
            disconnectItem(item);
        Q_EMIT q_ptr->itemSelected(item, false);
        return true;
    }

    bool DynamicMixingAnchorSelectionModelPrivate::clearSelection() {
        if (selectedItems.isEmpty())
            return false;
        const auto items = selectedItems.values();
        bool changed = false;
        for (auto *item : items)
            changed |= removeFromSelection(item);
        return changed;
    }

    void DynamicMixingAnchorSelectionModelPrivate::dropItem(DynamicMixingAnchor *item) {
        if (!item)
            return;
        const int oldCount = selectedItems.size();
        const bool selectionChanged = removeFromSelection(item);
        bool currentChanged = false;
        if (currentItem == item) {
            if (!selectedItems.contains(item))
                disconnectItem(item);
            currentItem = nullptr;
            currentChanged = true;
        }
        if (selectionChanged) {
            Q_EMIT q_ptr->selectedItemsChanged();
            if (oldCount != selectedItems.size())
                Q_EMIT q_ptr->selectedCountChanged();
        }
        if (currentChanged)
            Q_EMIT q_ptr->currentItemChanged();
    }

    void DynamicMixingAnchorSelectionModelPrivate::setCurrentItem(DynamicMixingAnchor *item) {
        if (!isValidItem(item))
            item = nullptr;
        if (currentItem == item)
            return;
        auto *oldItem = currentItem;
        currentItem = item;
        if (oldItem && !selectedItems.contains(oldItem))
            disconnectItem(oldItem);
        if (currentItem && !selectedItems.contains(currentItem))
            connectItem(currentItem);
        Q_EMIT q_ptr->currentItemChanged();
    }

    void DynamicMixingAnchorSelectionModelPrivate::connectSequence(DynamicMixingAnchorSequence *sequence) {
        if (!sequence)
            return;
        QObject::disconnect(sequenceDestroyedConnection);
        sequenceDestroyedConnection = QObject::connect(sequence, &QObject::destroyed, q_ptr, [this] {
            clearAllAndResetSequence();
        });
    }

    void DynamicMixingAnchorSelectionModelPrivate::disconnectSequence() {
        QObject::disconnect(sequenceDestroyedConnection);
        sequenceDestroyedConnection = {};
    }

    void DynamicMixingAnchorSelectionModelPrivate::clearAllAndResetSequence() {
        const int oldCount = selectedItems.size();
        const bool selectionChanged = clearSelection();
        setCurrentItem(nullptr);
        const bool sequenceChanged = dynamicMixingAnchorSequenceWithSelectedItems != nullptr;
        disconnectSequence();
        dynamicMixingAnchorSequenceWithSelectedItems = nullptr;
        if (selectionChanged) {
            Q_EMIT q_ptr->selectedItemsChanged();
            if (oldCount != selectedItems.size())
                Q_EMIT q_ptr->selectedCountChanged();
        }
        if (sequenceChanged)
            Q_EMIT q_ptr->dynamicMixingAnchorSequenceWithSelectedItemsChanged();
    }

    void DynamicMixingAnchorSelectionModelPrivate::select(
        DynamicMixingAnchor *item, SelectionModel::SelectionCommand command,
        DynamicMixingAnchorSequence *containerItemHint) {
        const int oldCount = selectedItems.size();
        bool selectionChanged = false;
        bool sequenceChanged = false;

        if (command & SelectionModel::ClearPreviousSelection)
            selectionChanged |= clearSelection();

        if (!item && containerItemHint && isValidSequence(containerItemHint)
            && dynamicMixingAnchorSequenceWithSelectedItems != containerItemHint) {
            selectionChanged |= clearSelection();
            if (currentItem && currentItem->dynamicMixingAnchorSequence() != containerItemHint)
                setCurrentItem(nullptr);
            disconnectSequence();
            dynamicMixingAnchorSequenceWithSelectedItems = containerItemHint;
            connectSequence(containerItemHint);
            sequenceChanged = true;
        }

        if (item && isValidItem(item)) {
            auto *itemSequence = item->dynamicMixingAnchorSequence();
            if (!dynamicMixingAnchorSequenceWithSelectedItems) {
                dynamicMixingAnchorSequenceWithSelectedItems = itemSequence;
                connectSequence(itemSequence);
                sequenceChanged = true;
            } else if (dynamicMixingAnchorSequenceWithSelectedItems != itemSequence) {
                selectionChanged |= clearSelection();
                setCurrentItem(nullptr);
                disconnectSequence();
                dynamicMixingAnchorSequenceWithSelectedItems = itemSequence;
                connectSequence(itemSequence);
                sequenceChanged = true;
            }

            if ((command & SelectionModel::Select) && (command & SelectionModel::Deselect)) {
                selectionChanged |= selectedItems.contains(item) ? removeFromSelection(item)
                                                                  : addToSelection(item);
            } else if (command & SelectionModel::Select) {
                selectionChanged |= addToSelection(item);
            } else if (command & SelectionModel::Deselect) {
                selectionChanged |= removeFromSelection(item);
            }
            if (command & SelectionModel::SetCurrentItem)
                setCurrentItem(item);
        } else if (command & SelectionModel::SetCurrentItem) {
            setCurrentItem(nullptr);
        }

        if (selectionChanged) {
            Q_EMIT q_ptr->selectedItemsChanged();
            if (oldCount != selectedItems.size())
                Q_EMIT q_ptr->selectedCountChanged();
        }
        if (sequenceChanged)
            Q_EMIT q_ptr->dynamicMixingAnchorSequenceWithSelectedItemsChanged();
    }

    DynamicMixingAnchorSelectionModel::DynamicMixingAnchorSelectionModel(QObject *parent)
        : QObject(parent), d_ptr(new DynamicMixingAnchorSelectionModelPrivate) {
        Q_D(DynamicMixingAnchorSelectionModel);
        d->q_ptr = this;
        d->selectionModel = qobject_cast<SelectionModel *>(parent);
    }

    DynamicMixingAnchorSelectionModel::~DynamicMixingAnchorSelectionModel() = default;

    DynamicMixingAnchor *DynamicMixingAnchorSelectionModel::currentItem() const {
        Q_D(const DynamicMixingAnchorSelectionModel);
        return d->currentItem;
    }

    QList<DynamicMixingAnchor *> DynamicMixingAnchorSelectionModel::selectedItems() const {
        Q_D(const DynamicMixingAnchorSelectionModel);
        return d->selectedItems.values();
    }

    int DynamicMixingAnchorSelectionModel::selectedCount() const {
        Q_D(const DynamicMixingAnchorSelectionModel);
        return d->selectedItems.size();
    }

    DynamicMixingAnchorSequence *DynamicMixingAnchorSelectionModel::dynamicMixingAnchorSequenceWithSelectedItems() const {
        Q_D(const DynamicMixingAnchorSelectionModel);
        return d->dynamicMixingAnchorSequenceWithSelectedItems;
    }

    bool DynamicMixingAnchorSelectionModel::isItemSelected(DynamicMixingAnchor *item) const {
        Q_D(const DynamicMixingAnchorSelectionModel);
        return d->selectedItems.contains(item);
    }

}

#include "moc_DynamicMixingAnchorSelectionModel.cpp"
