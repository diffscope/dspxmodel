#include "SingingClip.h"
#include "SingingClip_p.h"

#include <vector>

#include <dini/engine.h>
#include <dini/transaction.h>
#include <opendspx/singingclip.h>

#include <dspxmodelCore/Schema.h>
#include <dspxmodelORM/Model.h>
#include <dspxmodelORM/OpenDSPXConversion.h>
#include <dspxmodelORM/ParameterMap.h>
#include <dspxmodelORM/Sources.h>
#include <dspxmodelORM/private/Clip_p.h>
#include <dspxmodelORM/private/ConversionUtils_p.h>
#include <dspxmodelORM/private/Model_p.h>
#include <dspxmodelORM/private/NoteSequence_p.h>
#include <dspxmodelORM/private/ORMBinding_p.h>
#include <dspxmodelORM/private/ORMUtils_p.h>
#include <dspxmodelORM/private/ParameterMap_p.h>

namespace dspx {

    namespace {

        const std::vector<orm::ColumnBinding<SingingClip>> &singingClipColumnBindings() {
            static const std::vector<orm::ColumnBinding<SingingClip>> bindings {
                orm::stringFieldWithSignal<SingingClip, SingingClipPrivate>(Schema::singingClipDefaultLanguageColumn(), &SingingClipPrivate::defaultLanguage, &SingingClip::defaultLanguageChanged),
            };
            return bindings;
        }

    }

    namespace orm {

        void syncSingingClipColumns(SingingClip *item, const dini::ItemSnapshot &snapshot, bool notify) {
            syncColumnBindings(singingClipColumnBindings(), item, snapshot, notify);
        }

        bool applySingingClipColumn(SingingClip *item, const dini::ColumnHandle &column, const dini::Value &value, bool notify) {
            return applyColumnBinding(singingClipColumnBindings(), item, column, value, notify);
        }

    }

    SingingClipPrivate::SingingClipPrivate(SingingClip *q) : q_ptr(q) {
    }

    SingingClip::SingingClip(Handle handle, Model *model) : Clip(handle, model), d_ptr(new SingingClipPrivate(this)) {
        ClipPrivate::get(static_cast<Clip *>(this))->kind = Clip::Singing;
        d_ptr->notes = NoteSequencePrivate::create(this);
        d_ptr->parameters = ParameterMapPrivate::create(this);
        NoteSequencePrivate::get(d_ptr->notes)->refresh(false);
        ParameterMapPrivate::get(d_ptr->parameters)->refresh(false);
    }

    SingingClip::~SingingClip() = default;

    QString SingingClip::defaultLanguage() const {
        Q_D(const SingingClip);
        return d->defaultLanguage;
    }

    void SingingClip::setDefaultLanguage(const QString &defaultLanguage) {
        ModelPrivate::get(model())->update(handle(), Schema::singingClipDefaultLanguageColumn(), orm::valueFromString(defaultLanguage));
    }

    Sources *SingingClip::sources() const {
        Q_D(const SingingClip);
        if (!d->sourcesResolved) {
            auto *mutableData = const_cast<SingingClipPrivate *>(d);
            auto *modelData = ModelPrivate::get(model());
            const auto view = modelData->engine->query(Schema::sourcesTable(), {
                .filter = orm::parentFilter(Schema::sourcesParent(), handle()),
            });
            mutableData->sources = nullptr;
            if (auto snapshot = orm::firstSnapshot(view)) {
                mutableData->sources = modelData->ensure<Sources>(*snapshot);
            }
            mutableData->sourcesResolved = true;
        }
        return d->sources;
    }

    void SingingClip::setSources(Sources *sources) {
        Q_D(SingingClip);
        if (sources && sources->model() != model()) {
            return;
        }
        if (this->sources() == sources) {
            return;
        }
        if (d->sources) {
            ModelPrivate::get(model())->update(d->sources->handle(), Schema::sourcesParent().column(), dini::Value::null());
        }
        if (sources) {
            ModelPrivate::get(model())->update(sources->handle(), Schema::sourcesParent().column(), orm::valueFromHandle(handle()));
        }
    }

    NoteSequence *SingingClip::notes() const {
        Q_D(const SingingClip);
        return d->notes;
    }

    ParameterMap *SingingClip::parameters() const {
        Q_D(const SingingClip);
        return d->parameters;
    }

    opendspx::SingingClip SingingClip::toOpenDSPX() const {
        opendspx::SingingClip target;
        toOpenDSPXBase(target);
        target.workspace["diffscope"]["defaultLanguage"] = defaultLanguage().toStdString();
        target.notes = notes()->toOpenDSPX();
        target.params = parameters()->toOpenDSPX();
        if (auto *sourceData = sources()) {
            target.sources = sourceData->toOpenDSPX();
        } else {
            target.sources.reset();
        }

        OpenDSPXConversion::convertClipToOpenDSPX(this, target);
        return target;
    }

    void SingingClip::fromOpenDSPX(const opendspx::SingingClip &clip) {
        fromOpenDSPXBase(clip);
        QString defaultLanguage;
        if (auto it = clip.workspace.find("diffscope"); it != clip.workspace.end()) {
            if (auto value = conv::optionalChain(it->second, "defaultLanguage"); value.isString()) {
                defaultLanguage = QString::fromStdString(value.toString());
            }
        }
        setDefaultLanguage(defaultLanguage);
        notes()->fromOpenDSPX(clip.notes);
        parameters()->fromOpenDSPX(clip.params);
        if (clip.sources.has_value()) {
            auto *targetSources = sources();
            if (!targetSources) {
                targetSources = model()->createSources();
                setSources(targetSources);
            }
            targetSources->fromOpenDSPX(*clip.sources);
        } else {
            setSources(nullptr);
        }
        OpenDSPXConversion::convertClipFromOpenDSPX(this, clip);
    }

}


#include "moc_SingingClip.cpp"
