#include "Sources.h"
#include "Sources_p.h"

#include <dini/transaction.h>
#include <opendspx/sources.h>

#include <dspxmodelCore/Schema.h>
#include <dspxmodelORM/DynamicMixingAnchorSequence.h>
#include <dspxmodelORM/SingerList.h>
#include <dspxmodelORM/SingingClip.h>
#include <dspxmodelORM/private/DynamicMixingAnchorSequence_p.h>
#include <dspxmodelORM/private/Model_p.h>
#include <dspxmodelORM/private/ORMBinding_p.h>
#include <dspxmodelORM/private/ORMUtils_p.h>
#include <dspxmodelORM/private/SingerList_p.h>
#include <dspxmodelORM/private/SingingClip_p.h>

namespace dspx {

    namespace {

        void setSingingClipSources(SingingClip *clip, Sources *sources, bool notify) {
            if (!clip) {
                return;
            }
            auto *d = SingingClipPrivate::get(clip);
            const bool changed = d->sources != sources;
            d->sources = sources;
            d->sourcesResolved = true;
            if (notify && changed) {
                emit clip->sourcesChanged(sources);
            }
        }

        bool setSourcesSingingClip(Sources *sources, SingingClip *clip, bool notify) {
            auto *d = SourcesPrivate::get(sources);
            auto *oldClip = d->singingClip;
            if (oldClip == clip) {
                setSingingClipSources(clip, sources, notify);
                return false;
            }

            if (oldClip) {
                setSingingClipSources(oldClip, nullptr, notify);
            }
            if (clip) {
                if (auto *previousSources = SingingClipPrivate::get(clip)->sources; previousSources && previousSources != sources) {
                    SourcesPrivate::get(previousSources)->singingClip = nullptr;
                    if (notify) {
                        emit previousSources->singingClipChanged(nullptr);
                    }
                }
                setSingingClipSources(clip, sources, notify);
            }

            d->singingClip = clip;
            if (notify) {
                emit sources->singingClipChanged(clip);
            }
            return true;
        }

        SingingClip *singingClipFromValue(ModelPrivate &model, const dini::Value &value) {
            const auto handle = orm::handleFromValue(value);
            return handle ? model.ensure<SingingClip>(handle) : nullptr;
        }

        bool applySourcesParent(Sources *sources, const dini::Value &value, bool notify) {
            auto *model = ModelPrivate::get(sources->model());
            return setSourcesSingingClip(sources, singingClipFromValue(*model, value), notify);
        }

        const std::vector<orm::ColumnBinding<Sources>> &sourcesColumnBindings() {
            static const std::vector<orm::ColumnBinding<Sources>> bindings {
                orm::stringFieldWithSignal<Sources, SourcesPrivate>(Schema::sourcesCategoryColumn(),
                                                                    &SourcesPrivate::category,
                                                                    &Sources::categoryChanged),
            };
            return bindings;
        }

    }

    namespace orm {

        const TableBinding &sourcesTableBinding() {
            static const TableBinding binding {
                .table = Schema::sourcesTable(),
                .itemInserted = [](ModelPrivate &model, const dini::ItemInsertedChange &change) {
                    if (auto *item = model.ensure<Sources>(change.item)) {
                        syncSourcesColumns(item, change.item, true);
                    }
                },
                .itemRemoved = [](ModelPrivate &model, const dini::ItemSnapshot &snapshot, bool) {
                    const auto handle = orm::handleFromId(snapshot.id);
                    if (auto *item = model.find<Sources>(handle)) {
                        applySourcesParent(item, dini::Value::null(), true);
                        model.sourcesObjects.remove(handle);
                        item->deleteLater();
                        return;
                    }
                    model.sourcesObjects.remove(handle);
                },
                .columnUpdated = [](ModelPrivate &model, const dini::ColumnUpdatedChange &change) {
                    const auto handle = orm::handleFromId(change.itemId);
                    if (auto *item = model.find<Sources>(handle)) {
                        applySourcesColumn(item, change.column, change.newValue, true);
                        return;
                    }
                    auto *item = model.ensure<Sources>(handle);
                    if (!item) {
                        return;
                    }
                    if (change.column == Schema::sourcesParent().column()) {
                        applySourcesParent(item, change.oldValue, false);
                    }
                    applySourcesColumn(item, change.column, change.newValue, true);
                },
            };
            return binding;
        }

        void syncSourcesColumns(Sources *item, const dini::ItemSnapshot &snapshot, bool notify) {
            syncColumnBindings(sourcesColumnBindings(), item, snapshot, notify);
            applySourcesParent(item, snapshotValue(snapshot, Schema::sourcesParent().column()), notify);
        }

        bool applySourcesColumn(Sources *item, const dini::ColumnHandle &column, const dini::Value &value, bool notify) {
            if (applyColumnBinding(sourcesColumnBindings(), item, column, value, notify)) {
                return true;
            }
            if (column == Schema::sourcesParent().column()) {
                applySourcesParent(item, value, notify);
                return true;
            }
            return false;
        }

    }

    SourcesPrivate::SourcesPrivate(Sources *q) : q_ptr(q) {
    }

    Sources::Sources(Handle handle, Model *model) : EntityObject(handle, model, model), d_ptr(new SourcesPrivate(this)) {
        Q_D(Sources);
        d->singers = SingerListPrivate::create(this);
        d->dynamicMixingAnchors = DynamicMixingAnchorSequencePrivate::create(this);
        SingerListPrivate::get(d->singers)->refresh(false, false);
        DynamicMixingAnchorSequencePrivate::get(d->dynamicMixingAnchors)->refresh(false);
    }

    Sources::~Sources() = default;

    QString Sources::category() const {
        Q_D(const Sources);
        return d->category;
    }

    void Sources::setCategory(const QString &category) {
        ModelPrivate::get(model())->update(handle(), Schema::sourcesCategoryColumn(), orm::valueFromString(category));
    }

    SingerList *Sources::singers() const {
        Q_D(const Sources);
        return d->singers;
    }

    DynamicMixingAnchorSequence *Sources::dynamicMixingAnchors() const {
        Q_D(const Sources);
        return d->dynamicMixingAnchors;
    }

    SingingClip *Sources::singingClip() const {
        Q_D(const Sources);
        return d->singingClip;
    }

    opendspx::Sources Sources::toOpenDSPX() const {
        return opendspx::Sources {
            .category = category().toStdString(),
            .mix = dynamicMixingAnchors()->toOpenDSPX(),
            .singers = singers()->toOpenDSPX(),
        };
    }

    void Sources::fromOpenDSPX(const opendspx::Sources &sources) {
        setCategory(QString::fromStdString(sources.category));
        dynamicMixingAnchors()->fromOpenDSPX(sources.mix);
        singers()->fromOpenDSPX(sources.singers);
    }

}


#include "moc_Sources.cpp"
