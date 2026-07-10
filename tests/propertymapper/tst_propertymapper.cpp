#include <QMetaType>
#include <QVariant>
#include <QtTest/QtTest>

#include "orm_test_context.h"

#include <dspxmodelORM/AnchorNode.h>
#include <dspxmodelORM/AnchorNodeSequence.h>
#include <dspxmodelORM/Clip.h>
#include <dspxmodelORM/ClipSequence.h>
#include <dspxmodelORM/KeySignature.h>
#include <dspxmodelORM/KeySignatureSequence.h>
#include <dspxmodelORM/Label.h>
#include <dspxmodelORM/LabelSequence.h>
#include <dspxmodelORM/Model.h>
#include <dspxmodelORM/Note.h>
#include <dspxmodelORM/NoteSequence.h>
#include <dspxmodelORM/Parameter.h>
#include <dspxmodelORM/ParameterMap.h>
#include <dspxmodelORM/Phoneme.h>
#include <dspxmodelORM/PhonemeSequence.h>
#include <dspxmodelORM/SingingClip.h>
#include <dspxmodelORM/Tempo.h>
#include <dspxmodelORM/TempoSequence.h>
#include <dspxmodelORM/Track.h>
#include <dspxmodelORM/TrackList.h>
#include <dspxmodelPropertyMapper/ClipPropertyMapper.h>
#include <dspxmodelPropertyMapper/KeySignaturePropertyMapper.h>
#include <dspxmodelPropertyMapper/LabelPropertyMapper.h>
#include <dspxmodelPropertyMapper/NotePropertyMapper.h>
#include <dspxmodelPropertyMapper/TempoPropertyMapper.h>
#include <dspxmodelPropertyMapper/TrackPropertyMapper.h>
#include <dspxmodelSelectionModel/SelectionModel.h>

using namespace dspx;

namespace {

constexpr auto SelectCurrent = SelectionModel::Select | SelectionModel::SetCurrentItem;
constexpr auto SelectOnlyCurrent = SelectionModel::Select | SelectionModel::ClearPreviousSelection | SelectionModel::SetCurrentItem;

QVariant mappedValue(const QVariant &value) {
    if (value.metaType() == QMetaType::fromType<QVariant>()) {
        return value.value<QVariant>();
    }
    return value;
}

template <typename T>
T mappedAs(const QVariant &value) {
    return mappedValue(value).value<T>();
}

bool mappedInvalid(const QVariant &value) {
    return !mappedValue(value).isValid();
}

void selectOnly(SelectionModel &selection, QObject *item) {
    selection.select(item, SelectOnlyCurrent);
}

struct ClipFixture {
    Track *track = nullptr;
    SingingClip *clip = nullptr;
    Note *note = nullptr;
    Parameter *parameter = nullptr;
    AnchorNode *anchor = nullptr;
};

ClipFixture createClipFixture(OrmTestContext &context) {
    ClipFixture fixture;
    context.withTransaction([&] {
        fixture.track = context.model.createTrack();
        fixture.clip = context.model.createSingingClip();
        fixture.note = context.model.createNote();
        fixture.parameter = context.model.createParameter();
        fixture.anchor = context.model.createAnchorNode();

        fixture.track->setName(QStringLiteral("Lead"));
        fixture.track->setColorId(4);
        fixture.track->setHeight(80.0);
        fixture.track->setMute(false);
        fixture.track->setSolo(true);
        fixture.track->setRecord(false);
        fixture.track->setGain(0.8);
        fixture.track->setPan(-0.25);

        fixture.clip->setName(QStringLiteral("Verse"));
        fixture.clip->setGain(0.75);
        fixture.clip->setPan(0.5);
        fixture.clip->setMute(true);
        fixture.clip->setPosition(240);
        fixture.clip->setClipStart(60);
        fixture.clip->setClipLength(720);
        fixture.clip->setLength(840);

        fixture.note->setLanguage(QStringLiteral("ja"));
        fixture.note->setLyric(QStringLiteral("la"));
        fixture.note->setKeyNumber(60);
        fixture.note->setPosition(120);
        fixture.note->setLength(360);

        fixture.anchor->setX(120);
        fixture.anchor->setY(12);
        fixture.anchor->setInterpolationMode(AnchorNode::Linear);

        QVERIFY(context.model.tracks()->insertItem(0, fixture.track));
        QVERIFY(fixture.track->clips()->insertItem(fixture.clip));
        QVERIFY(fixture.clip->notes()->insertItem(fixture.note));
        QVERIFY(fixture.parameter->anchorTransform()->insertItem(fixture.anchor));
        QVERIFY(fixture.clip->parameters()->insertItem(QStringLiteral("pitch"), fixture.parameter));
    });
    return fixture;
}

void verifyClipMapper(ClipPropertyMapper &mapper, SingingClip *clip, Track *track) {
    QCOMPARE(mappedValue(mapper.name()).toString(), clip->name());
    QCOMPARE(mappedAs<Clip::ClipType>(mapper.type()), Clip::Singing);
    QCOMPARE(mappedAs<Track *>(mapper.associatedTrack()), track);
    QCOMPARE(mappedValue(mapper.mute()).toBool(), clip->mute());
    QCOMPARE(mappedValue(mapper.gain()).toDouble(), clip->gain());
    QCOMPARE(mappedValue(mapper.pan()).toDouble(), clip->pan());
    QCOMPARE(mappedValue(mapper.position()).toInt(), clip->position());
    QCOMPARE(mappedValue(mapper.clipStart()).toInt(), clip->clipStart());
    QCOMPARE(mappedValue(mapper.clipLength()).toInt(), clip->clipLength());
    QCOMPARE(mappedValue(mapper.length()).toInt(), clip->length());
}

void verifyTrackMapper(TrackPropertyMapper &mapper, Track *track) {
    QCOMPARE(mappedValue(mapper.name()).toString(), track->name());
    QCOMPARE(mappedValue(mapper.colorId()).toInt(), track->colorId());
    QCOMPARE(mappedValue(mapper.height()).toDouble(), track->height());
    QCOMPARE(mappedValue(mapper.mute()).toBool(), track->mute());
    QCOMPARE(mappedValue(mapper.solo()).toBool(), track->solo());
    QCOMPARE(mappedValue(mapper.record()).toBool(), track->record());
    QCOMPARE(mappedValue(mapper.gain()).toDouble(), track->gain());
    QCOMPARE(mappedValue(mapper.pan()).toDouble(), track->pan());
}

}

class PropertyMapperTest : public QObject {
    Q_OBJECT

private slots:
    void noteMapperUnifiedAndDivergentValues();
    void noteMapperWritesSelectedNotes();
    void clipMapperFromClipNoteAndAnchorSelection();
    void clipMapperAssociatedTrackMovesClip();
    void trackMapperFromTrackClipNoteAndAnchorSelection();
    void simpleTimelineMappers();
    void destroyedConnectionDoesNotAccumulate();
};

void PropertyMapperTest::noteMapperUnifiedAndDivergentValues() {
    OrmTestContext context;
    SelectionModel selection(&context.model);
    NotePropertyMapper mapper;
    mapper.setSelectionModel(&selection);

    Track *track = nullptr;
    SingingClip *clip = nullptr;
    Note *note1 = nullptr;
    Note *note2 = nullptr;

    context.withTransaction([&] {
        track = context.model.createTrack();
        clip = context.model.createSingingClip();
        note1 = context.model.createNote();
        note2 = context.model.createNote();

        note1->setLanguage(QStringLiteral("ja"));
        note2->setLanguage(QStringLiteral("ja"));
        note1->setLyric(QStringLiteral("la"));
        note2->setLyric(QStringLiteral("la"));
        note1->setKeyNumber(60);
        note2->setKeyNumber(64);
        note1->setPosition(0);
        note2->setPosition(480);

        QVERIFY(context.model.tracks()->insertItem(0, track));
        QVERIFY(track->clips()->insertItem(clip));
        QVERIFY(clip->notes()->insertItem(note1));
        QVERIFY(clip->notes()->insertItem(note2));

        auto *edited1 = context.model.createPhoneme();
        auto *edited2 = context.model.createPhoneme();
        auto *original1 = context.model.createPhoneme();
        auto *original2 = context.model.createPhoneme();
        QVERIFY(note1->editedPhonemes()->insertItem(edited1));
        QVERIFY(note2->editedPhonemes()->insertItem(edited2));
        QVERIFY(note1->originalPhonemes()->insertItem(original1));
        QVERIFY(note2->originalPhonemes()->insertItem(original2));
    });

    selection.select(note1, SelectCurrent);
    selection.select(note2, SelectionModel::Select);

    QCOMPARE(mappedValue(mapper.language()).toString(), QStringLiteral("ja"));
    QCOMPARE(mappedValue(mapper.lyric()).toString(), QStringLiteral("la"));
    QVERIFY(mappedInvalid(mapper.keyNumber()));
    QVERIFY(mappedInvalid(mapper.position()));
    QCOMPARE(mappedAs<SingingClip *>(mapper.singingClip()), clip);
    QVERIFY(mappedInvalid(mapper.editedPhonemes()));
    QVERIFY(mappedInvalid(mapper.originalPhonemes()));

    selection.select(note2, SelectionModel::Deselect);

    QCOMPARE(mappedAs<SingingClip *>(mapper.singingClip()), clip);
    QCOMPARE(mappedAs<PhonemeSequence *>(mapper.editedPhonemes()), note1->editedPhonemes());
    QCOMPARE(mappedAs<PhonemeSequence *>(mapper.originalPhonemes()), note1->originalPhonemes());
}

void PropertyMapperTest::noteMapperWritesSelectedNotes() {
    OrmTestContext context;
    SelectionModel selection(&context.model);
    NotePropertyMapper mapper;
    mapper.setSelectionModel(&selection);

    Track *track = nullptr;
    SingingClip *clip = nullptr;
    Note *note1 = nullptr;
    Note *note2 = nullptr;

    context.withTransaction([&] {
        track = context.model.createTrack();
        clip = context.model.createSingingClip();
        note1 = context.model.createNote();
        note2 = context.model.createNote();

        note1->setLyric(QStringLiteral("old"));
        note2->setLyric(QStringLiteral("old"));
        note1->setKeyNumber(60);
        note2->setKeyNumber(62);
        note1->setPosition(0);
        note2->setPosition(480);
        note1->setOriginalPronunciation(QStringLiteral("o"));
        note2->setOriginalPronunciation(QStringLiteral("o"));
        note1->setEditedPronunciation(QStringLiteral("e"));
        note2->setEditedPronunciation(QStringLiteral("e"));

        QVERIFY(context.model.tracks()->insertItem(0, track));
        QVERIFY(track->clips()->insertItem(clip));
        QVERIFY(clip->notes()->insertItem(note1));
        QVERIFY(clip->notes()->insertItem(note2));
    });

    selection.select(note1, SelectCurrent);
    selection.select(note2, SelectionModel::Select);

    QSignalSpy lyricSpy(&mapper, &NotePropertyMapper::lyricChanged);
    QSignalSpy keyNumberSpy(&mapper, &NotePropertyMapper::keyNumberChanged);
    QSignalSpy originalPronunciationSpy(&mapper, &NotePropertyMapper::originalPronunciationChanged);
    QSignalSpy editedPronunciationSpy(&mapper, &NotePropertyMapper::editedPronunciationChanged);

    context.withTransaction([&] {
        mapper.setLyric(QStringLiteral("new"));
        mapper.setKeyNumber(67);
        mapper.setOriginalPronunciation(QStringLiteral("orig"));
        mapper.setEditedPronunciation(QStringLiteral("edit"));
    });

    QCOMPARE(note1->lyric(), QStringLiteral("new"));
    QCOMPARE(note2->lyric(), QStringLiteral("new"));
    QCOMPARE(note1->keyNumber(), 67);
    QCOMPARE(note2->keyNumber(), 67);
    QCOMPARE(note1->originalPronunciation(), QStringLiteral("orig"));
    QCOMPARE(note2->originalPronunciation(), QStringLiteral("orig"));
    QCOMPARE(note1->editedPronunciation(), QStringLiteral("edit"));
    QCOMPARE(note2->editedPronunciation(), QStringLiteral("edit"));
    QVERIFY(lyricSpy.count() >= 1);
    QVERIFY(keyNumberSpy.count() >= 1);
    QVERIFY(originalPronunciationSpy.count() >= 1);
    QVERIFY(editedPronunciationSpy.count() >= 1);
}

void PropertyMapperTest::clipMapperFromClipNoteAndAnchorSelection() {
    OrmTestContext context;
    SelectionModel selection(&context.model);
    ClipPropertyMapper mapper;
    mapper.setSelectionModel(&selection);
    const auto fixture = createClipFixture(context);

    selectOnly(selection, fixture.clip);
    verifyClipMapper(mapper, fixture.clip, fixture.track);

    selectOnly(selection, fixture.note);
    verifyClipMapper(mapper, fixture.clip, fixture.track);

    selectOnly(selection, fixture.anchor);
    verifyClipMapper(mapper, fixture.clip, fixture.track);
}

void PropertyMapperTest::clipMapperAssociatedTrackMovesClip() {
    OrmTestContext context;
    SelectionModel selection(&context.model);
    ClipPropertyMapper mapper;
    mapper.setSelectionModel(&selection);

    Track *track1 = nullptr;
    Track *track2 = nullptr;
    SingingClip *clip = nullptr;

    context.withTransaction([&] {
        track1 = context.model.createTrack();
        track2 = context.model.createTrack();
        clip = context.model.createSingingClip();
        QVERIFY(context.model.tracks()->insertItem(0, track1));
        QVERIFY(context.model.tracks()->insertItem(1, track2));
        QVERIFY(track1->clips()->insertItem(clip));
    });

    selectOnly(selection, clip);
    QCOMPARE(mappedAs<Track *>(mapper.associatedTrack()), track1);

    context.withTransaction([&] {
        mapper.setAssociatedTrack(QVariant::fromValue(track2));
    });

    QVERIFY(!track1->clips()->contains(clip));
    QVERIFY(track2->clips()->contains(clip));
    QCOMPARE(clip->clipSequence(), track2->clips());
    QCOMPARE(mappedAs<Track *>(mapper.associatedTrack()), track2);
}

void PropertyMapperTest::trackMapperFromTrackClipNoteAndAnchorSelection() {
    OrmTestContext context;
    SelectionModel selection(&context.model);
    TrackPropertyMapper mapper;
    mapper.setSelectionModel(&selection);
    const auto fixture = createClipFixture(context);

    selectOnly(selection, fixture.track);
    verifyTrackMapper(mapper, fixture.track);

    selectOnly(selection, fixture.clip);
    verifyTrackMapper(mapper, fixture.track);

    selectOnly(selection, fixture.note);
    verifyTrackMapper(mapper, fixture.track);

    selectOnly(selection, fixture.anchor);
    verifyTrackMapper(mapper, fixture.track);
}

void PropertyMapperTest::simpleTimelineMappers() {
    OrmTestContext context;
    SelectionModel selection(&context.model);
    LabelPropertyMapper labelMapper;
    TempoPropertyMapper tempoMapper;
    KeySignaturePropertyMapper keySignatureMapper;
    labelMapper.setSelectionModel(&selection);
    tempoMapper.setSelectionModel(&selection);
    keySignatureMapper.setSelectionModel(&selection);

    Label *label1 = nullptr;
    Label *label2 = nullptr;
    Tempo *tempo1 = nullptr;
    Tempo *tempo2 = nullptr;
    KeySignature *key1 = nullptr;
    KeySignature *key2 = nullptr;

    context.withTransaction([&] {
        label1 = context.model.createLabel();
        label2 = context.model.createLabel();
        label1->setPosition(0);
        label2->setPosition(480);
        label1->setText(QStringLiteral("mark"));
        label2->setText(QStringLiteral("mark"));
        QVERIFY(context.model.labels()->insertItem(label1));
        QVERIFY(context.model.labels()->insertItem(label2));

        tempo1 = context.model.createTempo();
        tempo2 = context.model.createTempo();
        tempo1->setPosition(0);
        tempo2->setPosition(480);
        tempo1->setValue(120.0);
        tempo2->setValue(120.0);
        QVERIFY(context.model.tempos()->insertItem(tempo1));
        QVERIFY(context.model.tempos()->insertItem(tempo2));

        key1 = context.model.createKeySignature();
        key2 = context.model.createKeySignature();
        key1->setPosition(0);
        key2->setPosition(480);
        key1->setMode(2741);
        key2->setMode(2741);
        key1->setTonality(0);
        key2->setTonality(0);
        key1->setAccidentalType(KeySignature::Flat);
        key2->setAccidentalType(KeySignature::Flat);
        QVERIFY(context.model.keySignatures()->insertItem(key1));
        QVERIFY(context.model.keySignatures()->insertItem(key2));
    });

    selection.select(label1, SelectCurrent);
    selection.select(label2, SelectionModel::Select);
    QCOMPARE(mappedValue(labelMapper.text()).toString(), QStringLiteral("mark"));
    QVERIFY(mappedInvalid(labelMapper.position()));
    context.withTransaction([&] {
        labelMapper.setText(QStringLiteral("marker"));
    });
    QCOMPARE(label1->text(), QStringLiteral("marker"));
    QCOMPARE(label2->text(), QStringLiteral("marker"));
    selectOnly(selection, label1);
    QCOMPARE(mappedValue(labelMapper.position()).toInt(), 0);
    context.withTransaction([&] {
        labelMapper.setPosition(960);
    });
    QCOMPARE(label1->position(), 960);

    selection.select(tempo1, SelectCurrent);
    selection.select(tempo2, SelectionModel::Select);
    QCOMPARE(mappedValue(tempoMapper.value()).toDouble(), 120.0);
    QVERIFY(mappedInvalid(tempoMapper.position()));
    context.withTransaction([&] {
        tempoMapper.setValue(144.0);
    });
    QCOMPARE(tempo1->value(), 144.0);
    QCOMPARE(tempo2->value(), 144.0);
    selectOnly(selection, tempo1);
    QCOMPARE(mappedValue(tempoMapper.position()).toInt(), 0);
    context.withTransaction([&] {
        tempoMapper.setPosition(960);
    });
    QCOMPARE(tempo1->position(), 960);

    selection.select(key1, SelectCurrent);
    selection.select(key2, SelectionModel::Select);
    QCOMPARE(mappedValue(keySignatureMapper.mode()).toInt(), 2741);
    QCOMPARE(mappedValue(keySignatureMapper.tonality()).toInt(), 0);
    QCOMPARE(mappedAs<KeySignature::AccidentalType>(keySignatureMapper.accidentalType()), KeySignature::Flat);
    QVERIFY(mappedInvalid(keySignatureMapper.position()));
    context.withTransaction([&] {
        keySignatureMapper.setMode(1453);
        keySignatureMapper.setTonality(7);
        keySignatureMapper.setAccidentalType(QVariant::fromValue(KeySignature::Sharp));
    });
    QCOMPARE(key1->mode(), 1453);
    QCOMPARE(key2->mode(), 1453);
    QCOMPARE(key1->tonality(), 7);
    QCOMPARE(key2->tonality(), 7);
    QCOMPARE(key1->accidentalType(), KeySignature::Sharp);
    QCOMPARE(key2->accidentalType(), KeySignature::Sharp);
    selectOnly(selection, key1);
    QCOMPARE(mappedValue(keySignatureMapper.position()).toInt(), 0);
    context.withTransaction([&] {
        keySignatureMapper.setPosition(960);
    });
    QCOMPARE(key1->position(), 960);
}

void PropertyMapperTest::destroyedConnectionDoesNotAccumulate() {
    OrmTestContext context;
    SelectionModel selection(&context.model);
    ClipPropertyMapper mapper;
    mapper.setSelectionModel(&selection);

    Track *track = nullptr;
    SingingClip *clip = nullptr;

    context.withTransaction([&] {
        track = context.model.createTrack();
        clip = context.model.createSingingClip();
        clip->setName(QStringLiteral("Initial"));
        QVERIFY(context.model.tracks()->insertItem(0, track));
        QVERIFY(track->clips()->insertItem(clip));
    });

    QSignalSpy nameSpy(&mapper, &ClipPropertyMapper::nameChanged);

    for (int i = 0; i < 5; ++i) {
        selection.select(clip, SelectCurrent);
        selection.select(clip, SelectionModel::Deselect);
    }

    selection.select(clip, SelectCurrent);
    nameSpy.clear();
    context.withTransaction([&] {
        clip->setName(QStringLiteral("Once"));
    });
    QCOMPARE(nameSpy.count(), 1);

    selection.select(clip, SelectionModel::Deselect);
    nameSpy.clear();
    context.withTransaction([&] {
        clip->setName(QStringLiteral("After Deselect"));
    });
    QCOMPARE(nameSpy.count(), 0);
}

QTEST_GUILESS_MAIN(PropertyMapperTest)

#include "tst_propertymapper.moc"
