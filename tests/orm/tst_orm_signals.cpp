#include <QtTest/QtTest>

#include "orm_test_context.h"

#include <QBuffer>

#include <cstdint>
#include <memory>

#include <dspxmodelCore/Schema.h>
#include <dspxmodelORM/AudioClip.h>
#include <dspxmodelORM/ClipSequence.h>
#include <dspxmodelORM/FreeValueDataArray.h>
#include <dspxmodelORM/Model.h>
#include <dspxmodelORM/Note.h>
#include <dspxmodelORM/NoteSequence.h>
#include <dspxmodelORM/Parameter.h>
#include <dspxmodelORM/SingingClip.h>
#include <dspxmodelORM/SingerList.h>
#include <dspxmodelORM/SingleSinger.h>
#include <dspxmodelORM/Sources.h>
#include <dspxmodelORM/Track.h>
#include <dspxmodelORM/TrackList.h>
#include <dspxmodelORM/VibratoPointDataArray.h>

using namespace dspx;

class OrmSignalsTest : public QObject {
    Q_OBJECT

private slots:
    void initTestCase();
    void modelPropertySignals();
    void entityPropertySignals();
    void trackListSignals();
    void undoDestroyedIndexedListItemsRestoresOwnerState();
    void noteSequenceSignals();
    void clipSequenceCountSignalsAndStorageUpdates();
    void clipSequenceCountsSurviveUndoRedoCascadeAndRestore();
    void undoInsertedNoteDoesNotReadRemovedItem();
    void undoDataArraySpliceEmitsSingleSignal();
};

void OrmSignalsTest::initTestCase() {
    qRegisterMetaType<Note *>("Note*");
    qRegisterMetaType<Note *>("dspx::Note*");
    qRegisterMetaType<NoteSequence *>("NoteSequence*");
    qRegisterMetaType<NoteSequence *>("dspx::NoteSequence*");
    qRegisterMetaType<Track *>("Track*");
    qRegisterMetaType<Track *>("dspx::Track*");
    qRegisterMetaType<TrackList *>("TrackList*");
    qRegisterMetaType<TrackList *>("dspx::TrackList*");
}

void OrmSignalsTest::modelPropertySignals() {
    OrmTestContext context;

    QSignalSpy projectNameSpy(&context.model, &Model::projectNameChanged);
    QSignalSpy gainSpy(&context.model, &Model::gainChanged);
    QVERIFY(projectNameSpy.isValid());
    QVERIFY(gainSpy.isValid());

    context.withTransaction([&] {
        context.model.setProjectName(QStringLiteral("Signals"));
        context.model.setGain(0.5);
    });

    QCOMPARE(projectNameSpy.count(), 1);
    QCOMPARE(projectNameSpy.at(0).at(0).toString(), QStringLiteral("Signals"));
    QCOMPARE(gainSpy.count(), 1);
    QCOMPARE(gainSpy.at(0).at(0).toDouble(), 0.5);
}

void OrmSignalsTest::entityPropertySignals() {
    OrmTestContext context;
    Track *track = nullptr;
    Note *note = nullptr;

    context.withTransaction([&] {
        track = context.model.createTrack();
        note = context.model.createNote();
        context.verifyEntity(track);
        context.verifyEntity(note);
    });

    QSignalSpy trackNameSpy(track, &Track::nameChanged);
    QSignalSpy trackPanSpy(track, &Track::panChanged);
    QSignalSpy noteLyricSpy(note, &Note::lyricChanged);
    QSignalSpy notePositionSpy(note, &Note::positionChanged);
    QVERIFY(trackNameSpy.isValid());
    QVERIFY(trackPanSpy.isValid());
    QVERIFY(noteLyricSpy.isValid());
    QVERIFY(notePositionSpy.isValid());

    context.withTransaction([&] {
        track->setName(QStringLiteral("Signal Track"));
        track->setPan(-0.5);
        note->setLyric(QStringLiteral("mi"));
        note->setPosition(240);
    });

    QCOMPARE(trackNameSpy.count(), 1);
    QCOMPARE(trackNameSpy.at(0).at(0).toString(), QStringLiteral("Signal Track"));
    QCOMPARE(trackPanSpy.count(), 1);
    QCOMPARE(trackPanSpy.at(0).at(0).toDouble(), -0.5);
    QCOMPARE(noteLyricSpy.count(), 1);
    QCOMPARE(noteLyricSpy.at(0).at(0).toString(), QStringLiteral("mi"));
    QCOMPARE(notePositionSpy.count(), 1);
    QCOMPARE(notePositionSpy.at(0).at(0).toInt(), 240);
}

void OrmSignalsTest::trackListSignals() {
    OrmTestContext context;

    context.withTransaction([&] {
        auto *track = context.model.createTrack();
        context.verifyEntity(track);

        auto *tracks = context.model.tracks();
        QSignalSpy aboutToInsertSpy(tracks, &TrackList::itemAboutToInsert);
        QSignalSpy insertedSpy(tracks, &TrackList::itemInserted);
        QSignalSpy aboutToRemoveSpy(tracks, &TrackList::itemAboutToRemove);
        QSignalSpy removedSpy(tracks, &TrackList::itemRemoved);
        QSignalSpy itemsSpy(tracks, &TrackList::itemsChanged);
        QSignalSpy sizeSpy(tracks, &TrackList::sizeChanged);
        QVERIFY(aboutToInsertSpy.isValid());
        QVERIFY(insertedSpy.isValid());
        QVERIFY(aboutToRemoveSpy.isValid());
        QVERIFY(removedSpy.isValid());
        QVERIFY(itemsSpy.isValid());
        QVERIFY(sizeSpy.isValid());

        QVERIFY(tracks->insertItem(0, track));
        QCOMPARE(aboutToInsertSpy.count(), 1);
        QCOMPARE(insertedSpy.count(), 1);
        QCOMPARE(itemsSpy.count(), 1);
        QCOMPARE(sizeSpy.count(), 1);

        QVERIFY(tracks->removeItem(0));
        QCOMPARE(aboutToRemoveSpy.count(), 1);
        QCOMPARE(removedSpy.count(), 1);
        QCOMPARE(itemsSpy.count(), 2);
        QCOMPARE(sizeSpy.count(), 2);
    });
}

void OrmSignalsTest::undoDestroyedIndexedListItemsRestoresOwnerState() {
    OrmTestContext context;
    Track *track = nullptr;
    Sources *sources = nullptr;
    SingleSinger *singer = nullptr;

    context.withTransaction([&] {
        track = context.model.createTrack();
        sources = context.model.createSources();
        singer = context.model.createSingleSinger();
        context.verifyEntity(track);
        context.verifyEntity(sources);
        context.verifyEntity(singer);
        QVERIFY(context.model.tracks()->insertItem(0, track));
        QVERIFY(sources->singers()->insertItem(0, singer));
    });

    auto *tracks = context.model.tracks();
    auto *singers = sources->singers();
    QCOMPARE(tracks->size(), 1);
    QCOMPARE(singers->size(), 1);

    QSignalSpy trackSizeSpy(tracks, &TrackList::sizeChanged);
    QSignalSpy trackItemsSpy(tracks, &TrackList::itemsChanged);
    QSignalSpy singerSizeSpy(singers, &SingerList::sizeChanged);
    QSignalSpy singerItemsSpy(singers, &SingerList::itemsChanged);
    QVERIFY(trackSizeSpy.isValid());
    QVERIFY(trackItemsSpy.isValid());
    QVERIFY(singerSizeSpy.isValid());
    QVERIFY(singerItemsSpy.isValid());

    QObject signalContext;
    int trackAboutToInsertCount = 0;
    int trackInsertedCount = 0;
    int singerAboutToInsertCount = 0;
    int singerInsertedCount = 0;
    QObject::connect(tracks, &TrackList::itemAboutToInsert, &signalContext,
                     [&](int, Track *) { ++trackAboutToInsertCount; });
    QObject::connect(tracks, &TrackList::itemInserted, &signalContext,
                     [&](int, Track *) { ++trackInsertedCount; });
    QObject::connect(singers, &SingerList::itemAboutToInsert, &signalContext,
                     [&](int, Singer *, SingerList *) { ++singerAboutToInsertCount; });
    QObject::connect(singers, &SingerList::itemInserted, &signalContext,
                     [&](int, Singer *, SingerList *) { ++singerInsertedCount; });

    context.document.engine()->clearUndoHistory();
    context.withTransaction([&] {
        QVERIFY(context.model.destroyItem(track));
    });
    QCOMPARE(tracks->size(), 0);

    context.document.engine()->undo();
    QCOMPARE(tracks->size(), 1);
    QCOMPARE(tracks->items().size(), 1);
    auto *restoredTrack = tracks->item(0);
    QVERIFY(restoredTrack != nullptr);
    QCOMPARE(restoredTrack->trackList(), tracks);
    QCOMPARE(trackSizeSpy.count(), 2);
    QCOMPARE(trackSizeSpy.at(0).at(0).toInt(), 0);
    QCOMPARE(trackSizeSpy.at(1).at(0).toInt(), 1);
    QCOMPARE(trackItemsSpy.count(), 2);
    QCOMPARE(trackAboutToInsertCount, 1);
    QCOMPARE(trackInsertedCount, 1);

    context.document.engine()->clearUndoHistory();
    context.withTransaction([&] {
        QVERIFY(context.model.destroyItem(singer));
    });
    QCOMPARE(singers->size(), 0);

    context.document.engine()->undo();
    QCOMPARE(singers->size(), 1);
    QCOMPARE(singers->items().size(), 1);
    auto *restoredSinger = singers->item(0);
    QVERIFY(restoredSinger != nullptr);
    QCOMPARE(restoredSinger->singerList(), singers);
    QCOMPARE(singerSizeSpy.count(), 2);
    QCOMPARE(singerSizeSpy.at(0).at(0).toInt(), 0);
    QCOMPARE(singerSizeSpy.at(1).at(0).toInt(), 1);
    QCOMPARE(singerItemsSpy.count(), 2);
    QCOMPARE(singerAboutToInsertCount, 1);
    QCOMPARE(singerInsertedCount, 1);
}

void OrmSignalsTest::noteSequenceSignals() {
    OrmTestContext context;

    context.withTransaction([&] {
        auto *clip = context.model.createSingingClip();
        auto *note = context.model.createNote();
        context.verifyEntity(clip);
        context.verifyEntity(note);
        note->setPosition(0);
        note->setLength(120);

        auto *notes = clip->notes();
        QSignalSpy aboutToInsertSpy(notes, &NoteSequence::itemAboutToInsert);
        QSignalSpy insertedSpy(notes, &NoteSequence::itemInserted);
        QSignalSpy aboutToRemoveSpy(notes, &NoteSequence::itemAboutToRemove);
        QSignalSpy removedSpy(notes, &NoteSequence::itemRemoved);
        QSignalSpy firstSpy(notes, &NoteSequence::firstItemChanged);
        QSignalSpy lastSpy(notes, &NoteSequence::lastItemChanged);
        QSignalSpy sizeSpy(notes, &NoteSequence::sizeChanged);
        QVERIFY(aboutToInsertSpy.isValid());
        QVERIFY(insertedSpy.isValid());
        QVERIFY(aboutToRemoveSpy.isValid());
        QVERIFY(removedSpy.isValid());
        QVERIFY(firstSpy.isValid());
        QVERIFY(lastSpy.isValid());
        QVERIFY(sizeSpy.isValid());

        QVERIFY(notes->insertItem(note));
        QCOMPARE(aboutToInsertSpy.count(), 1);
        QCOMPARE(insertedSpy.count(), 1);
        QCOMPARE(firstSpy.count(), 1);
        QCOMPARE(lastSpy.count(), 1);
        QCOMPARE(sizeSpy.count(), 1);

        QVERIFY(notes->removeItem(note));
        QCOMPARE(aboutToRemoveSpy.count(), 1);
        QCOMPARE(removedSpy.count(), 1);
        QCOMPARE(sizeSpy.count(), 2);
    });
}

void OrmSignalsTest::clipSequenceCountSignalsAndStorageUpdates() {
    OrmTestContext context;
    Track *track1 = nullptr;
    Track *track2 = nullptr;

    context.withTransaction([&] {
        track1 = context.model.createTrack();
        track2 = context.model.createTrack();
        context.verifyEntity(track1);
        context.verifyEntity(track2);
        QVERIFY(context.model.tracks()->insertItem(0, track1));
        QVERIFY(context.model.tracks()->insertItem(1, track2));
    });

    auto *clips1 = track1->clips();
    auto *clips2 = track2->clips();
    QCOMPARE(clips1->audioClipCount(), 0);
    QCOMPARE(clips1->singingClipCount(), 0);
    QCOMPARE(clips2->audioClipCount(), 0);
    QCOMPARE(clips2->singingClipCount(), 0);

    QSignalSpy audio1Spy(clips1, &ClipSequence::audioClipCountChanged);
    QSignalSpy singing1Spy(clips1, &ClipSequence::singingClipCountChanged);
    QSignalSpy audio2Spy(clips2, &ClipSequence::audioClipCountChanged);
    QSignalSpy singing2Spy(clips2, &ClipSequence::singingClipCountChanged);
    QVERIFY(audio1Spy.isValid());
    QVERIFY(singing1Spy.isValid());
    QVERIFY(audio2Spy.isValid());
    QVERIFY(singing2Spy.isValid());

    AudioClip *audio = nullptr;
    SingingClip *singing = nullptr;
    context.withTransaction([&] {
        audio = context.model.createAudioClip();
        singing = context.model.createSingingClip();
        QVERIFY(clips1->insertItem(audio));
        QVERIFY(clips1->insertItem(singing));
    });

    QCOMPARE(clips1->size(), 2);
    QCOMPARE(clips1->audioClipCount(), 1);
    QCOMPARE(clips1->singingClipCount(), 1);
    QCOMPARE(audio1Spy.count(), 1);
    QCOMPARE(audio1Spy.at(0).at(0).toInt(), 1);
    QCOMPARE(singing1Spy.count(), 1);
    QCOMPARE(singing1Spy.at(0).at(0).toInt(), 1);
    QCOMPARE(audio2Spy.count(), 0);
    QCOMPARE(singing2Spy.count(), 0);

    context.withTransaction([&] {
        QVERIFY(clips1->moveItem(audio, clips2));
    });

    QCOMPARE(clips1->audioClipCount(), 0);
    QCOMPARE(clips1->singingClipCount(), 1);
    QCOMPARE(clips2->audioClipCount(), 1);
    QCOMPARE(clips2->singingClipCount(), 0);
    QCOMPARE(audio1Spy.count(), 2);
    QCOMPARE(audio1Spy.at(1).at(0).toInt(), 0);
    QCOMPARE(audio2Spy.count(), 1);
    QCOMPARE(audio2Spy.at(0).at(0).toInt(), 1);
    QCOMPARE(singing1Spy.count(), 1);
    QCOMPARE(singing2Spy.count(), 0);

    context.withTransaction([&] {
        QVERIFY(clips1->removeItem(singing));
    });

    QCOMPARE(clips1->size(), 0);
    QCOMPARE(clips1->audioClipCount(), 0);
    QCOMPARE(clips1->singingClipCount(), 0);
    QCOMPARE(singing1Spy.count(), 2);
    QCOMPARE(singing1Spy.at(1).at(0).toInt(), 0);

    dini::ItemId rawSingingId = 0;
    context.withTransaction([&] {
        rawSingingId = context.document.transaction()->insert(
            Schema::clipTable(),
            {{
                .column = Schema::clipParent().column(),
                .value = dini::Value(static_cast<std::uint64_t>(track2->handle().d)),
            }},
            Schema::singingClipVariant());
    });

    QCOMPARE(clips2->size(), 2);
    QCOMPARE(clips2->audioClipCount(), 1);
    QCOMPARE(clips2->singingClipCount(), 1);
    QCOMPARE(singing2Spy.count(), 1);
    QCOMPARE(singing2Spy.at(0).at(0).toInt(), 1);

    context.withTransaction([&] {
        context.document.transaction()->remove(rawSingingId);
    });

    QCOMPARE(clips2->size(), 1);
    QCOMPARE(clips2->audioClipCount(), 1);
    QCOMPARE(clips2->singingClipCount(), 0);
    QCOMPARE(singing2Spy.count(), 2);
    QCOMPARE(singing2Spy.at(1).at(0).toInt(), 0);
}

void OrmSignalsTest::clipSequenceCountsSurviveUndoRedoCascadeAndRestore() {
    OrmTestContext context;
    Track *track = nullptr;

    context.withTransaction([&] {
        track = context.model.createTrack();
        context.verifyEntity(track);
        QVERIFY(context.model.tracks()->insertItem(0, track));
    });
    context.document.engine()->clearUndoHistory();

    auto *clips = track->clips();
    QSignalSpy audioSpy(clips, &ClipSequence::audioClipCountChanged);
    QSignalSpy singingSpy(clips, &ClipSequence::singingClipCountChanged);
    QVERIFY(audioSpy.isValid());
    QVERIFY(singingSpy.isValid());

    context.withTransaction([&] {
        QVERIFY(clips->insertItem(context.model.createAudioClip()));
        QVERIFY(clips->insertItem(context.model.createSingingClip()));
    });
    QCOMPARE(clips->audioClipCount(), 1);
    QCOMPARE(clips->singingClipCount(), 1);

    context.document.engine()->undo();
    QCOMPARE(clips->size(), 0);
    QCOMPARE(clips->audioClipCount(), 0);
    QCOMPARE(clips->singingClipCount(), 0);
    QCOMPARE(audioSpy.count(), 2);
    QCOMPARE(audioSpy.at(1).at(0).toInt(), 0);
    QCOMPARE(singingSpy.count(), 2);
    QCOMPARE(singingSpy.at(1).at(0).toInt(), 0);

    context.document.engine()->redo();
    QCOMPARE(clips->size(), 2);
    QCOMPARE(clips->audioClipCount(), 1);
    QCOMPARE(clips->singingClipCount(), 1);
    QCOMPARE(audioSpy.count(), 3);
    QCOMPARE(audioSpy.at(2).at(0).toInt(), 1);
    QCOMPARE(singingSpy.count(), 3);
    QCOMPARE(singingSpy.at(2).at(0).toInt(), 1);

    context.document.engine()->clearUndoHistory();
    const auto trackId = static_cast<dini::ItemId>(track->handle().d);
    context.withTransaction([&] {
        QVERIFY(context.model.destroyItem(track));
    });
    QCOMPARE(context.model.tracks()->size(), 0);

    context.document.engine()->undo();
    QCOMPARE(context.model.tracks()->size(), 1);
    QVERIFY(context.document.engine()->contains(trackId));
    QCOMPARE(context.document.engine()->read(trackId, Schema::trackAudioClipCountColumn()).asInt64(), INT64_C(1));
    QCOMPARE(context.document.engine()->read(trackId, Schema::trackSingingClipCountColumn()).asInt64(), INT64_C(1));
    auto *restoredTrack = context.model.tracks()->item(0);
    QVERIFY(restoredTrack != nullptr);
    QCOMPARE(restoredTrack->clips()->size(), 2);
    QCOMPARE(restoredTrack->clips()->audioClipCount(), 1);
    QCOMPARE(restoredTrack->clips()->singingClipCount(), 1);

    QBuffer snapshot;
    QBuffer commitLog;
    QVERIFY(snapshot.open(QIODevice::ReadWrite));
    QVERIFY(commitLog.open(QIODevice::ReadWrite));
    context.document.writeSnapshot(&snapshot);
    context.document.setCommitLogDevice(&commitLog);
    context.withTransaction([&] {
        QVERIFY(restoredTrack->clips()->insertItem(context.model.createSingingClip()));
    });
    context.document.setCommitLogDevice(nullptr);

    QVERIFY(snapshot.seek(0));
    QVERIFY(commitLog.seek(0));
    std::unique_ptr<Document> restoredDocument(Document::restore(&snapshot, &commitLog));
    QVERIFY(restoredDocument != nullptr);
    Model restoredModel(restoredDocument.get());
    QCOMPARE(restoredModel.tracks()->size(), 1);
    auto *replayedTrack = restoredModel.tracks()->item(0);
    QVERIFY(replayedTrack != nullptr);
    QCOMPARE(replayedTrack->clips()->size(), 3);
    QCOMPARE(replayedTrack->clips()->audioClipCount(), 1);
    QCOMPARE(replayedTrack->clips()->singingClipCount(), 2);
}

void OrmSignalsTest::undoInsertedNoteDoesNotReadRemovedItem() {
    OrmTestContext context;
    SingingClip *clip = nullptr;
    Note *note = nullptr;

    context.withTransaction([&] {
        clip = context.model.createSingingClip();
        context.verifyEntity(clip);
    });

    context.withTransaction([&] {
        note = context.model.createNote();
        context.verifyEntity(note);
        note->setPosition(0);
        note->setLength(120);
        note->setKeyNumber(60);
        note->setLyric(QStringLiteral("la"));
        QVERIFY(clip->notes()->insertItem(note));
    });

    QCOMPARE(clip->notes()->size(), 1);
    QVERIFY(context.document.engine()->canUndo());

    try {
        context.document.engine()->undo();
    } catch (const std::exception &e) {
        QFAIL(e.what());
    }

    QCOMPARE(clip->notes()->size(), 0);
    QVERIFY(!context.document.engine()->contains(static_cast<dini::ItemId>(note->handle().d)));
}

void OrmSignalsTest::undoDataArraySpliceEmitsSingleSignal() {
    OrmTestContext context;
    Parameter *parameter = nullptr;
    Note *note = nullptr;

    context.withTransaction([&] {
        parameter = context.model.createParameter();
        note = context.model.createNote();
        context.verifyEntity(parameter);
        context.verifyEntity(note);
    });

    auto *freeValues = parameter->freeTransform();
    auto *vibratoPoints = note->vibratoAmplitudeControlPoints();
    context.withTransaction([&] {
        QVERIFY(freeValues->splice(0, 0, QList<QVariant> {10, 20}));
        QVERIFY(vibratoPoints->splice(0, 0, QList<QPointF> {
                                                    QPointF(0.0, 0.1),
                                                    QPointF(1.0, 0.1),
                                                }));
    });
    context.withTransaction([&] {
        QVERIFY(freeValues->splice(1, 0, QList<QVariant> {1, 2, 3}));
        QVERIFY(vibratoPoints->splice(1, 0, QList<QPointF> {
                                                    QPointF(0.0, 0.2),
                                                    QPointF(0.5, 0.7),
                                                    QPointF(1.0, 0.4),
                                                }));
    });

    QSignalSpy freeValuesSpy(freeValues, &FreeValueDataArray::spliced);
    QSignalSpy vibratoPointsSpy(vibratoPoints, &VibratoPointDataArray::spliced);
    QVERIFY(freeValuesSpy.isValid());
    QVERIFY(vibratoPointsSpy.isValid());

    context.document.engine()->undo();

    QCOMPARE(freeValues->items(), QList<QVariant>({10, 20}));
    QCOMPARE(vibratoPoints->items(), QList<QPointF>({
                                        QPointF(0.0, 0.1),
                                        QPointF(1.0, 0.1),
                                    }));

    QCOMPARE(freeValuesSpy.count(), 1);
    QCOMPARE(freeValuesSpy.at(0).at(0).toInt(), 1);
    QCOMPARE(freeValuesSpy.at(0).at(1).toInt(), 3);
    QCOMPARE(freeValuesSpy.at(0).at(2).value<QList<QVariant>>(), QList<QVariant>());

    QCOMPARE(vibratoPointsSpy.count(), 1);
    QCOMPARE(vibratoPointsSpy.at(0).at(0).toInt(), 1);
    QCOMPARE(vibratoPointsSpy.at(0).at(1).toInt(), 3);
    QCOMPARE(vibratoPointsSpy.at(0).at(2).value<QList<QPointF>>(), QList<QPointF>());
}

QTEST_GUILESS_MAIN(OrmSignalsTest)

#include "tst_orm_signals.moc"
