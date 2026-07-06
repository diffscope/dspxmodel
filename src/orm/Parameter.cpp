#include "Parameter.h"
#include "Parameter_p.h"

#include <utility>

#include <dspxmodelCore/Schema.h>
#include <opendspx/param.h>
#include <dspxmodelORM/AnchorNodeSequence.h>
#include <dspxmodelORM/FreeValueDataArray.h>
#include <dspxmodelORM/ParameterMap.h>
#include <dspxmodelORM/SingingClip.h>
#include <dspxmodelORM/private/AnchorNodeSequence_p.h>
#include <dspxmodelORM/private/FreeValueDataArray_p.h>
#include <dspxmodelORM/private/Model_p.h>
#include <dspxmodelORM/private/ORMBinding_p.h>
#include <dspxmodelORM/private/ORMUtils_p.h>
#include <dspxmodelORM/private/ParameterMap_p.h>

namespace dspx {

    namespace {

        QString parameterKeyFromSnapshot(const dini::ItemSnapshot &snapshot) {
            const auto &value = orm::snapshotValue(snapshot, Schema::parameterKeyColumn());
            return value.isNull() ? QString() : orm::stringFromValue(value);
        }

        ParameterMap *parameterOwnerFromPlacement(ModelPrivate &model, Handle singingClipHandle, bool hasKey) {
            if (!singingClipHandle || !hasKey) {
                return nullptr;
            }
            auto *singingClip = model.ensure<SingingClip>(singingClipHandle);
            return singingClip ? singingClip->parameters() : nullptr;
        }

        ParameterMap *parameterOwnerFromSnapshot(ModelPrivate &model, const dini::ItemSnapshot &snapshot) {
            return parameterOwnerFromPlacement(
                model,
                orm::handleFromValue(orm::snapshotValue(snapshot, Schema::parameterParent().column())),
                !orm::snapshotValue(snapshot, Schema::parameterKeyColumn()).isNull());
        }

    }

    namespace orm {

        const TableBinding &parameterTableBinding() {
            static const TableBinding binding = makeKeyedAssociatedTableBinding<Parameter, ParameterMap>({
                .table = Schema::parameterTable(),
                .associationColumn = Schema::parameterParent().column(),
                .keyColumn = Schema::parameterKeyColumn(),
                .ensure = [](ModelPrivate &model, const dini::ItemSnapshot &snapshot) { return model.ensure<Parameter>(snapshot); },
                .find = [](ModelPrivate &model, Handle handle) { return model.find<Parameter>(handle); },
                .removeObject = [](ModelPrivate &model, Handle handle) { model.parameterObjects.remove(handle); },
                .sync = [](Parameter *item, const dini::ItemSnapshot &snapshot, bool notify) { syncParameterColumns(item, snapshot, notify); },
                .applyColumn = [](Parameter *item, const dini::ColumnHandle &column, const dini::Value &value, bool notify) { return applyParameterColumn(item, column, value, notify); },
                .ownerForSnapshot = [](ModelPrivate &model, const dini::ItemSnapshot &snapshot) {
                    return parameterOwnerFromSnapshot(model, snapshot);
                },
                .keyForSnapshot = [](const dini::ItemSnapshot &snapshot) {
                    return parameterKeyFromSnapshot(snapshot);
                },
                .setPlacement = [](Parameter *item, ParameterMap *owner, QString key, bool notify) {
                    ParameterPrivate::get(item)->setPlacement(owner, std::move(key), notify);
                },
                .refreshOwner = [](ParameterMap *owner, bool notify) { ParameterMapPrivate::get(owner)->refresh(notify); },
                .itemAboutToInsert = [](ParameterMap *owner, const QString &key, Parameter *item, ParameterMap *movedFrom) {
                    emit owner->itemAboutToInsert(key, item, movedFrom);
                },
                .itemInserted = [](ParameterMap *owner, const QString &key, Parameter *item, ParameterMap *movedFrom) {
                    emit owner->itemInserted(key, item, movedFrom);
                },
                .itemAboutToRemove = [](ParameterMap *owner, const QString &key, Parameter *item, ParameterMap *movedTo) {
                    emit owner->itemAboutToRemove(key, item, movedTo);
                },
                .itemRemoved = [](ParameterMap *owner, const QString &key, Parameter *item, ParameterMap *movedTo) {
                    emit owner->itemRemoved(key, item, movedTo);
                },
            });
            return binding;
        }

        void syncParameterColumns(Parameter *item, const dini::ItemSnapshot &snapshot, bool notify) {
            auto *modelData = ModelPrivate::get(item->model());
            ParameterPrivate::get(item)->setPlacement(
                parameterOwnerFromSnapshot(*modelData, snapshot),
                parameterKeyFromSnapshot(snapshot),
                notify);
        }

        bool applyParameterColumn(Parameter *item, const dini::ColumnHandle &column, const dini::Value &value, bool notify) {
            if (column != Schema::parameterParent().column() && column != Schema::parameterKeyColumn()) {
                return false;
            }

            auto *d = ParameterPrivate::get(item);
            auto *modelData = ModelPrivate::get(item->model());
            const bool keyUpdated = column == Schema::parameterKeyColumn();
            const bool hasKey = keyUpdated ? !value.isNull() : d->parameterMap != nullptr;
            const auto key = keyUpdated ? (value.isNull() ? QString() : stringFromValue(value)) : d->key;
            const auto singingClipHandle = column == Schema::parameterParent().column()
                                               ? handleFromValue(value)
                                               : (d->parameterMap ? d->parameterMap->singingClip()->handle() : Handle {});
            d->setPlacement(parameterOwnerFromPlacement(*modelData, singingClipHandle, hasKey), key, notify);
            return true;
        }

    }

    ParameterPrivate::ParameterPrivate(Parameter *q) : q_ptr(q) {
    }

    void ParameterPrivate::setPlacement(ParameterMap *newParameterMap, QString newKey, bool notify) {
        Q_Q(Parameter);
        const bool mapChanged = parameterMap != newParameterMap;
        parameterMap = newParameterMap;
        key = std::move(newKey);
        if (notify && mapChanged) {
            emit q->parameterMapChanged(parameterMap);
        }
    }

    Parameter::Parameter(Handle handle, Model *model) : EntityObject(handle, model, model), d_ptr(new ParameterPrivate(this)) {
        Q_D(Parameter);
        d->original = FreeValueDataArrayPrivate::create(this, FreeValueDataArray::Original);
        d->freeTransform = FreeValueDataArrayPrivate::create(this, FreeValueDataArray::Transform);
        d->freeEdited = FreeValueDataArrayPrivate::create(this, FreeValueDataArray::Edited);
        d->anchorTransform = AnchorNodeSequencePrivate::create(this, AnchorNodeSequence::Transform);
        d->anchorEdited = AnchorNodeSequencePrivate::create(this, AnchorNodeSequence::Edited);
        FreeValueDataArrayPrivate::get(d->original)->refresh(false, false);
        FreeValueDataArrayPrivate::get(d->freeTransform)->refresh(false, false);
        FreeValueDataArrayPrivate::get(d->freeEdited)->refresh(false, false);
        AnchorNodeSequencePrivate::get(d->anchorTransform)->refresh(false);
        AnchorNodeSequencePrivate::get(d->anchorEdited)->refresh(false);
    }

    Parameter::~Parameter() = default;

    FreeValueDataArray *Parameter::original() const {
        Q_D(const Parameter);
        return d->original;
    }

    FreeValueDataArray *Parameter::freeTransform() const {
        Q_D(const Parameter);
        return d->freeTransform;
    }

    FreeValueDataArray *Parameter::freeEdited() const {
        Q_D(const Parameter);
        return d->freeEdited;
    }

    AnchorNodeSequence *Parameter::anchorTransform() const {
        Q_D(const Parameter);
        return d->anchorTransform;
    }

    AnchorNodeSequence *Parameter::anchorEdited() const {
        Q_D(const Parameter);
        return d->anchorEdited;
    }

    ParameterMap *Parameter::parameterMap() const {
        Q_D(const Parameter);
        return d->parameterMap;
    }

    opendspx::Param Parameter::toOpenDSPX() const {
        return {};
    }

    void Parameter::fromOpenDSPX(const opendspx::Param &) {
    }

}


#include "moc_Parameter.cpp"
