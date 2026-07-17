#include "VibratoPointDataArray.h"
#include "VibratoPointDataArray_p.h"

#include <cstdint>
#include <vector>

#include <dini/engine.h>
#include <dini/transaction.h>
#include <opendspx/controlpoint.h>

#include <dspxmodelCore/Schema.h>
#include <dspxmodelORM/Model.h>
#include <dspxmodelORM/Note.h>
#include <dspxmodelORM/private/Model_p.h>
#include <dspxmodelORM/private/ORMBinding_p.h>
#include <dspxmodelORM/private/ORMUtils_p.h>

namespace dspx {

    namespace {

        dini::QuerySpec vibratoPointRelationQuery(Handle noteHandle, VibratoPointDataArray::VibratoPointRole role) {
            return dini::QuerySpec {
                .filter = dini::FilterExpression::all({
                    orm::parentFilter(Schema::noteVibratoPointRelationParent(), noteHandle),
                    orm::equalFilter(dini::FieldRef::column(Schema::noteVibratoPointRelationRoleColumn()),
                                     dini::Value(static_cast<std::int64_t>(role))),
                }),
            };
        }

        dini::QuerySpec vibratoPointQuery(Handle relationHandle) {
            return dini::QuerySpec {
                .filter = orm::parentFilter(Schema::vibratoPointParent(), relationHandle),
            };
        }

        QPointF pointFromSnapshot(const dini::ItemSnapshot &snapshot) {
            return QPointF(orm::snapshotValue(snapshot, Schema::vibratoPointXColumn()).asDouble(),
                           orm::snapshotValue(snapshot, Schema::vibratoPointYColumn()).asDouble());
        }

        QList<QPointF> pointsFromView(const dini::View &view) {
            QList<QPointF> result;
            for (const auto &snapshot : view.toVector()) {
                result.append(pointFromSnapshot(snapshot));
            }
            return result;
        }

        std::vector<dini::ColumnValue> pointValues(const QPointF &point) {
            return {
                dini::ColumnValue {.column = Schema::vibratoPointXColumn(), .value = dini::Value(point.x())},
                dini::ColumnValue {.column = Schema::vibratoPointYColumn(), .value = dini::Value(point.y())},
            };
        }

        VibratoPointDataArray *vibratoPointOwnerFromAssociationValue(ModelPrivate &model, const dini::Value &value) {
            const auto relationHandle = orm::handleFromValue(value);
            if (!relationHandle || !model.engine->contains(orm::idFromHandle(relationHandle))) {
                return nullptr;
            }
            const auto relation = model.engine->read(orm::idFromHandle(relationHandle));
            if (!orm::isContainer(relation, Schema::noteVibratoPointRelationTable())) {
                return nullptr;
            }
            const auto noteHandle = orm::handleFromValue(orm::snapshotValue(relation, Schema::noteVibratoPointRelationParent().column()));
            const auto roleValue = orm::snapshotValue(relation, Schema::noteVibratoPointRelationRoleColumn());
            if (!noteHandle || roleValue.isNull()) {
                return nullptr;
            }
            auto *note = model.ensure<Note>(noteHandle);
            if (!note) {
                return nullptr;
            }
            const auto role = static_cast<VibratoPointDataArray::VibratoPointRole>(roleValue.asInt64());
            if (role == VibratoPointDataArray::Amplitude) {
                return note->vibratoAmplitudeControlPoints();
            }
            if (role == VibratoPointDataArray::Frequency) {
                return note->vibratoFrequencyControlPoints();
            }
            return nullptr;
        }

    }

    namespace orm {

        const ListBinding &vibratoPointDataArrayBinding() {
            static const ListBinding binding = makeDataArrayListBinding<VibratoPointDataArray, QPointF>({
                .list = Schema::vibratoPointList(),
                .associationColumn = Schema::vibratoPointParent().column(),
                .ownerForAssociationValue = [](ModelPrivate &model, const dini::Value &value) {
                    return vibratoPointOwnerFromAssociationValue(model, value);
                },
                .refreshOwner = [](VibratoPointDataArray *owner, bool notify, bool itemsChanged) {
                    VibratoPointDataArrayPrivate::get(owner)->refresh(notify, itemsChanged);
                },
                .refreshOwnerAfterCommit = [](VibratoPointDataArray *owner, bool sizeChanged, bool itemsChanged) {
                    if (sizeChanged) emit owner->sizeChangedAfterCommit(owner->size());
                    if (itemsChanged) emit owner->itemsChangedAfterCommit();
                },
                .decodeItem = [](const dini::ItemSnapshot &snapshot) {
                    return pointFromSnapshot(snapshot);
                },
                .notificationsSuppressed = [](VibratoPointDataArray *owner) {
                    return VibratoPointDataArrayPrivate::get(owner)->suppressNotifications;
                },
                .aboutToSplice = [](VibratoPointDataArray *owner, int index, int length, const QList<QPointF> &values) {
                    emit owner->aboutToSplice(index, length, values);
                },
                .spliced = [](VibratoPointDataArray *owner, int index, int length, const QList<QPointF> &values) {
                    emit owner->spliced(index, length, values);
                },
                .aboutToRotate = [](VibratoPointDataArray *owner, int left, int middle, int right) {
                    emit owner->aboutToRotate(left, middle, right);
                },
                .rotated = [](VibratoPointDataArray *owner, int left, int middle, int right) {
                    emit owner->rotated(left, middle, right);
                },
                .aboutToSpliceAfterCommit = [](VibratoPointDataArray *owner, int index, int length, const QList<QPointF> &values) {
                    emit owner->aboutToSpliceAfterCommit(index, length, values);
                },
                .splicedAfterCommit = [](VibratoPointDataArray *owner, int index, int length, const QList<QPointF> &values) {
                    emit owner->splicedAfterCommit(index, length, values);
                },
                .aboutToRotateAfterCommit = [](VibratoPointDataArray *owner, int left, int middle, int right) {
                    emit owner->aboutToRotateAfterCommit(left, middle, right);
                },
                .rotatedAfterCommit = [](VibratoPointDataArray *owner, int left, int middle, int right) {
                    emit owner->rotatedAfterCommit(left, middle, right);
                },
            });
            return binding;
        }

    }

    VibratoPointDataArrayPrivate::VibratoPointDataArrayPrivate(VibratoPointDataArray *q,
                                                               Note *note,
                                                               VibratoPointDataArray::VibratoPointRole role)
        : q_ptr(q), note(note), role(role), jsIterable(new JSIterable(q, JSIterable::DataArray)) {
    }

    Handle VibratoPointDataArrayPrivate::relationHandle() const {
        auto *modelData = ModelPrivate::get(note->model());
        if (auto relation = orm::firstSnapshot(modelData->engine->query(Schema::noteVibratoPointRelationTable(),
                                                                        vibratoPointRelationQuery(note->handle(), role)))) {
            return orm::handleFromId(relation->id);
        }
        return {};
    }

    dini::Value VibratoPointDataArrayPrivate::associationValue() const {
        return orm::valueFromHandle(relationHandle());
    }

    void VibratoPointDataArrayPrivate::refresh(bool notify, bool itemsChanged) {
        Q_Q(VibratoPointDataArray);
        const auto relation = relationHandle();
        auto *modelData = ModelPrivate::get(note->model());
        const auto newSize = relation ? static_cast<int>(modelData->engine->listLength(Schema::vibratoPointList(), orm::valueFromHandle(relation))) : 0;
        const bool sizeChanged = size != newSize;
        size = newSize;

        if (!notify) {
            return;
        }
        if (sizeChanged) {
            emit q->sizeChanged(size);
        }
        if (itemsChanged || sizeChanged) {
            emit q->itemsChanged();
        }
    }

    VibratoPointDataArray::VibratoPointDataArray(Note *note, VibratoPointRole role)
        : QObject(note), d_ptr(new VibratoPointDataArrayPrivate(this, note, role)) {
    }

    VibratoPointDataArray::~VibratoPointDataArray() = default;

    int VibratoPointDataArray::size() const {
        Q_D(const VibratoPointDataArray);
        return d->size;
    }

    QList<QPointF> VibratoPointDataArray::items() const {
        Q_D(const VibratoPointDataArray);
        const auto relation = d->relationHandle();
        if (!relation) {
            return {};
        }
        auto *modelData = ModelPrivate::get(note()->model());
        return pointsFromView(modelData->engine->query(Schema::vibratoPointList(), vibratoPointQuery(relation)));
    }

    QList<QPointF> VibratoPointDataArray::slice(int index, int length) const {
        if (index < 0 || length < 0) {
            return {};
        }
        Q_D(const VibratoPointDataArray);
        const auto relation = d->relationHandle();
        if (!relation) {
            return {};
        }
        auto *modelData = ModelPrivate::get(note()->model());
        return pointsFromView(modelData->engine->query(Schema::vibratoPointList(), vibratoPointQuery(relation))
                                  .offset(static_cast<std::size_t>(index))
                                  .limit(static_cast<std::size_t>(length)));
    }

    bool VibratoPointDataArray::splice(int index, int length, const QList<QPointF> &values) {
        Q_D(VibratoPointDataArray);
        if (index < 0 || length < 0 || index > d->size || length > d->size - index) {
            return false;
        }
        const auto associationValue = d->associationValue();
        if (associationValue.isNull()) {
            return false;
        }

        auto *transaction = ModelPrivate::get(note()->model())->requireTransaction();
        emit aboutToSplice(index, length, values);
        d->suppressNotifications = true;
        try {
            for (int i = 0; i < length; ++i) {
                transaction->removeAt(Schema::vibratoPointList(), associationValue, static_cast<std::size_t>(index));
            }
            for (int i = 0; i < values.size(); ++i) {
                transaction->insert(Schema::vibratoPointList(),
                                    associationValue,
                                    static_cast<std::size_t>(index + i),
                                    pointValues(values.at(i)));
            }
        } catch (...) {
            d->suppressNotifications = false;
            throw;
        }
        d->suppressNotifications = false;
        d->refresh(true, true);
        emit spliced(index, length, values);
        return true;
    }

    bool VibratoPointDataArray::rotate(int leftIndex, int middleIndex, int rightIndex) {
        Q_D(VibratoPointDataArray);
        if (leftIndex < 0 || middleIndex < leftIndex || rightIndex < middleIndex || rightIndex > d->size) {
            return false;
        }
        const auto associationValue = d->associationValue();
        if (associationValue.isNull()) {
            return false;
        }
        ModelPrivate::get(note()->model())->rotate(Schema::vibratoPointList(), associationValue, leftIndex, middleIndex, rightIndex);
        return true;
    }

    VibratoPointDataArray::VibratoPointRole VibratoPointDataArray::role() const {
        Q_D(const VibratoPointDataArray);
        return d->role;
    }

    Note *VibratoPointDataArray::note() const {
        Q_D(const VibratoPointDataArray);
        return d->note;
    }

    std::vector<opendspx::ControlPoint> VibratoPointDataArray::toOpenDSPX() const {
        std::vector<opendspx::ControlPoint> result;
        const auto source = items();
        result.reserve(static_cast<std::size_t>(source.size()));
        for (const auto &point : source) {
            result.push_back(opendspx::ControlPoint {
                .x = point.x(),
                .y = point.y(),
            });
        }
        return result;
    }

    void VibratoPointDataArray::fromOpenDSPX(const std::vector<opendspx::ControlPoint> &points) {
        QList<QPointF> target;
        target.reserve(static_cast<qsizetype>(points.size()));
        for (const auto &point : points) {
            target.append(QPointF(point.x, point.y));
        }
        splice(0, size(), target);
    }

}


#include "moc_VibratoPointDataArray.cpp"
