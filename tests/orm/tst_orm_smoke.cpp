#include <functional>
#include <utility>

#include <QJsonObject>
#include <QPointF>
#include <QVariant>
#include <QtTest/QtTest>

#include <dini/engine.h>
#include <dini/transaction.h>

#include <dspxmodelCore/Document.h>

#include <dspxmodelORM/AnchorNode.h>
#include <dspxmodelORM/AnchorNodeSequence.h>
#include <dspxmodelORM/AudioClip.h>
#include <dspxmodelORM/ClipSequence.h>
#include <dspxmodelORM/DynamicMixingAnchor.h>
#include <dspxmodelORM/DynamicMixingAnchorSequence.h>
#include <dspxmodelORM/EntityObject.h>
#include <dspxmodelORM/FreeValueDataArray.h>
#include <dspxmodelORM/KeySignature.h>
#include <dspxmodelORM/KeySignatureSequence.h>
#include <dspxmodelORM/Label.h>
#include <dspxmodelORM/LabelSequence.h>
#include <dspxmodelORM/MixedSinger.h>
#include <dspxmodelORM/Model.h>
#include <dspxmodelORM/Note.h>
#include <dspxmodelORM/NoteSequence.h>
#include <dspxmodelORM/Parameter.h>
#include <dspxmodelORM/ParameterMap.h>
#include <dspxmodelORM/Phoneme.h>
#include <dspxmodelORM/PhonemeSequence.h>
#include <dspxmodelORM/SingerList.h>
#include <dspxmodelORM/SingingClip.h>
#include <dspxmodelORM/SingleSinger.h>
#include <dspxmodelORM/Sources.h>
#include <dspxmodelORM/Tempo.h>
#include <dspxmodelORM/TempoSequence.h>
#include <dspxmodelORM/TimeSignature.h>
#include <dspxmodelORM/TimeSignatureSequence.h>
#include <dspxmodelORM/Track.h>
#include <dspxmodelORM/TrackList.h>
#include <dspxmodelORM/VibratoPointDataArray.h>

using namespace dspx;

class OrmSmokeTest : public QObject {
    Q_OBJECT

private slots:
    void modelPropertiesAndRootContainers();
    void timelineEntitiesAndSequences();
    void trackClipNoteAndPhonemeGraph();
    void parametersAndDataContainers();
    void sourcesSingersAndDynamicMixing();
    void destroyItem();

private:
    template <typename Func>
    void withTransaction(Document &document, Func &&func)
    {
        auto transaction = document.engine()->beginTransaction();
        document.setTransaction(&transaction);
        std::invoke(std::forward<Func>(func));
        transaction.commit();
        document.setTransaction(nullptr);
    }

    void verifyEntity(EntityObject *entity, Model *model)
    {
        QVERIFY(entity != nullptr);
        QCOMPARE(entity->model(), model);
        QVERIFY(entity->handle());
    }
};

void OrmSmokeTest::modelPropertiesAndRootContainers()
{
    Document document;
    Model model(&document);

    withTransaction(document, [&] {
        model.setProjectName(QStringLiteral("Smoke Project"));
        model.setProjectAuthor(QStringLiteral("Qt Test"));
        model.setGlobalCentShift(12);
        model.setMultiChannelOutput(true);
        model.setGain(0.75);
        model.setPan(-0.25);
        model.setMute(true);
        model.setLoopEnabled(true);
        model.setLoopStart(120);
        model.setLoopLength(480);
    });

    QCOMPARE(model.document(), &document);
    QCOMPARE(model.projectName(), QStringLiteral("Smoke Project"));
    QCOMPARE(model.projectAuthor(), QStringLiteral("Qt Test"));
    QCOMPARE(model.globalCentShift(), 12);
    QCOMPARE(model.multiChannelOutput(), true);
    QCOMPARE(model.gain(), 0.75);
    QCOMPARE(model.pan(), -0.25);
    QCOMPARE(model.mute(), true);
    QCOMPARE(model.loopEnabled(), true);
    QCOMPARE(model.loopStart(), 120);
    QCOMPARE(model.loopLength(), 480);

    QVERIFY(model.labels() != nullptr);
    QVERIFY(model.keySignatures() != nullptr);
    QVERIFY(model.tempos() != nullptr);
    QVERIFY(model.timeSignatures() != nullptr);
    QVERIFY(model.tracks() != nullptr);
}

void OrmSmokeTest::timelineEntitiesAndSequences()
{
    Document document;
    Model model(&document);

    withTransaction(document, [&] {
        auto *label1 = model.createLabel();
        auto *label2 = model.createLabel();
        verifyEntity(label1, &model);
        verifyEntity(label2, &model);
        label1->setPosition(10);
        label1->setText(QStringLiteral("intro"));
        label2->setPosition(20);
        label2->setText(QStringLiteral("verse"));

        auto *labels = model.labels();
        QCOMPARE(labels->model(), &model);
        QVERIFY(labels->insertItem(label1));
        QVERIFY(labels->insertItem(label2));
        QCOMPARE(labels->size(), 2);
        QCOMPARE(labels->firstItem(), label1);
        QCOMPARE(labels->lastItem(), label2);
        QVERIFY(labels->contains(label1));
        QCOMPARE(labels->slice(0, 30).size(), 2);
        QCOMPARE(label1->labelSequence(), labels);
        (void) label1->previousItem();
        (void) label1->nextItem();
        QVERIFY(labels->removeItem(label2));
        QVERIFY(labels->insertItem(label2));

        auto *key1 = model.createKeySignature();
        auto *key2 = model.createKeySignature();
        verifyEntity(key1, &model);
        verifyEntity(key2, &model);
        key1->setPosition(0);
        key1->setMode(2741);
        key1->setTonality(0);
        key1->setAccidentalType(KeySignature::Sharp);
        key2->setPosition(480);
        key2->setMode(1453);
        key2->setTonality(5);
        key2->setAccidentalType(KeySignature::Flat);

        auto *keys = model.keySignatures();
        QCOMPARE(keys->model(), &model);
        QVERIFY(keys->insertItem(key1));
        QVERIFY(keys->insertItem(key2));
        QCOMPARE(keys->size(), 2);
        QCOMPARE(keys->firstItem(), key1);
        QCOMPARE(keys->lastItem(), key2);
        QVERIFY(keys->contains(key1));
        QCOMPARE(keys->slice(0, 960).size(), 2);
        QCOMPARE(key1->position(), 0);
        QCOMPARE(key1->mode(), 2741);
        QCOMPARE(key1->tonality(), 0);
        QCOMPARE(key1->accidentalType(), KeySignature::Sharp);
        QCOMPARE(key1->keySignatureSequence(), keys);
        (void) key1->previousItem();
        (void) key1->nextItem();
        QVERIFY(keys->removeItem(key2));
        QVERIFY(keys->insertItem(key2));

        auto *tempo1 = model.createTempo();
        auto *tempo2 = model.createTempo();
        verifyEntity(tempo1, &model);
        verifyEntity(tempo2, &model);
        tempo1->setPosition(0);
        tempo1->setValue(120.0);
        tempo2->setPosition(960);
        tempo2->setValue(150.0);

        auto *tempos = model.tempos();
        QCOMPARE(tempos->model(), &model);
        QVERIFY(tempos->insertItem(tempo1));
        QVERIFY(tempos->insertItem(tempo2));
        QCOMPARE(tempos->size(), 2);
        QCOMPARE(tempos->firstItem(), tempo1);
        QCOMPARE(tempos->lastItem(), tempo2);
        QVERIFY(tempos->contains(tempo1));
        QCOMPARE(tempos->slice(0, 1200).size(), 2);
        QCOMPARE(tempo1->position(), 0);
        QCOMPARE(tempo1->value(), 120.0);
        QCOMPARE(tempo1->tempoSequence(), tempos);
        (void) tempo1->previousItem();
        (void) tempo1->nextItem();
        QVERIFY(tempos->removeItem(tempo2));
        QVERIFY(tempos->insertItem(tempo2));

        auto *time1 = model.createTimeSignature();
        auto *time2 = model.createTimeSignature();
        verifyEntity(time1, &model);
        verifyEntity(time2, &model);
        time1->setIndex(0);
        time1->setNumerator(4);
        time1->setDenominator(4);
        time2->setIndex(4);
        time2->setNumerator(3);
        time2->setDenominator(8);

        auto *times = model.timeSignatures();
        QCOMPARE(times->model(), &model);
        QVERIFY(times->insertItem(time1));
        QVERIFY(times->insertItem(time2));
        QCOMPARE(times->size(), 2);
        QCOMPARE(times->firstItem(), time1);
        QCOMPARE(times->lastItem(), time2);
        QVERIFY(times->contains(time1));
        QCOMPARE(times->slice(0, 8).size(), 2);
        QCOMPARE(time1->index(), 0);
        QCOMPARE(time1->numerator(), 4);
        QCOMPARE(time1->denominator(), 4);
        QCOMPARE(time1->timeSignatureSequence(), times);
        (void) time1->previousItem();
        (void) time1->nextItem();
        QVERIFY(times->removeItem(time2));
        QVERIFY(times->insertItem(time2));
    });
}

void OrmSmokeTest::trackClipNoteAndPhonemeGraph()
{
    Document document;
    Model model(&document);

    withTransaction(document, [&] {
        auto *track1 = model.createTrack();
        auto *track2 = model.createTrack();
        auto *track3 = model.createTrack();
        verifyEntity(track1, &model);
        verifyEntity(track2, &model);
        verifyEntity(track3, &model);

        track1->setColorId(1);
        track1->setHeight(64.0);
        track1->setName(QStringLiteral("Lead"));
        track1->setGain(0.9);
        track1->setPan(0.1);
        track1->setMute(false);
        track1->setSolo(true);
        track1->setRecord(true);

        auto *tracks = model.tracks();
        QCOMPARE(tracks->model(), &model);
        QVERIFY(tracks->insertItem(0, track1));
        QVERIFY(tracks->insertItem(1, track2));
        QVERIFY(tracks->insertItem(2, track3));
        QCOMPARE(tracks->size(), 3);
        QCOMPARE(tracks->items().size(), 3);
        QVERIFY(tracks->contains(track1));
        QCOMPARE(tracks->item(0), track1);
        QVERIFY(tracks->rotate(0, 1, 3));
        auto *removedTrack = tracks->item(2);
        QVERIFY(tracks->removeItem(2));
        QVERIFY(tracks->insertItem(2, removedTrack));

        QCOMPARE(track1->colorId(), 1);
        QCOMPARE(track1->height(), 64.0);
        QCOMPARE(track1->name(), QStringLiteral("Lead"));
        QCOMPARE(track1->gain(), 0.9);
        QCOMPARE(track1->pan(), 0.1);
        QCOMPARE(track1->mute(), false);
        QCOMPARE(track1->solo(), true);
        QCOMPARE(track1->record(), true);
        QVERIFY(track1->clips() != nullptr);
        QVERIFY(track1->trackList() != nullptr);

        auto *audioClip = model.createAudioClip();
        auto *singingClip1 = model.createSingingClip();
        auto *singingClip2 = model.createSingingClip();
        verifyEntity(audioClip, &model);
        verifyEntity(singingClip1, &model);
        verifyEntity(singingClip2, &model);

        audioClip->setName(QStringLiteral("Audio"));
        audioClip->setGain(0.8);
        audioClip->setPan(-0.2);
        audioClip->setMute(false);
        audioClip->setPosition(0);
        audioClip->setLength(480);
        audioClip->setClipStart(10);
        audioClip->setClipLength(470);
        AudioPathInfo path;
        path.absoluteDir = QStringLiteral("C:/audio");
        path.relativeDir = QStringLiteral("audio");
        path.fileName = QStringLiteral("take.wav");
        path.formatEntryClassName = QStringLiteral("Wave");
        path.userData = QStringLiteral("userdata");
        path.sha512 = QStringLiteral("hash");
        audioClip->setPath(path);

        singingClip1->setName(QStringLiteral("Vocal A"));
        singingClip1->setPosition(480);
        singingClip1->setLength(960);
        singingClip1->setClipStart(0);
        singingClip1->setClipLength(960);
        singingClip2->setName(QStringLiteral("Vocal B"));
        singingClip2->setPosition(1440);
        singingClip2->setLength(480);
        singingClip2->setClipStart(0);
        singingClip2->setClipLength(480);

        auto *clips1 = track1->clips();
        auto *clips2 = track2->clips();
        QCOMPARE(clips1->track(), track1);
        QVERIFY(clips1->insertItem(audioClip));
        QVERIFY(clips1->insertItem(singingClip1));
        QVERIFY(clips2->insertItem(singingClip2));
        QCOMPARE(clips1->size(), 2);
        QCOMPARE(clips1->firstItem(), audioClip);
        QCOMPARE(clips1->lastItem(), singingClip1);
        QVERIFY(clips1->contains(audioClip));
        QCOMPARE(clips1->slice(0, 2000).size(), 2);
        QVERIFY(clips1->moveItem(singingClip1, clips2));
        QVERIFY(clips2->moveItem(singingClip1, clips1));
        QVERIFY(clips1->removeItem(audioClip));
        QVERIFY(clips1->insertItem(audioClip));

        QCOMPARE(audioClip->name(), QStringLiteral("Audio"));
        QCOMPARE(audioClip->gain(), 0.8);
        QCOMPARE(audioClip->pan(), -0.2);
        QCOMPARE(audioClip->mute(), false);
        QCOMPARE(audioClip->position(), 0);
        QCOMPARE(audioClip->length(), 480);
        QCOMPARE(audioClip->clipStart(), 10);
        QCOMPARE(audioClip->clipLength(), 470);
        QCOMPARE(audioClip->start(), -10);
        QCOMPARE(audioClip->type(), Clip::Audio);
        QVERIFY(audioClip->path() == path);
        QVERIFY(audioClip->clipSequence() != nullptr);
        (void) audioClip->previousItem();
        (void) audioClip->nextItem();
        (void) audioClip->overlapped();

        QCOMPARE(singingClip1->type(), Clip::Singing);
        QVERIFY(singingClip1->notes() != nullptr);
        QVERIFY(singingClip1->parameters() != nullptr);

        auto *note1 = model.createNote();
        auto *note2 = model.createNote();
        auto *note3 = model.createNote();
        verifyEntity(note1, &model);
        verifyEntity(note2, &model);
        verifyEntity(note3, &model);

        note1->setCentShift(10);
        note1->setKeyNumber(60);
        note1->setLanguage(QStringLiteral("zh"));
        note1->setLength(240);
        note1->setLyric(QStringLiteral("la"));
        note1->setPosition(0);
        note1->setOriginalPronunciation(QStringLiteral("la0"));
        note1->setEditedPronunciation(QStringLiteral("la1"));
        note1->setVibratoAmplitude(30);
        note1->setVibratoEnd(0.8);
        note1->setVibratoFrequency(5.5);
        note1->setVibratoOffset(15);
        note1->setVibratoPhase(0.25);
        note1->setVibratoStart(0.2);
        note2->setPosition(240);
        note2->setLength(240);
        note3->setPosition(480);
        note3->setLength(240);

        auto *notes1 = singingClip1->notes();
        auto *notes2 = singingClip2->notes();
        QCOMPARE(notes1->singingClip(), singingClip1);
        QVERIFY(notes1->insertItem(note1));
        QVERIFY(notes1->insertItem(note2));
        QVERIFY(notes2->insertItem(note3));
        QCOMPARE(notes1->size(), 2);
        QCOMPARE(notes1->firstItem(), note1);
        QCOMPARE(notes1->lastItem(), note2);
        QVERIFY(notes1->contains(note1));
        QCOMPARE(notes1->slice(0, 500).size(), 2);
        QVERIFY(notes2->moveItem(note3, notes1));
        QVERIFY(notes1->moveItem(note3, notes2));
        QVERIFY(notes1->removeItem(note2));
        QVERIFY(notes1->insertItem(note2));

        QCOMPARE(note1->centShift(), 10);
        QCOMPARE(note1->keyNumber(), 60);
        QCOMPARE(note1->language(), QStringLiteral("zh"));
        QCOMPARE(note1->length(), 240);
        QCOMPARE(note1->lyric(), QStringLiteral("la"));
        QCOMPARE(note1->position(), 0);
        QCOMPARE(note1->originalPronunciation(), QStringLiteral("la0"));
        QCOMPARE(note1->editedPronunciation(), QStringLiteral("la1"));
        QCOMPARE(note1->vibratoAmplitude(), 30);
        QCOMPARE(note1->vibratoEnd(), 0.8);
        QCOMPARE(note1->vibratoFrequency(), 5.5);
        QCOMPARE(note1->vibratoOffset(), 15);
        QCOMPARE(note1->vibratoPhase(), 0.25);
        QCOMPARE(note1->vibratoStart(), 0.2);
        QVERIFY(note1->editedPhonemes() != nullptr);
        QVERIFY(note1->originalPhonemes() != nullptr);
        QVERIFY(note1->vibratoAmplitudeControlPoints() != nullptr);
        QVERIFY(note1->vibratoFrequencyControlPoints() != nullptr);
        QVERIFY(note1->noteSequence() != nullptr);
        (void) note1->overlapped();
        (void) note1->previousItem();
        (void) note1->nextItem();

        auto *phoneme1 = model.createPhoneme();
        auto *phoneme2 = model.createPhoneme();
        verifyEntity(phoneme1, &model);
        verifyEntity(phoneme2, &model);
        phoneme1->setLanguage(QStringLiteral("zh"));
        phoneme1->setStart(0);
        phoneme1->setToken(QStringLiteral("l"));
        phoneme1->setOnset(true);
        phoneme2->setLanguage(QStringLiteral("zh"));
        phoneme2->setStart(120);
        phoneme2->setToken(QStringLiteral("a"));
        phoneme2->setOnset(false);

        auto *originalPhonemes = note1->originalPhonemes();
        auto *editedPhonemes = note1->editedPhonemes();
        QCOMPARE(originalPhonemes->role(), PhonemeSequence::Original);
        QCOMPARE(editedPhonemes->role(), PhonemeSequence::Edited);
        QCOMPARE(originalPhonemes->note(), note1);
        QVERIFY(originalPhonemes->insertItem(phoneme1));
        QVERIFY(editedPhonemes->insertItem(phoneme2));
        QCOMPARE(originalPhonemes->size(), 1);
        QCOMPARE(originalPhonemes->firstItem(), phoneme1);
        QCOMPARE(originalPhonemes->lastItem(), phoneme1);
        QVERIFY(originalPhonemes->contains(phoneme1));
        QCOMPARE(originalPhonemes->slice(0, 200).size(), 1);
        QVERIFY(editedPhonemes->moveItem(phoneme2, originalPhonemes));
        QVERIFY(originalPhonemes->moveItem(phoneme2, editedPhonemes));
        QVERIFY(originalPhonemes->removeItem(phoneme1));
        QVERIFY(originalPhonemes->insertItem(phoneme1));

        QCOMPARE(phoneme1->language(), QStringLiteral("zh"));
        QCOMPARE(phoneme1->start(), 0);
        QCOMPARE(phoneme1->token(), QStringLiteral("l"));
        QCOMPARE(phoneme1->onset(), true);
        QVERIFY(phoneme1->phonemeSequence() != nullptr);
        (void) phoneme1->previousItem();
        (void) phoneme1->nextItem();
    });
}

void OrmSmokeTest::parametersAndDataContainers()
{
    Document document;
    Model model(&document);

    withTransaction(document, [&] {
        auto *clip1 = model.createSingingClip();
        auto *clip2 = model.createSingingClip();
        auto *parameter1 = model.createParameter();
        auto *parameter2 = model.createParameter();
        auto *parameter3 = model.createParameter();
        verifyEntity(clip1, &model);
        verifyEntity(clip2, &model);
        verifyEntity(parameter1, &model);
        verifyEntity(parameter2, &model);
        verifyEntity(parameter3, &model);

        auto *map1 = clip1->parameters();
        auto *map2 = clip2->parameters();
        QCOMPARE(map1->singingClip(), clip1);
        QVERIFY(map1->insertItem(QStringLiteral("pitch"), parameter1));
        QVERIFY(map1->insertItem(QStringLiteral("energy"), parameter2));
        QVERIFY(map2->insertItem(QStringLiteral("gender"), parameter3));
        QCOMPARE(map1->size(), 2);
        QCOMPARE(map1->keys().size(), 2);
        QCOMPARE(map1->items().size(), 2);
        QVERIFY(map1->containsKey(QStringLiteral("pitch")));
        QVERIFY(map1->containsItem(parameter1));
        QCOMPARE(map1->item(QStringLiteral("pitch")), parameter1);
        QVERIFY(map1->moveItem(QStringLiteral("energy"), map2, QStringLiteral("energy")));
        QVERIFY(map2->moveItem(QStringLiteral("energy"), map1, QStringLiteral("energy")));
        QVERIFY(map1->removeItem(QStringLiteral("energy")));
        QVERIFY(map1->insertItem(QStringLiteral("energy"), parameter2));

        QVERIFY(parameter1->original() != nullptr);
        QVERIFY(parameter1->freeTransform() != nullptr);
        QVERIFY(parameter1->freeEdited() != nullptr);
        QVERIFY(parameter1->anchorTransform() != nullptr);
        QVERIFY(parameter1->anchorEdited() != nullptr);
        QCOMPARE(parameter1->parameterMap(), map1);

        auto *original = parameter1->original();
        auto *freeTransform = parameter1->freeTransform();
        auto *freeEdited = parameter1->freeEdited();
        QCOMPARE(original->role(), FreeValueDataArray::Original);
        QCOMPARE(freeTransform->role(), FreeValueDataArray::Transform);
        QCOMPARE(freeEdited->role(), FreeValueDataArray::Edited);
        QCOMPARE(original->parameter(), parameter1);
        QCOMPARE(original->step(), 5);
        QVERIFY(original->splice(0, 0, QList<QVariant> {1, 2, 3}));
        QCOMPARE(original->size(), 3);
        QCOMPARE(original->items().size(), 3);
        QCOMPARE(original->slice(0, 2).size(), 2);
        QVERIFY(original->rotate(0, 1, 3));
        QVERIFY(freeTransform->splice(0, 0, QList<QVariant> {4, 5}));
        QVERIFY(freeTransform->rotate(0, 1, 2));
        QVERIFY(freeEdited->splice(0, 0, QList<QVariant> {6}));

        auto *anchor1 = model.createAnchorNode();
        auto *anchor2 = model.createAnchorNode();
        verifyEntity(anchor1, &model);
        verifyEntity(anchor2, &model);
        anchor1->setInterpolationMode(AnchorNode::Hermite);
        anchor1->setX(0);
        anchor1->setY(64);
        anchor2->setInterpolationMode(AnchorNode::Linear);
        anchor2->setX(120);
        anchor2->setY(72);

        auto *anchorTransform = parameter1->anchorTransform();
        auto *anchorEdited = parameter1->anchorEdited();
        QCOMPARE(anchorTransform->role(), AnchorNodeSequence::Transform);
        QCOMPARE(anchorEdited->role(), AnchorNodeSequence::Edited);
        QCOMPARE(anchorTransform->parameter(), parameter1);
        QVERIFY(anchorTransform->insertItem(anchor1));
        QVERIFY(anchorEdited->insertItem(anchor2));
        QCOMPARE(anchorTransform->size(), 1);
        QCOMPARE(anchorTransform->firstItem(), anchor1);
        QCOMPARE(anchorTransform->lastItem(), anchor1);
        QVERIFY(anchorTransform->contains(anchor1));
        QCOMPARE(anchorTransform->slice(0, 200).size(), 1);
        QVERIFY(anchorEdited->moveItem(anchor2, anchorTransform));
        QVERIFY(anchorTransform->moveItem(anchor2, anchorEdited));
        QVERIFY(anchorTransform->removeItem(anchor1));
        QVERIFY(anchorTransform->insertItem(anchor1));

        QCOMPARE(anchor1->interpolationMode(), AnchorNode::Hermite);
        QCOMPARE(anchor1->x(), 0);
        QCOMPARE(anchor1->y(), 64);
        QVERIFY(anchor1->anchorNodeSequence() != nullptr);
        (void) anchor1->previousItem();
        (void) anchor1->nextItem();

        auto *note = model.createNote();
        verifyEntity(note, &model);
        auto *amplitudePoints = note->vibratoAmplitudeControlPoints();
        auto *frequencyPoints = note->vibratoFrequencyControlPoints();
        QCOMPARE(amplitudePoints->role(), VibratoPointDataArray::Amplitude);
        QCOMPARE(frequencyPoints->role(), VibratoPointDataArray::Frequency);
        QCOMPARE(amplitudePoints->note(), note);
        QVERIFY(amplitudePoints->splice(0, 0, QList<QPointF> {QPointF(0.0, 0.5), QPointF(1.0, 0.8)}));
        QCOMPARE(amplitudePoints->size(), 2);
        QCOMPARE(amplitudePoints->items().size(), 2);
        QCOMPARE(amplitudePoints->slice(0, 1).size(), 1);
        QVERIFY(amplitudePoints->rotate(0, 1, 2));
        QVERIFY(frequencyPoints->splice(0, 0, QList<QPointF> {QPointF(0.0, 5.0)}));
    });
}

void OrmSmokeTest::sourcesSingersAndDynamicMixing()
{
    Document document;
    Model model(&document);

    withTransaction(document, [&] {
        auto *clip1 = model.createSingingClip();
        auto *clip2 = model.createSingingClip();
        auto *sources1 = model.createSources();
        auto *sources2 = model.createSources();
        verifyEntity(clip1, &model);
        verifyEntity(clip2, &model);
        verifyEntity(sources1, &model);
        verifyEntity(sources2, &model);

        sources1->setCategory(QStringLiteral("main"));
        sources2->setCategory(QStringLiteral("backup"));
        clip1->setSources(sources1);
        clip2->setSources(sources2);
        QCOMPARE(sources1->category(), QStringLiteral("main"));
        QCOMPARE(sources1->singingClip(), clip1);
        QCOMPARE(clip1->sources(), sources1);
        QVERIFY(sources1->singers() != nullptr);
        QVERIFY(sources1->dynamicMixingAnchors() != nullptr);

        auto *single1 = model.createSingleSinger();
        auto *single2 = model.createSingleSinger();
        auto *single3 = model.createSingleSinger();
        auto *mixed = model.createMixedSinger();
        verifyEntity(single1, &model);
        verifyEntity(single2, &model);
        verifyEntity(single3, &model);
        verifyEntity(mixed, &model);

        single1->setExtra(QJsonObject {{"style", QStringLiteral("bright")}});
        single1->setId(QStringLiteral("singer-a"));
        single2->setId(QStringLiteral("singer-b"));
        single3->setId(QStringLiteral("singer-c"));
        mixed->setExtra(QJsonObject {{"blend", true}});
        mixed->setRatio(QList<double> {0.25, 0.25});

        auto *sourceSingers = sources1->singers();
        auto *mixedChildren = mixed->singers();
        QCOMPARE(sourceSingers->sources(), sources1);
        QVERIFY(sourceSingers->mixedSinger() == nullptr);
        QCOMPARE(mixedChildren->mixedSinger(), mixed);
        QVERIFY(mixedChildren->sources() == nullptr);
        QVERIFY(sourceSingers->insertItem(0, single1));
        QVERIFY(sourceSingers->insertItem(1, mixed));
        QVERIFY(mixedChildren->insertItem(0, single2));
        QVERIFY(mixedChildren->insertItem(1, single3));
        QCOMPARE(sourceSingers->size(), 2);
        QCOMPARE(sourceSingers->items().size(), 2);
        QVERIFY(sourceSingers->contains(single1));
        QCOMPARE(sourceSingers->item(0), single1);
        QVERIFY(mixedChildren->moveItem(1, sourceSingers, 1));
        QVERIFY(sourceSingers->moveItem(1, mixedChildren, 1));
        QVERIFY(mixedChildren->rotate(0, 1, 2));
        auto *removedSinger = mixedChildren->item(1);
        QVERIFY(mixedChildren->removeItem(1));
        QVERIFY(mixedChildren->insertItem(1, removedSinger));

        QCOMPARE(single1->type(), Singer::Single);
        QCOMPARE(single1->extra().toObject().value(QStringLiteral("style")).toString(), QStringLiteral("bright"));
        QCOMPARE(single1->id(), QStringLiteral("singer-a"));
        QVERIFY(single1->singerList() != nullptr);
        QCOMPARE(mixed->type(), Singer::Mixed);
        QCOMPARE(mixed->ratio(), QList<double>({0.25, 0.25}));
        QVERIFY(mixed->singers() != nullptr);

        auto *anchor1 = model.createDynamicMixingAnchor();
        auto *anchor2 = model.createDynamicMixingAnchor();
        verifyEntity(anchor1, &model);
        verifyEntity(anchor2, &model);
        anchor1->setPosition(0);
        anchor1->setRatio(QList<double> {0.4});
        anchor2->setPosition(240);
        anchor2->setRatio(QList<double> {0.5});

        auto *mixing1 = sources1->dynamicMixingAnchors();
        auto *mixing2 = sources2->dynamicMixingAnchors();
        QCOMPARE(mixing1->sources(), sources1);
        QVERIFY(mixing1->insertItem(anchor1));
        QVERIFY(mixing2->insertItem(anchor2));
        QCOMPARE(mixing1->size(), 1);
        QCOMPARE(mixing1->firstItem(), anchor1);
        QCOMPARE(mixing1->lastItem(), anchor1);
        QVERIFY(mixing1->contains(anchor1));
        QCOMPARE(mixing1->slice(0, 300).size(), 1);
        QVERIFY(mixing2->moveItem(anchor2, mixing1));
        QVERIFY(mixing1->moveItem(anchor2, mixing2));
        QVERIFY(mixing1->removeItem(anchor1));
        QVERIFY(mixing1->insertItem(anchor1));

        QCOMPARE(anchor1->position(), 0);
        QCOMPARE(anchor1->ratio(), QList<double>({0.4}));
        QVERIFY(anchor1->dynamicMixingAnchorSequence() != nullptr);
        (void) anchor1->previousItem();
        (void) anchor1->nextItem();
    });
}

void OrmSmokeTest::destroyItem()
{
    Document document;
    Model model(&document);

    withTransaction(document, [&] {
        auto *track = model.createTrack();
        verifyEntity(track, &model);
        QVERIFY(model.destroyItem(track));
    });
}

QTEST_GUILESS_MAIN(OrmSmokeTest)

#include "tst_orm_smoke.moc"
