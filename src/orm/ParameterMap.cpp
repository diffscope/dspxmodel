#include "ParameterMap.h"
#include "ParameterMap_p.h"

#include <cstdint>

#include <dini/engine.h>
#include <dini/query.h>
#include <opendspx/params.h>

#include <dspxmodelCore/Schema.h>
#include <dspxmodelORM/Model.h>
#include <dspxmodelORM/Parameter.h>
#include <dspxmodelORM/SingingClip.h>
#include <dspxmodelORM/private/Model_p.h>
#include <dspxmodelORM/private/ORMUtils_p.h>

namespace dspx {

    namespace {

        dini::QuerySpec parameterMapQuery(Handle singingClipHandle) {
            return dini::QuerySpec {
                .filter = orm::parentFilter(Schema::parameterParent(), singingClipHandle),
                .sortKeys = {
                    dini::SortKey {
                        .field = dini::FieldRef::column(Schema::parameterKeyColumn()),
                        .direction = dini::SortDirection::Ascending,
                    },
                },
            };
        }

        dini::QuerySpec parameterByKeyQuery(Handle singingClipHandle, const QString &key) {
            return dini::QuerySpec {
                .filter = dini::FilterExpression::all({
                    orm::parentFilter(Schema::parameterParent(), singingClipHandle),
                    orm::equalFilter(dini::FieldRef::column(Schema::parameterKeyColumn()), orm::valueFromString(key)),
                }),
            };
        }

        QString keyFromSnapshot(const dini::ItemSnapshot &snapshot) {
            const auto &value = orm::snapshotValue(snapshot, Schema::parameterKeyColumn());
            return value.isNull() ? QString() : orm::stringFromValue(value);
        }

        QList<Parameter *> parametersFromView(ModelPrivate *model, const dini::View &view) {
            QList<Parameter *> result;
            for (const auto &snapshot : view.toVector()) {
                if (auto *item = model->ensure<Parameter>(snapshot)) {
                    result.append(item);
                }
            }
            return result;
        }

    }

    ParameterMapPrivate::ParameterMapPrivate(ParameterMap *q, SingingClip *singingClip)
        : q_ptr(q), singingClip(singingClip) {
    }

    void ParameterMapPrivate::refresh(bool notify) {
        Q_Q(ParameterMap);
        auto *modelData = ModelPrivate::get(singingClip->model());
        const auto view = modelData->engine->query(Schema::parameterTable(), parameterMapQuery(singingClip->handle()));
        const auto newSize = static_cast<int>(view.count());
        const bool sizeChanged = size != newSize;
        size = newSize;

        if (!notify) {
            return;
        }
        if (sizeChanged) {
            emit q->sizeChanged(size);
        }
        emit q->keysChanged();
        emit q->itemsChanged();
    }

    ParameterMap::ParameterMap(SingingClip *singingClip)
        : QObject(singingClip), d_ptr(new ParameterMapPrivate(this, singingClip)) {
    }

    ParameterMap::~ParameterMap() = default;

    int ParameterMap::size() const {
        Q_D(const ParameterMap);
        return d->size;
    }

    QStringList ParameterMap::keys() const {
        Q_D(const ParameterMap);
        QStringList result;
        auto *modelData = ModelPrivate::get(singingClip()->model());
        const auto view = modelData->engine->query(Schema::parameterTable(), parameterMapQuery(d->singingClip->handle()));
        for (const auto &snapshot : view.toVector()) {
            result.append(keyFromSnapshot(snapshot));
        }
        return result;
    }

    QList<Parameter *> ParameterMap::items() const {
        Q_D(const ParameterMap);
        auto *modelData = ModelPrivate::get(singingClip()->model());
        return parametersFromView(modelData, modelData->engine->query(Schema::parameterTable(),
                                                                      parameterMapQuery(d->singingClip->handle())));
    }

    bool ParameterMap::containsKey(const QString &key) const {
        return item(key) != nullptr;
    }

    bool ParameterMap::containsItem(Parameter *item) const {
        return item && item->parameterMap() == this;
    }

    Parameter *ParameterMap::item(const QString &key) const {
        Q_D(const ParameterMap);
        auto *modelData = ModelPrivate::get(singingClip()->model());
        const auto view = modelData->engine->query(Schema::parameterTable(),
                                                   parameterByKeyQuery(d->singingClip->handle(), key));
        if (auto snapshot = orm::firstSnapshot(view)) {
            return modelData->ensure<Parameter>(*snapshot);
        }
        return nullptr;
    }

    bool ParameterMap::insertItem(const QString &key, Parameter *item) {
        if (!item || item->model() != singingClip()->model() || item->parameterMap() || containsKey(key)) {
            return false;
        }
        ModelPrivate::get(singingClip()->model())->update(item->handle(), {
            dini::ColumnValue {.column = Schema::parameterParent().column(), .value = orm::valueFromHandle(singingClip()->handle())},
            dini::ColumnValue {.column = Schema::parameterKeyColumn(), .value = orm::valueFromString(key)},
        });
        return true;
    }

    bool ParameterMap::removeItem(const QString &key) {
        auto *parameter = item(key);
        if (!parameter) {
            return false;
        }
        ModelPrivate::get(singingClip()->model())->update(parameter->handle(), {
            dini::ColumnValue {.column = Schema::parameterParent().column(), .value = dini::Value::null()},
            dini::ColumnValue {.column = Schema::parameterKeyColumn(), .value = dini::Value::null()},
        });
        return true;
    }

    bool ParameterMap::moveItem(const QString &key, ParameterMap *map, const QString &newKey) {
        auto *parameter = item(key);
        if (!parameter || !map || map->singingClip()->model() != singingClip()->model() || map->containsKey(newKey)) {
            return false;
        }
        ModelPrivate::get(singingClip()->model())->update(parameter->handle(), {
            dini::ColumnValue {.column = Schema::parameterParent().column(), .value = orm::valueFromHandle(map->singingClip()->handle())},
            dini::ColumnValue {.column = Schema::parameterKeyColumn(), .value = orm::valueFromString(newKey)},
        });
        return true;
    }

    opendspx::Params ParameterMap::toOpenDSPX() const {
        opendspx::Params result;
        const auto sourceKeys = keys();
        for (const auto &key : sourceKeys) {
            auto *parameter = item(key);
            if (parameter) {
                result[key.toStdString()] = parameter->toOpenDSPX();
            }
        }
        return result;
    }

    void ParameterMap::fromOpenDSPX(const opendspx::Params &params) {
        const auto sourceKeys = keys();
        for (const auto &key : sourceKeys) {
            removeItem(key);
        }
        for (const auto &[key, source] : params) {
            auto *parameter = singingClip()->model()->createParameter();
            parameter->fromOpenDSPX(source);
            insertItem(QString::fromStdString(key), parameter);
        }
    }

    SingingClip *ParameterMap::singingClip() const {
        Q_D(const ParameterMap);
        return d->singingClip;
    }

}


#include "moc_ParameterMap.cpp"
