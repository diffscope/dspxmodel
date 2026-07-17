#include "Parameter.h"
#include "Parameter_p.h"

#include <algorithm>
#include <cmath>
#include <memory>
#include <unordered_set>
#include <utility>
#include <vector>

#include <dspxmodelCore/Schema.h>
#include <opendspx/anchornode.h>
#include <opendspx/param.h>
#include <opendspx/paramcurveanchor.h>
#include <opendspx/paramcurvefree.h>
#include <opendspxinterpolator/interpolator.h>
#include <dspxmodelORM/AnchorNode.h>
#include <dspxmodelORM/AnchorNodeSequence.h>
#include <dspxmodelORM/FreeValueDataArray.h>
#include <dspxmodelORM/Model.h>
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

        constexpr int freeValueStep = FreeValueDataArray::step();

        std::vector<opendspx::ParamCurveRef> freeCurvesToOpenDSPX(const FreeValueDataArray *array) {
            std::vector<opendspx::ParamCurveRef> result;
            const auto source = array->items();
            std::vector<int> values;
            int start = 0;
            for (int i = 0; i < source.size(); ++i) {
                const auto &value = source.at(i);
                if (value.isValid()) {
                    if (values.empty()) {
                        start = i * freeValueStep;
                    }
                    values.push_back(value.toInt());
                    continue;
                }
                if (!values.empty()) {
                    result.push_back(std::make_shared<opendspx::ParamCurveFree>(start, freeValueStep, std::move(values)));
                    values = {};
                }
            }
            if (!values.empty()) {
                result.push_back(std::make_shared<opendspx::ParamCurveFree>(start, freeValueStep, std::move(values)));
            }
            return result;
        }

        opendspx::AnchorNode anchorNodeToOpenDSPX(const AnchorNode *node, int start) {
            auto target = node->toOpenDSPX();
            target.x -= start;
            return target;
        }

        void appendAnchorSegment(std::vector<opendspx::ParamCurveRef> &result, const std::vector<AnchorNode *> &nodes) {
            if (nodes.empty()) {
                return;
            }
            const int start = nodes.front()->x();
            std::vector<opendspx::AnchorNode> targetNodes;
            targetNodes.reserve(nodes.size());
            for (const auto *node : nodes) {
                targetNodes.push_back(anchorNodeToOpenDSPX(node, start));
            }
            targetNodes.back().interp = opendspx::AnchorNode::Interpolation::None;
            result.push_back(std::make_shared<opendspx::ParamCurveAnchor>(start, std::move(targetNodes)));
        }

        std::vector<opendspx::ParamCurveRef> anchorCurvesToOpenDSPX(const AnchorNodeSequence *sequence) {
            std::vector<opendspx::ParamCurveRef> result;
            if (!sequence->lastItem()) {
                return result;
            }
            const auto source = sequence->slice(0, sequence->lastItem()->x() + 1);
            std::vector<AnchorNode *> current;
            for (auto *node : source) {
                current.push_back(node);
                if (node->interpolationMode() == AnchorNode::None) {
                    appendAnchorSegment(result, current);
                    current.clear();
                }
            }
            appendAnchorSegment(result, current);
            return result;
        }

        void appendCurves(std::vector<opendspx::ParamCurveRef> &target, std::vector<opendspx::ParamCurveRef> curves) {
            target.reserve(target.size() + curves.size());
            for (auto &curve : curves) {
                target.push_back(std::move(curve));
            }
        }

        void setFreeValue(QList<QVariant> &values, int index, int value) {
            if (index >= values.size()) {
                values.resize(index + 1);
            }
            values[index] = value;
        }

        QList<QVariant> freeValuesFromOpenDSPX(const std::vector<opendspx::ParamCurveRef> &curves) {
            QList<QVariant> result;
            for (const auto &curveRef : curves) {
                if (curveRef->type != opendspx::ParamCurve::Free) {
                    continue;
                }
                const auto &curve = static_cast<const opendspx::ParamCurveFree &>(*curveRef);
                const int startIndex = curve.start / freeValueStep;
                if (startIndex + static_cast<int>(curve.values.size()) > result.size()) {
                    result.resize(startIndex + static_cast<int>(curve.values.size()));
                }
                for (int i = 0; i < static_cast<int>(curve.values.size()); ++i) {
                    result[startIndex + i] = curve.values[static_cast<std::size_t>(i)];
                }
            }
            return result;
        }

        struct AbsoluteAnchorNode {
            opendspx::AnchorNode::Interpolation interp = opendspx::AnchorNode::Interpolation::None;
            int x = 0;
            int y = 0;
        };

        std::vector<AbsoluteAnchorNode> absoluteAnchorNodes(const opendspx::ParamCurveAnchor &curve) {
            std::vector<AbsoluteAnchorNode> result;
            result.reserve(curve.nodes.size());
            for (std::size_t i = 0; i < curve.nodes.size(); ++i) {
                const auto &source = curve.nodes[i];
                result.push_back({
                    .interp = i + 1 == curve.nodes.size() ? opendspx::AnchorNode::Interpolation::None : source.interp,
                    .x = curve.start + source.x,
                    .y = source.y,
                });
            }
            return result;
        }

        void clearFreeValues(FreeValueDataArray *array) {
            array->splice(0, array->size(), {});
        }

        void clearAnchorNodes(AnchorNodeSequence *sequence) {
            while (auto *node = sequence->firstItem()) {
                sequence->removeItem(node);
            }
        }

        void applyFreeValues(FreeValueDataArray *array, const QList<QVariant> &values) {
            array->splice(0, array->size(), values);
        }

        void importAnchorCurves(AnchorNodeSequence *sequence, const std::vector<opendspx::ParamCurveRef> &curves, Model *model) {
            std::unordered_set<int> insertedX;
            for (const auto &curveRef : curves) {
                if (curveRef->type != opendspx::ParamCurve::Anchor) {
                    continue;
                }
                const auto &curve = static_cast<const opendspx::ParamCurveAnchor &>(*curveRef);
                const auto nodes = absoluteAnchorNodes(curve);
                for (const auto &source : nodes) {
                    if (insertedX.contains(source.x)) {
                        continue;
                    }
                    opendspx::AnchorNode target;
                    target.interp = source.interp;
                    target.x = source.x;
                    target.y = source.y;
                    auto *node = model->createAnchorNode();
                    node->fromOpenDSPX(target);
                    if (sequence->insertItem(node)) {
                        insertedX.insert(source.x);
                    }
                }
            }
        }

        opendspx::Interpolator<double> hermiteInterpolator(const std::vector<AbsoluteAnchorNode> &nodes, std::size_t index) {
            const auto &left = nodes[index];
            const auto &right = nodes[index + 1];
            const bool hasPrevious = index > 0;
            const bool hasNext = index + 2 < nodes.size();
            if (hasPrevious && hasNext) {
                const auto &previous = nodes[index - 1];
                const auto &next = nodes[index + 2];
                return opendspx::Interpolator<double>::create(
                    left.x, left.y, right.x, right.y, previous.x, previous.y, next.x, next.y);
            }
            if (hasPrevious) {
                const auto &previous = nodes[index - 1];
                return opendspx::Interpolator<double>::createWithRef1Only(
                    left.x, left.y, right.x, right.y, previous.x, previous.y);
            }
            if (hasNext) {
                const auto &next = nodes[index + 2];
                return opendspx::Interpolator<double>::createWithRef2Only(
                    left.x, left.y, right.x, right.y, next.x, next.y);
            }
            return opendspx::Interpolator<double>::createLinear(left.x, left.y, right.x, right.y);
        }

        opendspx::Interpolator<double> interpolatorForSegment(const std::vector<AbsoluteAnchorNode> &nodes, std::size_t index) {
            const auto &left = nodes[index];
            const auto &right = nodes[index + 1];
            if (left.interp == opendspx::AnchorNode::Interpolation::Hermite) {
                return hermiteInterpolator(nodes, index);
            }
            return opendspx::Interpolator<double>::createLinear(left.x, left.y, right.x, right.y);
        }

        int ceilToStepIndex(int x) {
            return (x + freeValueStep - 1) / freeValueStep;
        }

        int floorToStepIndex(int x) {
            return x / freeValueStep;
        }

        int roundToStepIndex(int x) {
            return (x + freeValueStep / 2) / freeValueStep;
        }

        void applyAnchorCurveToFreeValues(QList<QVariant> &values, const opendspx::ParamCurveAnchor &curve) {
            const auto nodes = absoluteAnchorNodes(curve);
            if (nodes.empty()) {
                return;
            }
            if (nodes.size() == 1) {
                setFreeValue(values, roundToStepIndex(nodes.front().x), nodes.front().y);
                return;
            }
            for (std::size_t i = 0; i + 1 < nodes.size(); ++i) {
                const auto &left = nodes[i];
                const auto &right = nodes[i + 1];
                if (left.interp == opendspx::AnchorNode::Interpolation::None) {
                    setFreeValue(values, roundToStepIndex(left.x), left.y);
                    continue;
                }
                const auto interpolator = interpolatorForSegment(nodes, i);
                const int startIndex = ceilToStepIndex(left.x);
                const int endIndex = floorToStepIndex(right.x);
                for (int index = startIndex; index <= endIndex; ++index) {
                    const int x = index * freeValueStep;
                    setFreeValue(values, index, static_cast<int>(std::lround(interpolator.evaluate(x))));
                }
            }
            setFreeValue(values, roundToStepIndex(nodes.back().x), nodes.back().y);
        }

        void applyAnchorCurvesToFreeValues(QList<QVariant> &values, const std::vector<opendspx::ParamCurveRef> &curves) {
            for (const auto &curveRef : curves) {
                if (curveRef->type != opendspx::ParamCurve::Anchor) {
                    continue;
                }
                applyAnchorCurveToFreeValues(values, static_cast<const opendspx::ParamCurveAnchor &>(*curveRef));
            }
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
                .placementChangedAfterCommit = [](Parameter *item, ParameterMap *owner) {
                    emit item->parameterMapChangedAfterCommit(owner);
                },
                .refreshOwner = [](ParameterMap *owner, bool notify) { ParameterMapPrivate::get(owner)->refresh(notify); },
                .refreshOwnerAfterCommit = [](ParameterMap *owner, bool sizeChanged, bool itemsChanged) {
                    if (sizeChanged) emit owner->sizeChangedAfterCommit(owner->size());
                    if (itemsChanged) {
                        emit owner->keysChangedAfterCommit();
                        emit owner->itemsChangedAfterCommit();
                    }
                },
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
                .itemAboutToInsertAfterCommit = [](ParameterMap *owner, const QString &key, Parameter *item, ParameterMap *movedFrom) {
                    emit owner->itemAboutToInsertAfterCommit(key, item, movedFrom);
                },
                .itemInsertedAfterCommit = [](ParameterMap *owner, const QString &key, Parameter *item, ParameterMap *movedFrom) {
                    emit owner->itemInsertedAfterCommit(key, item, movedFrom);
                },
                .itemAboutToRemoveAfterCommit = [](ParameterMap *owner, const QString &key, Parameter *item, ParameterMap *movedTo) {
                    emit owner->itemAboutToRemoveAfterCommit(key, item, movedTo);
                },
                .itemRemovedAfterCommit = [](ParameterMap *owner, const QString &key, Parameter *item, ParameterMap *movedTo) {
                    emit owner->itemRemovedAfterCommit(key, item, movedTo);
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
            if (currentNotificationStage == NotificationStage::AfterCommit) {
                return true;
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
        opendspx::Param target;
        target.original = freeCurvesToOpenDSPX(original());
        target.transform = freeCurvesToOpenDSPX(freeTransform());
        appendCurves(target.transform, anchorCurvesToOpenDSPX(anchorTransform()));
        target.edited = freeCurvesToOpenDSPX(freeEdited());
        appendCurves(target.edited, anchorCurvesToOpenDSPX(anchorEdited()));
        return target;
    }

    void Parameter::fromOpenDSPX(const opendspx::Param &parameter) {
        clearFreeValues(original());
        clearFreeValues(freeTransform());
        clearFreeValues(freeEdited());
        clearAnchorNodes(anchorTransform());
        clearAnchorNodes(anchorEdited());

        auto originalValues = freeValuesFromOpenDSPX(parameter.original);
        applyAnchorCurvesToFreeValues(originalValues, parameter.original);
        applyFreeValues(original(), originalValues);

        applyFreeValues(freeTransform(), freeValuesFromOpenDSPX(parameter.transform));
        applyFreeValues(freeEdited(), freeValuesFromOpenDSPX(parameter.edited));
        importAnchorCurves(anchorTransform(), parameter.transform, model());
        importAnchorCurves(anchorEdited(), parameter.edited, model());
    }

}


#include "moc_Parameter.cpp"
