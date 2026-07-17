#include <QtTest/QtTest>

#include <QEvent>
#include <QMetaMethod>
#include <QMetaProperty>
#include <QPointer>

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
    void undoInsertedNoteDoesNotReadRemovedItem();
    void afterCommitPropertySignals();
    void afterCommitListSignals();
    void destroyIsDeferredUntilAfterCommit();
    void rollbackCancelsAfterCommitAndDestruction();
    void propertyNotifyRemainsAfterApply();
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

void OrmSignalsTest::afterCommitPropertySignals() {
    OrmTestContext context;
    Track *track = nullptr;
    context.withTransaction([&] {
        track = context.model.createTrack();
        context.verifyEntity(track);
    });

    QSignalSpy projectNameApplySpy(&context.model, &Model::projectNameChanged);
    QSignalSpy projectNameCommitSpy(&context.model, &Model::projectNameChangedAfterCommit);
    QSignalSpy trackNameApplySpy(track, &Track::nameChanged);
    QSignalSpy trackNameCommitSpy(track, &Track::nameChangedAfterCommit);
    QSignalSpy trackPanCommitSpy(track, &Track::panChangedAfterCommit);
    QVERIFY(projectNameApplySpy.isValid());
    QVERIFY(projectNameCommitSpy.isValid());
    QVERIFY(trackNameApplySpy.isValid());
    QVERIFY(trackNameCommitSpy.isValid());
    QVERIFY(trackPanCommitSpy.isValid());

    auto transaction = context.document.engine()->beginTransaction();
    context.document.setTransaction(&transaction);
    context.model.setProjectName(QStringLiteral("First"));
    context.model.setProjectName(QStringLiteral("Committed"));
    track->setName(QStringLiteral("Track After Commit"));
    track->setPan(-0.25);

    QCOMPARE(projectNameApplySpy.count(), 2);
    QCOMPARE(trackNameApplySpy.count(), 1);
    QCOMPARE(projectNameCommitSpy.count(), 0);
    QCOMPARE(trackNameCommitSpy.count(), 0);
    QCOMPARE(trackPanCommitSpy.count(), 0);
    QCOMPARE(context.model.projectName(), QStringLiteral("Committed"));

    transaction.commit();
    context.document.setTransaction(nullptr);

    QCOMPARE(projectNameCommitSpy.count(), 1);
    QCOMPARE(projectNameCommitSpy.at(0).at(0).toString(), QStringLiteral("Committed"));
    QCOMPARE(trackNameCommitSpy.count(), 1);
    QCOMPARE(trackNameCommitSpy.at(0).at(0).toString(), QStringLiteral("Track After Commit"));
    QCOMPARE(trackPanCommitSpy.count(), 1);
    QCOMPARE(trackPanCommitSpy.at(0).at(0).toDouble(), -0.25);
}

void OrmSignalsTest::afterCommitListSignals() {
    OrmTestContext context;
    Track *track = nullptr;
    context.withTransaction([&] {
        track = context.model.createTrack();
        context.verifyEntity(track);
    });

    auto *tracks = context.model.tracks();
    QSignalSpy aboutToInsertApplySpy(tracks, &TrackList::itemAboutToInsert);
    QSignalSpy insertedApplySpy(tracks, &TrackList::itemInserted);
    QSignalSpy aboutToInsertCommitSpy(tracks, &TrackList::itemAboutToInsertAfterCommit);
    QSignalSpy insertedCommitSpy(tracks, &TrackList::itemInsertedAfterCommit);
    QSignalSpy sizeCommitSpy(tracks, &TrackList::sizeChangedAfterCommit);
    QSignalSpy itemsCommitSpy(tracks, &TrackList::itemsChangedAfterCommit);
    QVERIFY(aboutToInsertApplySpy.isValid());
    QVERIFY(insertedApplySpy.isValid());
    QVERIFY(aboutToInsertCommitSpy.isValid());
    QVERIFY(insertedCommitSpy.isValid());
    QVERIFY(sizeCommitSpy.isValid());
    QVERIFY(itemsCommitSpy.isValid());

    QStringList commitOrder;
    connect(tracks, &TrackList::itemAboutToInsertAfterCommit, this,
            [&commitOrder] { commitOrder.append(QStringLiteral("aboutToInsert")); });
    connect(tracks, &TrackList::itemInsertedAfterCommit, this,
            [&commitOrder] { commitOrder.append(QStringLiteral("inserted")); });

    auto transaction = context.document.engine()->beginTransaction();
    context.document.setTransaction(&transaction);
    QVERIFY(tracks->insertItem(0, track));
    QCOMPARE(aboutToInsertApplySpy.count(), 1);
    QCOMPARE(insertedApplySpy.count(), 1);
    QCOMPARE(aboutToInsertCommitSpy.count(), 0);
    QCOMPARE(insertedCommitSpy.count(), 0);
    QCOMPARE(sizeCommitSpy.count(), 0);
    QCOMPARE(itemsCommitSpy.count(), 0);

    transaction.commit();
    context.document.setTransaction(nullptr);

    QCOMPARE(aboutToInsertCommitSpy.count(), 1);
    QCOMPARE(insertedCommitSpy.count(), 1);
    QCOMPARE(aboutToInsertCommitSpy.at(0).at(0).toInt(), 0);
    QCOMPARE(qvariant_cast<Track *>(aboutToInsertCommitSpy.at(0).at(1)), track);
    QCOMPARE(sizeCommitSpy.count(), 1);
    QCOMPARE(sizeCommitSpy.at(0).at(0).toInt(), 1);
    QCOMPARE(itemsCommitSpy.count(), 1);
    QCOMPARE(commitOrder, QStringList({QStringLiteral("aboutToInsert"), QStringLiteral("inserted")}));
}

void OrmSignalsTest::destroyIsDeferredUntilAfterCommit() {
    OrmTestContext context;
    Track *track = nullptr;
    context.withTransaction([&] {
        track = context.model.createTrack();
        context.verifyEntity(track);
        QVERIFY(context.model.tracks()->insertItem(0, track));
    });

    QPointer<Track> guard(track);
    QSignalSpy removedCommitSpy(context.model.tracks(), &TrackList::itemRemovedAfterCommit);
    QSignalSpy destroyedSpy(track, &QObject::destroyed);
    QVERIFY(removedCommitSpy.isValid());
    QVERIFY(destroyedSpy.isValid());

    auto transaction = context.document.engine()->beginTransaction();
    context.document.setTransaction(&transaction);
    QVERIFY(context.model.destroyItem(track));
    QVERIFY(guard);
    QCOMPARE(removedCommitSpy.count(), 0);
    QCOMPARE(destroyedSpy.count(), 0);

    transaction.commit();
    context.document.setTransaction(nullptr);

    QVERIFY(guard);
    QCOMPARE(removedCommitSpy.count(), 1);
    QCOMPARE(qvariant_cast<Track *>(removedCommitSpy.at(0).at(1)), track);
    QCOMPARE(destroyedSpy.count(), 0);

    QCoreApplication::sendPostedEvents(nullptr, QEvent::DeferredDelete);
    QVERIFY(!guard);
    QCOMPARE(destroyedSpy.count(), 1);
}

void OrmSignalsTest::rollbackCancelsAfterCommitAndDestruction() {
    OrmTestContext context;
    Track *track = nullptr;
    context.withTransaction([&] {
        track = context.model.createTrack();
        context.verifyEntity(track);
        track->setName(QStringLiteral("Original"));
        QVERIFY(context.model.tracks()->insertItem(0, track));
    });

    const auto handle = track->handle();
    QPointer<Track> guard(track);
    QSignalSpy nameCommitSpy(track, &Track::nameChangedAfterCommit);
    QSignalSpy removedCommitSpy(context.model.tracks(), &TrackList::itemRemovedAfterCommit);
    QSignalSpy destroyedSpy(track, &QObject::destroyed);

    auto transaction = context.document.engine()->beginTransaction();
    context.document.setTransaction(&transaction);
    track->setName(QStringLiteral("Rolled Back"));
    QVERIFY(context.model.destroyItem(track));
    transaction.rollback();
    context.document.setTransaction(nullptr);

    QCOMPARE(nameCommitSpy.count(), 0);
    QCOMPARE(removedCommitSpy.count(), 0);
    QCOMPARE(destroyedSpy.count(), 0);
    QVERIFY(guard);
    QCOMPARE(context.model.find<Track>(handle), track);
    QCOMPARE(track->name(), QStringLiteral("Original"));
    QVERIFY(context.model.tracks()->contains(track));

    QCoreApplication::sendPostedEvents(nullptr, QEvent::DeferredDelete);
    QVERIFY(guard);
    QCOMPARE(destroyedSpy.count(), 0);
}

void OrmSignalsTest::propertyNotifyRemainsAfterApply() {
    const auto propertyIndex = Model::staticMetaObject.indexOfProperty("projectName");
    QVERIFY(propertyIndex >= 0);
    const auto property = Model::staticMetaObject.property(propertyIndex);
    QCOMPARE(property.notifySignal().methodIndex(), QMetaMethod::fromSignal(&Model::projectNameChanged).methodIndex());
    QVERIFY(property.notifySignal().methodIndex() != QMetaMethod::fromSignal(&Model::projectNameChangedAfterCommit).methodIndex());
}

QTEST_GUILESS_MAIN(OrmSignalsTest)

#include "tst_orm_signals.moc"
