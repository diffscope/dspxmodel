#include <QtTest/QtTest>

#include "orm_test_context.h"

#include <dspxmodelORM/ClipSequence.h>
#include <dspxmodelORM/Model.h>
#include <dspxmodelORM/Note.h>
#include <dspxmodelORM/NoteSequence.h>
#include <dspxmodelORM/SingingClip.h>
#include <dspxmodelORM/Track.h>
#include <dspxmodelORM/TrackList.h>

using namespace dspx;

class OrmSignalsTest : public QObject {
    Q_OBJECT

private slots:
    void initTestCase();
    void modelPropertySignals();
    void entityPropertySignals();
    void trackListSignals();
    void noteSequenceSignals();
};

void OrmSignalsTest::initTestCase()
{
    qRegisterMetaType<Note *>("Note*");
    qRegisterMetaType<Note *>("dspx::Note*");
    qRegisterMetaType<NoteSequence *>("NoteSequence*");
    qRegisterMetaType<NoteSequence *>("dspx::NoteSequence*");
    qRegisterMetaType<Track *>("Track*");
    qRegisterMetaType<Track *>("dspx::Track*");
    qRegisterMetaType<TrackList *>("TrackList*");
    qRegisterMetaType<TrackList *>("dspx::TrackList*");
}

void OrmSignalsTest::modelPropertySignals()
{
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

void OrmSignalsTest::entityPropertySignals()
{
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

void OrmSignalsTest::trackListSignals()
{
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

void OrmSignalsTest::noteSequenceSignals()
{
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

QTEST_GUILESS_MAIN(OrmSignalsTest)

#include "tst_orm_signals.moc"
