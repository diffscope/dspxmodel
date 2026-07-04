#include "SingingClip.h"
#include "SingingClip_p.h"

#include <dini/transaction.h>
#include <opendspx/singingclip.h>

#include <dspxmodelCore/Schema.h>
#include <dspxmodelORM/OpenDSPXConversion.h>
#include <dspxmodelORM/Sources.h>
#include <dspxmodelORM/private/Clip_p.h>
#include <dspxmodelORM/private/Model_p.h>
#include <dspxmodelORM/private/NoteSequence_p.h>
#include <dspxmodelORM/private/ORMUtils_p.h>

namespace dspx {

    SingingClipPrivate::SingingClipPrivate(SingingClip *q) : q_ptr(q) {
    }

    SingingClip::SingingClip(Handle handle, Model *model) : Clip(handle, model), d_ptr(new SingingClipPrivate(this)) {
        ClipPrivate::get(static_cast<Clip *>(this))->type = Clip::Singing;
        d_ptr->notes = NoteSequencePrivate::create(this);
        NoteSequencePrivate::get(d_ptr->notes)->refresh(false);
    }

    SingingClip::~SingingClip() = default;

    Sources *SingingClip::sources() const {
        Q_D(const SingingClip);
        return d->sources;
    }

    void SingingClip::setSources(Sources *sources) {
        Q_D(SingingClip);
        if (d->sources == sources) {
            return;
        }
        if (d->sources) {
            ModelPrivate::get(model())->update(d->sources->handle(), Schema::sourcesParent().column(), dini::Value::null());
        }
        if (sources) {
            ModelPrivate::get(model())->update(sources->handle(), Schema::sourcesParent().column(), orm::valueFromHandle(handle()));
        }
        d->sources = sources;
        emit sourcesChanged(d->sources);
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
        target.notes = notes()->toOpenDSPX();
        OpenDSPXConversion::convertClipToOpenDSPX(this, target);
        return target;
    }

    void SingingClip::fromOpenDSPX(const opendspx::SingingClip &clip) {
        fromOpenDSPXBase(clip);
        notes()->fromOpenDSPX(clip.notes);
        OpenDSPXConversion::convertClipFromOpenDSPX(this, clip);
    }

}
