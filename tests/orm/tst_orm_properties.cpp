#include <QJsonObject>
#include <QPointF>
#include <QVariant>
#include <QtTest/QtTest>

#include "orm_test_context.h"

#include <dspxmodelORM/AnchorNode.h>
#include <dspxmodelORM/AudioClip.h>
#include <dspxmodelORM/DynamicMixingAnchor.h>
#include <dspxmodelORM/FreeValueDataArray.h>
#include <dspxmodelORM/KeySignature.h>
#include <dspxmodelORM/Label.h>
#include <dspxmodelORM/MixedSinger.h>
#include <dspxmodelORM/Note.h>
#include <dspxmodelORM/Parameter.h>
#include <dspxmodelORM/Phoneme.h>
#include <dspxmodelORM/SingingClip.h>
#include <dspxmodelORM/SingleSinger.h>
#include <dspxmodelORM/Sources.h>
#include <dspxmodelORM/Tempo.h>
#include <dspxmodelORM/TimeSignature.h>
#include <dspxmodelORM/Track.h>
#include <dspxmodelORM/VibratoPointDataArray.h>

using namespace dspx;

class OrmPropertiesTest : public QObject {
    Q_OBJECT

private slots:
    void modelProperties();
    void timelineEntityProperties();
    void trackClipNoteAndPhonemeProperties();
    void parameterSourceSingerAndAnchorProperties();
};

void OrmPropertiesTest::modelProperties() {
    OrmTestContext context;

    context.withTransaction([&] {
        context.model.setProjectName(QStringLiteral("Property Test"));
        context.model.setProjectAuthor(QStringLiteral("Author"));
        context.model.setGlobalCentShift(-12);
        context.model.setMultiChannelOutput(true);
        context.model.setGain(1.25);
        context.model.setPan(0.5);
        context.model.setMute(true);
        context.model.setLoopEnabled(true);
        context.model.setLoopStart(1920);
        context.model.setLoopLength(960);
    });

    QCOMPARE(context.model.projectName(), QStringLiteral("Property Test"));
    QCOMPARE(context.model.projectAuthor(), QStringLiteral("Author"));
    QCOMPARE(context.model.globalCentShift(), -12);
    QCOMPARE(context.model.multiChannelOutput(), true);
    QCOMPARE(context.model.gain(), 1.25);
    QCOMPARE(context.model.pan(), 0.5);
    QCOMPARE(context.model.mute(), true);
    QCOMPARE(context.model.loopEnabled(), true);
    QCOMPARE(context.model.loopStart(), 1920);
    QCOMPARE(context.model.loopLength(), 960);
}

void OrmPropertiesTest::timelineEntityProperties() {
    OrmTestContext context;

    context.withTransaction([&] {
        auto *label = context.model.createLabel();
        context.verifyEntity(label);
        label->setPosition(480);
        label->setText(QStringLiteral("chorus"));
        QCOMPARE(label->position(), 480);
        QCOMPARE(label->text(), QStringLiteral("chorus"));

        auto *key = context.model.createKeySignature();
        context.verifyEntity(key);
        key->setPosition(960);
        key->setMode(2741);
        key->setTonality(7);
        key->setAccidentalType(KeySignature::Flat);
        QCOMPARE(key->position(), 960);
        QCOMPARE(key->mode(), 2741);
        QCOMPARE(key->tonality(), 7);
        QCOMPARE(key->accidentalType(), KeySignature::Flat);

        auto *tempo = context.model.createTempo();
        context.verifyEntity(tempo);
        tempo->setPosition(1440);
        tempo->setValue(132.5);
        QCOMPARE(tempo->position(), 1440);
        QCOMPARE(tempo->value(), 132.5);

        auto *time = context.model.createTimeSignature();
        context.verifyEntity(time);
        time->setIndex(8);
        time->setNumerator(6);
        time->setDenominator(8);
        QCOMPARE(time->index(), 8);
        QCOMPARE(time->numerator(), 6);
        QCOMPARE(time->denominator(), 8);
    });
}

void OrmPropertiesTest::trackClipNoteAndPhonemeProperties() {
    OrmTestContext context;

    context.withTransaction([&] {
        auto *track = context.model.createTrack();
        context.verifyEntity(track);
        track->setColorId(3);
        track->setHeight(72.0);
        track->setName(QStringLiteral("Harmony"));
        track->setGain(0.85);
        track->setPan(-0.4);
        track->setMute(false);
        track->setSolo(true);
        track->setRecord(false);
        QCOMPARE(track->colorId(), 3);
        QCOMPARE(track->height(), 72.0);
        QCOMPARE(track->name(), QStringLiteral("Harmony"));
        QCOMPARE(track->gain(), 0.85);
        QCOMPARE(track->pan(), -0.4);
        QCOMPARE(track->mute(), false);
        QCOMPARE(track->solo(), true);
        QCOMPARE(track->record(), false);

        auto *audioClip = context.model.createAudioClip();
        context.verifyEntity(audioClip);
        audioClip->setName(QStringLiteral("Backing Audio"));
        audioClip->setGain(0.7);
        audioClip->setPan(0.2);
        audioClip->setMute(true);
        audioClip->setPosition(240);
        audioClip->setLength(720);
        audioClip->setClipStart(120);
        audioClip->setClipLength(600);
        AudioPathInfo path;
        path.absoluteDir = QStringLiteral("D:/media");
        path.relativeDir = QStringLiteral("media");
        path.fileName = QStringLiteral("backing.flac");
        path.formatEntryClassName = QStringLiteral("Flac");
        path.userData = QVariant::fromValue(42);
        path.sha512 = QStringLiteral("abc");
        audioClip->setPath(path);
        QCOMPARE(audioClip->name(), QStringLiteral("Backing Audio"));
        QCOMPARE(audioClip->gain(), 0.7);
        QCOMPARE(audioClip->pan(), 0.2);
        QCOMPARE(audioClip->mute(), true);
        QCOMPARE(audioClip->position(), 240);
        QCOMPARE(audioClip->length(), 720);
        QCOMPARE(audioClip->clipStart(), 120);
        QCOMPARE(audioClip->clipLength(), 600);
        QVERIFY(audioClip->path() == path);

        auto *singingClip = context.model.createSingingClip();
        context.verifyEntity(singingClip);
        QVERIFY(singingClip->notes() != nullptr);
        QVERIFY(singingClip->parameters() != nullptr);

        auto *note = context.model.createNote();
        context.verifyEntity(note);
        note->setCentShift(-8);
        note->setKeyNumber(64);
        note->setLanguage(QStringLiteral("en"));
        note->setLength(360);
        note->setLyric(QStringLiteral("hello"));
        note->setPosition(120);
        note->setOriginalPronunciation(QStringLiteral("hh ax l ow"));
        note->setEditedPronunciation(QStringLiteral("h eh l ow"));
        note->setVibratoAmplitude(18);
        note->setVibratoEnd(0.9);
        note->setVibratoFrequency(6.0);
        note->setVibratoOffset(-20);
        note->setVibratoPhase(0.3);
        note->setVibratoStart(0.1);
        QCOMPARE(note->centShift(), -8);
        QCOMPARE(note->keyNumber(), 64);
        QCOMPARE(note->language(), QStringLiteral("en"));
        QCOMPARE(note->length(), 360);
        QCOMPARE(note->lyric(), QStringLiteral("hello"));
        QCOMPARE(note->position(), 120);
        QCOMPARE(note->originalPronunciation(), QStringLiteral("hh ax l ow"));
        QCOMPARE(note->editedPronunciation(), QStringLiteral("h eh l ow"));
        QCOMPARE(note->vibratoAmplitude(), 18);
        QCOMPARE(note->vibratoEnd(), 0.9);
        QCOMPARE(note->vibratoFrequency(), 6.0);
        QCOMPARE(note->vibratoOffset(), -20);
        QCOMPARE(note->vibratoPhase(), 0.3);
        QCOMPARE(note->vibratoStart(), 0.1);

        auto *phoneme = context.model.createPhoneme();
        context.verifyEntity(phoneme);
        phoneme->setLanguage(QStringLiteral("en"));
        phoneme->setStart(80);
        phoneme->setToken(QStringLiteral("h"));
        phoneme->setOnset(true);
        QCOMPARE(phoneme->language(), QStringLiteral("en"));
        QCOMPARE(phoneme->start(), 80);
        QCOMPARE(phoneme->token(), QStringLiteral("h"));
        QCOMPARE(phoneme->onset(), true);
    });
}

void OrmPropertiesTest::parameterSourceSingerAndAnchorProperties() {
    OrmTestContext context;

    context.withTransaction([&] {
        auto *parameter = context.model.createParameter();
        context.verifyEntity(parameter);
        QVERIFY(parameter->original() != nullptr);
        QVERIFY(parameter->freeTransform() != nullptr);
        QVERIFY(parameter->freeEdited() != nullptr);
        QVERIFY(parameter->anchorTransform() != nullptr);
        QVERIFY(parameter->anchorEdited() != nullptr);

        QVERIFY(parameter->original()->splice(0, 0, QList<QVariant> {1, QVariant(), 3}));
        QCOMPARE(parameter->original()->items().size(), 3);

        auto *anchorNode = context.model.createAnchorNode();
        context.verifyEntity(anchorNode);
        anchorNode->setInterpolationMode(AnchorNode::Linear);
        anchorNode->setX(240);
        anchorNode->setY(-12);
        QCOMPARE(anchorNode->interpolationMode(), AnchorNode::Linear);
        QCOMPARE(anchorNode->x(), 240);
        QCOMPARE(anchorNode->y(), -12);

        auto *note = context.model.createNote();
        context.verifyEntity(note);
        QVERIFY(note->vibratoAmplitudeControlPoints()->splice(0, 0, QList<QPointF> {QPointF(0.0, 0.5)}));
        QCOMPARE(note->vibratoAmplitudeControlPoints()->items().size(), 1);

        auto *sources = context.model.createSources();
        context.verifyEntity(sources);
        sources->setCategory(QStringLiteral("lead"));
        QCOMPARE(sources->category(), QStringLiteral("lead"));
        QVERIFY(sources->singers() != nullptr);
        QVERIFY(sources->dynamicMixingAnchors() != nullptr);

        auto *singleSinger = context.model.createSingleSinger();
        context.verifyEntity(singleSinger);
        singleSinger->setExtra(QJsonObject {{"vendor", QStringLiteral("test")}});
        singleSinger->setId(QStringLiteral("single-1"));
        QCOMPARE(singleSinger->type(), Singer::Single);
        QCOMPARE(singleSinger->extra().toObject().value(QStringLiteral("vendor")).toString(), QStringLiteral("test"));
        QCOMPARE(singleSinger->id(), QStringLiteral("single-1"));

        auto *mixedSinger = context.model.createMixedSinger();
        context.verifyEntity(mixedSinger);
        mixedSinger->setRatio(QList<double> {0.2, 0.3});
        QCOMPARE(mixedSinger->type(), Singer::Mixed);
        QCOMPARE(mixedSinger->ratio(), QList<double>({0.2, 0.3}));
        QVERIFY(mixedSinger->singers() != nullptr);

        auto *mixingAnchor = context.model.createDynamicMixingAnchor();
        context.verifyEntity(mixingAnchor);
        mixingAnchor->setPosition(360);
        mixingAnchor->setRatio(QList<double> {0.6});
        QCOMPARE(mixingAnchor->position(), 360);
        QCOMPARE(mixingAnchor->ratio(), QList<double>({0.6}));
    });
}

QTEST_GUILESS_MAIN(OrmPropertiesTest)

#include "tst_orm_properties.moc"
