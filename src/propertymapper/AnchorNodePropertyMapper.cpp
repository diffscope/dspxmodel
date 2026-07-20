#include "AnchorNodePropertyMapper.h"
#include "AnchorNodePropertyMapper_p.h"

#include <dspxmodelSelectionModel/AnchorNodeSelectionModel.h>
#include <dspxmodelSelectionModel/SelectionModel.h>

namespace dspx {

    AnchorNodePropertyMapper::AnchorNodePropertyMapper(QObject *parent)
        : QObject(parent), d_ptr(new AnchorNodePropertyMapperPrivate) {
        Q_D(AnchorNodePropertyMapper);
        d->q_ptr = this;
    }

    AnchorNodePropertyMapper::~AnchorNodePropertyMapper() = default;

    SelectionModel *AnchorNodePropertyMapper::selectionModel() const {
        Q_D(const AnchorNodePropertyMapper);
        return d->selectionModel;
    }

    void AnchorNodePropertyMapper::setSelectionModel(SelectionModel *selectionModel) {
        Q_D(AnchorNodePropertyMapper);
        if (d->selectionModel == selectionModel) {
            return;
        }
        d->setSelectionModel(selectionModel);
        Q_EMIT selectionModelChanged();
    }

    QVariant AnchorNodePropertyMapper::position() const {
        Q_D(const AnchorNodePropertyMapper);
        return d->value<AnchorNodePropertyMapperPrivate::PositionProperty>();
    }

    QVariant AnchorNodePropertyMapper::value() const {
        Q_D(const AnchorNodePropertyMapper);
        return d->value<AnchorNodePropertyMapperPrivate::ValueProperty>();
    }

    void AnchorNodePropertyMapper::setValue(const QVariant &value) {
        Q_D(AnchorNodePropertyMapper);
        d->setValue<AnchorNodePropertyMapperPrivate::ValueProperty>(value);
    }

    QVariant AnchorNodePropertyMapper::interpolationMode() const {
        Q_D(const AnchorNodePropertyMapper);
        return d->value<AnchorNodePropertyMapperPrivate::InterpolationModeProperty>();
    }

    void AnchorNodePropertyMapper::setInterpolationMode(const QVariant &interpolationMode) {
        Q_D(AnchorNodePropertyMapper);
        d->setValue<AnchorNodePropertyMapperPrivate::InterpolationModeProperty>(interpolationMode);
    }

    void AnchorNodePropertyMapperPrivate::setSelectionModel(SelectionModel *selectionModel_) {
        if (selectionModel == selectionModel_) {
            return;
        }
        detachSelectionModel();
        selectionModel = selectionModel_;
        attachSelectionModel();
        refreshCache();
    }

    void AnchorNodePropertyMapperPrivate::attachSelectionModel() {
        Q_Q(AnchorNodePropertyMapper);
        if (!selectionModel) {
            return;
        }
        anchorNodeSelectionModel = selectionModel->anchorNodeSelectionModel();
        QObject::connect(anchorNodeSelectionModel, &AnchorNodeSelectionModel::itemSelected, q,
                         [this](AnchorNode *item, bool selected) {
            handleItemSelected(item, selected);
        });
        for (auto *item : anchorNodeSelectionModel->selectedItems()) {
            addItem(item);
        }
        refreshCache();
    }

    void AnchorNodePropertyMapperPrivate::detachSelectionModel() {
        if (anchorNodeSelectionModel) {
            QObject::disconnect(anchorNodeSelectionModel, nullptr, q_ptr, nullptr);
        }
        clear();
        anchorNodeSelectionModel = nullptr;
        selectionModel = nullptr;
    }
}

#include "moc_AnchorNodePropertyMapper.cpp"
