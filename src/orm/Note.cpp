#include "Note.h"
#include "Note_p.h"

#include <cstdint>
#include <utility>

#include <dini/transaction.h>
#include <opendspx/note.h>

#include <dspxmodelCore/Schema.h>
#include <dspxmodelORM/NoteSequence.h>
#include <dspxmodelORM/OpenDSPXConversion.h>
#include <dspxmodelORM/PhonemeSequence.h>
#include <dspxmodelORM/SingingClip.h>
#include <dspxmodelORM/VibratoPointDataArray.h>
#include <dspxmodelORM/private/ConversionUtils_p.h>
#include <dspxmodelORM/private/Model_p.h>
#include <dspxmodelORM/private/NoteSequence_p.h>
#include <dspxmodelORM/private/ORMBinding_p.h>
#include <dspxmodelORM/private/ORMUtils_p.h>
#include <dspxmodelORM/private/PhonemeSequence_p.h>
#include <dspxmodelORM/private/VibratoPointDataArray_p.h>

namespace dspx {

    namespace {

        NoteSequence *noteOwnerFromValue(ModelPrivate &model, const dini::Value &value) {
            const auto singingClipHandle = orm::handleFromValue(value);
            auto *singingClip = singingClipHandle ? model.ensure<SingingClip>(singingClipHandle) : nullptr;
            return singingClip ? singingClip->notes() : nullptr;
        }

        const std::vector<orm::ColumnBinding<Note>> &noteColumnBindings() {
            static const std::vector<orm::ColumnBinding<Note>> bindings {
                orm::intFieldWithSignal<Note, NotePrivate>(Schema::noteCentShiftColumn(), &NotePrivate::centShift, &Note::centShiftChanged, &Note::centShiftChangedAfterCommit),
                orm::intFieldWithSignal<Note, NotePrivate>(Schema::noteKeyNumberColumn(), &NotePrivate::keyNumber, &Note::keyNumberChanged, &Note::keyNumberChangedAfterCommit),
                orm::stringFieldWithSignal<Note, NotePrivate>(Schema::noteLanguageColumn(), &NotePrivate::language, &Note::languageChanged, &Note::languageChangedAfterCommit),
                orm::intFieldWithSignal<Note, NotePrivate>(Schema::noteLengthColumn(), &NotePrivate::length, &Note::lengthChanged, &Note::lengthChangedAfterCommit),
                orm::stringFieldWithSignal<Note, NotePrivate>(Schema::noteLyricColumn(), &NotePrivate::lyric, &Note::lyricChanged, &Note::lyricChangedAfterCommit),
                orm::intFieldWithSignal<Note, NotePrivate>(Schema::notePositionColumn(), &NotePrivate::position, &Note::positionChanged, &Note::positionChangedAfterCommit),
                orm::stringFieldWithSignal<Note, NotePrivate>(Schema::noteOriginalPronunciationColumn(), &NotePrivate::originalPronunciation, &Note::originalPronunciationChanged, &Note::originalPronunciationChangedAfterCommit),
                orm::stringFieldWithSignal<Note, NotePrivate>(Schema::noteEditedPronunciationColumn(), &NotePrivate::editedPronunciation, &Note::editedPronunciationChanged, &Note::editedPronunciationChangedAfterCommit),
                orm::previousNextFieldWithSignal<Note, NotePrivate>(Schema::notePreviousItemColumn(), &NotePrivate::previousHandle, &NotePrivate::previous, &Note::previousItemChanged, &Note::previousItemChangedAfterCommit),
                orm::previousNextFieldWithSignal<Note, NotePrivate>(Schema::noteNextItemColumn(), &NotePrivate::nextHandle, &NotePrivate::next, &Note::nextItemChanged, &Note::nextItemChangedAfterCommit),
                {Schema::noteOverlappedCountColumn(), [](Note *q, const dini::Value &value) {
                     auto *d = NotePrivate::get(q);
                     const auto oldOverlapped = d->overlappedCount > 0;
                     d->overlappedCount = static_cast<int>(value.asInt64());
                     return oldOverlapped != (d->overlappedCount > 0);
                 }, [](Note *q) {
                     emit q->overlappedChanged(q->overlapped());
                 }, [](Note *q) {
                     emit q->overlappedChangedAfterCommit(q->overlapped());
                 }},
                orm::intFieldWithSignal<Note, NotePrivate>(Schema::noteVibratoAmplitudeColumn(), &NotePrivate::vibratoAmplitude, &Note::vibratoAmplitudeChanged, &Note::vibratoAmplitudeChangedAfterCommit),
                orm::doubleFieldWithSignal<Note, NotePrivate>(Schema::noteVibratoEndColumn(), &NotePrivate::vibratoEnd, &Note::vibratoEndChanged, &Note::vibratoEndChangedAfterCommit),
                orm::doubleFieldWithSignal<Note, NotePrivate>(Schema::noteVibratoFrequencyColumn(), &NotePrivate::vibratoFrequency, &Note::vibratoFrequencyChanged, &Note::vibratoFrequencyChangedAfterCommit),
                orm::intFieldWithSignal<Note, NotePrivate>(Schema::noteVibratoOffsetColumn(), &NotePrivate::vibratoOffset, &Note::vibratoOffsetChanged, &Note::vibratoOffsetChangedAfterCommit),
                orm::doubleFieldWithSignal<Note, NotePrivate>(Schema::noteVibratoPhaseColumn(), &NotePrivate::vibratoPhase, &Note::vibratoPhaseChanged, &Note::vibratoPhaseChangedAfterCommit),
                orm::doubleFieldWithSignal<Note, NotePrivate>(Schema::noteVibratoStartColumn(), &NotePrivate::vibratoStart, &Note::vibratoStartChanged, &Note::vibratoStartChangedAfterCommit),
                orm::binaryField<Note, NotePrivate>(Schema::noteWorkspaceColumn(), &NotePrivate::workspaceData, nullptr, nullptr),
                {Schema::noteParent().column(), [](Note *q, const dini::Value &value) {
                     auto *model = ModelPrivate::get(q->model());
                     auto *d = NotePrivate::get(q);
                     auto *newSequence = noteOwnerFromValue(*model, value);
                     const bool changed = d->sequence != newSequence;
                     d->sequence = newSequence;
                     return changed;
                 }, [](Note *q) {
                     emit q->noteSequenceChanged(NotePrivate::get(q)->sequence);
                 }, [](Note *q) {
                     emit q->noteSequenceChangedAfterCommit(NotePrivate::get(q)->sequence);
                 }},
            };
            return bindings;
        }

    }

    namespace orm {

        const OrderSpec &noteOrderSpec() {
            static const OrderSpec spec {{Schema::notePositionColumn(), Schema::noteLengthColumn(), Schema::noteKeyNumberColumn()}, true};
            return spec;
        }

        const TableBinding &noteTableBinding() {
            static const TableBinding binding = makeAssociatedTableBinding<Note, NoteSequence>({
                .table = Schema::noteTable(),
                .membershipColumns = {Schema::noteParent().column()},
                .orderColumns = {Schema::notePositionColumn(), Schema::noteLengthColumn(), Schema::noteKeyNumberColumn()},
                .moveSemantics = MoveSemantics::BetweenOwners,
                .ensure = [](ModelPrivate &model, const dini::ItemSnapshot &snapshot) { return model.ensure<Note>(snapshot); },
                .find = [](ModelPrivate &model, Handle handle) { return model.find<Note>(handle); },
                .removeObject = [](ModelPrivate &model, Handle handle) { model.noteObjects.remove(handle); },
                .sync = [](Note *item, const dini::ItemSnapshot &snapshot, bool notify) { syncNoteColumns(item, snapshot, notify); },
                .applyColumn = [](Note *item, const dini::ColumnHandle &column, const dini::Value &value, bool notify) { return applyNoteColumn(item, column, value, notify); },
                .ownerForSnapshot = [](ModelPrivate &model, const dini::ItemSnapshot &snapshot) {
                    return noteOwnerFromValue(model, orm::snapshotValue(snapshot, Schema::noteParent().column()));
                },
                .ownerForChange = [](ModelPrivate &model, const dini::ColumnUpdatedChange &change, bool oldValue) {
                    if (change.column == Schema::noteParent().column()) {
                        return noteOwnerFromValue(model, oldValue ? change.oldValue : change.newValue);
                    }
                    auto *note = model.find<Note>(orm::handleFromId(change.itemId));
                    return note ? note->noteSequence() : nullptr;
                },
                .setOwner = [](Note *item, NoteSequence *owner, bool notify) { NotePrivate::get(item)->setSequence(owner, notify); },
                .ownerChangedAfterCommit = [](Note *item, NoteSequence *owner) { emit item->noteSequenceChangedAfterCommit(owner); },
                .refreshOwner = [](NoteSequence *owner, bool notify) { NoteSequencePrivate::get(owner)->refresh(notify); },
                .refreshOwnerAfterCommit = [](NoteSequence *owner, bool sizeChanged, bool orderChanged) {
                    if (sizeChanged) emit owner->sizeChangedAfterCommit(owner->size());
                    if (orderChanged) {
                        emit owner->firstItemChangedAfterCommit(owner->firstItem());
                        emit owner->lastItemChangedAfterCommit(owner->lastItem());
                    }
                },
                .itemAboutToInsert = [](NoteSequence *owner, Note *item, NoteSequence *movedFrom) { emit owner->itemAboutToInsert(item, movedFrom); },
                .itemInserted = [](NoteSequence *owner, Note *item, NoteSequence *movedFrom) { emit owner->itemInserted(item, movedFrom); },
                .itemAboutToRemove = [](NoteSequence *owner, Note *item, NoteSequence *movedTo) { emit owner->itemAboutToRemove(item, movedTo); },
                .itemRemoved = [](NoteSequence *owner, Note *item, NoteSequence *movedTo) { emit owner->itemRemoved(item, movedTo); },
                .itemAboutToInsertAfterCommit = [](NoteSequence *owner, Note *item, NoteSequence *movedFrom) { emit owner->itemAboutToInsertAfterCommit(item, movedFrom); },
                .itemInsertedAfterCommit = [](NoteSequence *owner, Note *item, NoteSequence *movedFrom) { emit owner->itemInsertedAfterCommit(item, movedFrom); },
                .itemAboutToRemoveAfterCommit = [](NoteSequence *owner, Note *item, NoteSequence *movedTo) { emit owner->itemAboutToRemoveAfterCommit(item, movedTo); },
                .itemRemovedAfterCommit = [](NoteSequence *owner, Note *item, NoteSequence *movedTo) { emit owner->itemRemovedAfterCommit(item, movedTo); },
            });
            return binding;
        }

        void syncNoteColumns(Note *item, const dini::ItemSnapshot &snapshot, bool notify) {
            syncColumnBindings(noteColumnBindings(), item, snapshot, notify);
        }

        bool applyNoteColumn(Note *item, const dini::ColumnHandle &column, const dini::Value &value, bool notify) {
            return applyColumnBinding(noteColumnBindings(), item, column, value, notify);
        }

    }

    NotePrivate::NotePrivate(Note *q) : q_ptr(q) {
    }

    void NotePrivate::setSequence(NoteSequence *newSequence, bool notify) {
        Q_Q(Note);
        if (sequence == newSequence) {
            return;
        }
        sequence = newSequence;
        if (notify) {
            emit q->noteSequenceChanged(sequence);
        }
    }

    dini::ByteArray NotePrivate::workspace() const {
        return workspaceData;
    }

    void NotePrivate::setWorkspace(dini::ByteArray workspace) {
        Q_Q(Note);
        ModelPrivate::get(q->model())->update(q->handle(), Schema::noteWorkspaceColumn(), dini::Value(std::move(workspace)));
    }

    Note::Note(Handle handle, Model *model) : EntityObject(handle, model, model), d_ptr(new NotePrivate(this)) {
        Q_D(Note);
        d->originalPhonemes = PhonemeSequencePrivate::create(this, PhonemeSequence::Original);
        d->editedPhonemes = PhonemeSequencePrivate::create(this, PhonemeSequence::Edited);
        d->vibratoAmplitudeControlPoints = VibratoPointDataArrayPrivate::create(this, VibratoPointDataArray::Amplitude);
        d->vibratoFrequencyControlPoints = VibratoPointDataArrayPrivate::create(this, VibratoPointDataArray::Frequency);
        VibratoPointDataArrayPrivate::get(d->vibratoAmplitudeControlPoints)->refresh(false, false);
        VibratoPointDataArrayPrivate::get(d->vibratoFrequencyControlPoints)->refresh(false, false);
    }

    Note::~Note() = default;

    int Note::centShift() const {
        Q_D(const Note);
        return d->centShift;
    }

    void Note::setCentShift(int centShift) {
        ModelPrivate::get(model())->update(handle(), Schema::noteCentShiftColumn(), dini::Value(static_cast<std::int64_t>(centShift)));
    }

    int Note::keyNumber() const {
        Q_D(const Note);
        return d->keyNumber;
    }

    void Note::setKeyNumber(int keyNumber) {
        ModelPrivate::get(model())->update(handle(), Schema::noteKeyNumberColumn(), dini::Value(static_cast<std::int64_t>(keyNumber)));
    }

    QString Note::language() const {
        Q_D(const Note);
        return d->language;
    }

    void Note::setLanguage(const QString &language) {
        ModelPrivate::get(model())->update(handle(), Schema::noteLanguageColumn(), orm::valueFromString(language));
    }

    int Note::length() const {
        Q_D(const Note);
        return d->length;
    }

    void Note::setLength(int length) {
        ModelPrivate::get(model())->update(handle(), Schema::noteLengthColumn(), dini::Value(static_cast<std::int64_t>(length)));
    }

    QString Note::lyric() const {
        Q_D(const Note);
        return d->lyric;
    }

    void Note::setLyric(const QString &lyric) {
        ModelPrivate::get(model())->update(handle(), Schema::noteLyricColumn(), orm::valueFromString(lyric));
    }

    int Note::position() const {
        Q_D(const Note);
        return d->position;
    }

    void Note::setPosition(int position) {
        ModelPrivate::get(model())->update(handle(), Schema::notePositionColumn(), dini::Value(static_cast<std::int64_t>(position)));
    }

    QString Note::originalPronunciation() const {
        Q_D(const Note);
        return d->originalPronunciation;
    }

    void Note::setOriginalPronunciation(const QString &originalPronunciation) {
        ModelPrivate::get(model())->update(handle(), Schema::noteOriginalPronunciationColumn(), orm::valueFromString(originalPronunciation));
    }

    QString Note::editedPronunciation() const {
        Q_D(const Note);
        return d->editedPronunciation;
    }

    void Note::setEditedPronunciation(const QString &editedPronunciation) {
        ModelPrivate::get(model())->update(handle(), Schema::noteEditedPronunciationColumn(), orm::valueFromString(editedPronunciation));
    }

    bool Note::overlapped() const {
        Q_D(const Note);
        return d->overlappedCount > 0;
    }

    Note *Note::previousItem() const {
        Q_D(const Note);
        if (!d->previous && d->previousHandle) {
            d->previous = ModelPrivate::get(model())->ensure<Note>(d->previousHandle);
        }
        return d->previous;
    }

    Note *Note::nextItem() const {
        Q_D(const Note);
        if (!d->next && d->nextHandle) {
            d->next = ModelPrivate::get(model())->ensure<Note>(d->nextHandle);
        }
        return d->next;
    }

    int Note::vibratoAmplitude() const {
        Q_D(const Note);
        return d->vibratoAmplitude;
    }

    void Note::setVibratoAmplitude(int vibratoAmplitude) {
        ModelPrivate::get(model())->update(handle(), Schema::noteVibratoAmplitudeColumn(), dini::Value(static_cast<std::int64_t>(vibratoAmplitude)));
    }

    double Note::vibratoEnd() const {
        Q_D(const Note);
        return d->vibratoEnd;
    }

    void Note::setVibratoEnd(double vibratoEnd) {
        ModelPrivate::get(model())->update(handle(), Schema::noteVibratoEndColumn(), dini::Value(vibratoEnd));
    }

    double Note::vibratoFrequency() const {
        Q_D(const Note);
        return d->vibratoFrequency;
    }

    void Note::setVibratoFrequency(double vibratoFrequency) {
        ModelPrivate::get(model())->update(handle(), Schema::noteVibratoFrequencyColumn(), dini::Value(vibratoFrequency));
    }

    int Note::vibratoOffset() const {
        Q_D(const Note);
        return d->vibratoOffset;
    }

    void Note::setVibratoOffset(int vibratoOffset) {
        ModelPrivate::get(model())->update(handle(), Schema::noteVibratoOffsetColumn(), dini::Value(static_cast<std::int64_t>(vibratoOffset)));
    }

    double Note::vibratoPhase() const {
        Q_D(const Note);
        return d->vibratoPhase;
    }

    void Note::setVibratoPhase(double vibratoPhase) {
        ModelPrivate::get(model())->update(handle(), Schema::noteVibratoPhaseColumn(), dini::Value(vibratoPhase));
    }

    double Note::vibratoStart() const {
        Q_D(const Note);
        return d->vibratoStart;
    }

    void Note::setVibratoStart(double vibratoStart) {
        ModelPrivate::get(model())->update(handle(), Schema::noteVibratoStartColumn(), dini::Value(vibratoStart));
    }

    PhonemeSequence *Note::editedPhonemes() const {
        Q_D(const Note);
        return d->editedPhonemes;
    }

    PhonemeSequence *Note::originalPhonemes() const {
        Q_D(const Note);
        return d->originalPhonemes;
    }

    VibratoPointDataArray *Note::vibratoAmplitudeControlPoints() const {
        Q_D(const Note);
        return d->vibratoAmplitudeControlPoints;
    }

    VibratoPointDataArray *Note::vibratoFrequencyControlPoints() const {
        Q_D(const Note);
        return d->vibratoFrequencyControlPoints;
    }

    NoteSequence *Note::noteSequence() const {
        Q_D(const Note);
        return d->sequence;
    }

    opendspx::Note Note::toOpenDSPX() const {
        Q_D(const Note);
        opendspx::Note target {
            .pos = position(),
            .length = length(),
            .keyNum = keyNumber(),
            .centShift = centShift(),
            .language = language().toStdString(),
            .lyric = lyric().toStdString(),
            .pronunciation = {
                .original = originalPronunciation().toStdString(),
                .edited = editedPronunciation().toStdString(),
            },
            .phonemes = {
                .original = originalPhonemes()->toOpenDSPX(),
                .edited = editedPhonemes()->toOpenDSPX(),
            },
            .vibrato = {
                .start = vibratoStart(),
                .end = vibratoEnd(),
                .amp = vibratoAmplitude(),
                .freq = vibratoFrequency(),
                .phase = vibratoPhase(),
                .offset = vibratoOffset(),
                .points = {
                    .amp = vibratoAmplitudeControlPoints()->toOpenDSPX(),
                    .freq = vibratoFrequencyControlPoints()->toOpenDSPX(),
                },
            },
        };
        target.workspace = conv::deserializeWorkspace(d->workspace());
        OpenDSPXConversion::convertNoteToOpenDSPX(this, target);
        return target;
    }

    void Note::fromOpenDSPX(const opendspx::Note &note) {
        Q_D(Note);
        d->setWorkspace(conv::serializeWorkspace(note.workspace));
        setPosition(note.pos);
        setLength(note.length);
        setKeyNumber(note.keyNum);
        setCentShift(note.centShift);
        setLanguage(QString::fromStdString(note.language));
        setLyric(QString::fromStdString(note.lyric));
        setOriginalPronunciation(QString::fromStdString(note.pronunciation.original));
        setEditedPronunciation(QString::fromStdString(note.pronunciation.edited));
        originalPhonemes()->fromOpenDSPX(note.phonemes.original);
        editedPhonemes()->fromOpenDSPX(note.phonemes.edited);
        setVibratoStart(note.vibrato.start);
        setVibratoEnd(note.vibrato.end);
        setVibratoAmplitude(note.vibrato.amp);
        setVibratoFrequency(note.vibrato.freq);
        setVibratoPhase(note.vibrato.phase);
        setVibratoOffset(note.vibrato.offset);
        vibratoAmplitudeControlPoints()->fromOpenDSPX(note.vibrato.points.amp);
        vibratoFrequencyControlPoints()->fromOpenDSPX(note.vibrato.points.freq);
        OpenDSPXConversion::convertNoteFromOpenDSPX(this, note);
    }

}


#include "moc_Note.cpp"
