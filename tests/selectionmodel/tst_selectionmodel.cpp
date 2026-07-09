#include <QtTest/QtTest>

#include "orm_test_context.h"

#include <dspxmodelORM/AnchorNode.h>
#include <dspxmodelORM/AnchorNodeSequence.h>
#include <dspxmodelORM/ClipSequence.h>
#include <dspxmodelORM/KeySignature.h>
#include <dspxmodelORM/KeySignatureSequence.h>
#include <dspxmodelORM/Label.h>
#include <dspxmodelORM/LabelSequence.h>
#include <dspxmodelORM/Model.h>
#include <dspxmodelORM/Note.h>
#include <dspxmodelORM/NoteSequence.h>
#include <dspxmodelORM/Parameter.h>
#include <dspxmodelORM/SingingClip.h>
#include <dspxmodelORM/Tempo.h>
#include <dspxmodelORM/TempoSequence.h>
#include <dspxmodelORM/Track.h>
#include <dspxmodelORM/TrackList.h>
#include <dspxmodelSelectionModel/AnchorNodeSelectionModel.h>
#include <dspxmodelSelectionModel/ClipSelectionModel.h>
#include <dspxmodelSelectionModel/KeySignatureSelectionModel.h>
#include <dspxmodelSelectionModel/LabelSelectionModel.h>
#include <dspxmodelSelectionModel/NoteSelectionModel.h>
#include <dspxmodelSelectionModel/SelectionModel.h>
#include <dspxmodelSelectionModel/TempoSelectionModel.h>
#include <dspxmodelSelectionModel/TrackSelectionModel.h>

using namespace dspx;

class SelectionModelTest : public QObject {
    Q_OBJECT

private slots:
    void basicSelectionCommands();
    void topLevelSignalsAndSafeDispatch();
    void clipTrackConnectionsArePerItem();
    void clipMoveRebindsTrackConnection();
    void noteInvalidationEmitsSignals();
    void invalidNoteAfterClearStillEmits();
    void anchorNodeSelectionCommands();
    void anchorNodeSequenceConstraintAndEmptyContext();
    void anchorNodeInvalidationEmitsSignals();
};

void SelectionModelTest::basicSelectionCommands()
{
    OrmTestContext context;
    SelectionModel selection(&context.model);

    Track *track = nullptr;
    Tempo *tempo = nullptr;
    Label *label = nullptr;
    KeySignature *key = nullptr;
    SingingClip *clip = nullptr;
    Note *note = nullptr;

    context.withTransaction([&] {
        track = context.model.createTrack();
        tempo = context.model.createTempo();
        label = context.model.createLabel();
        key = context.model.createKeySignature();
        clip = context.model.createSingingClip();
        note = context.model.createNote();

        QVERIFY(context.model.tracks()->insertItem(0, track));
        QVERIFY(track->clips()->insertItem(clip));
        QVERIFY(clip->notes()->insertItem(note));
        QVERIFY(context.model.tempos()->insertItem(tempo));
        QVERIFY(context.model.labels()->insertItem(label));
        QVERIFY(context.model.keySignatures()->insertItem(key));
    });

    const auto selectCurrent = SelectionModel::Select | SelectionModel::SetCurrentItem;

    selection.select(track, selectCurrent);
    QCOMPARE(selection.selectionType(), SelectionModel::ST_Track);
    QCOMPARE(selection.currentItem(), static_cast<QObject *>(track));
    QCOMPARE(selection.selectedCount(), 1);
    QVERIFY(selection.trackSelectionModel()->isItemSelected(track));

    selection.select(tempo, selectCurrent);
    QCOMPARE(selection.selectionType(), SelectionModel::ST_Tempo);
    QCOMPARE(selection.currentItem(), static_cast<QObject *>(tempo));
    QCOMPARE(selection.selectedCount(), 1);
    QVERIFY(selection.tempoSelectionModel()->isItemSelected(tempo));
    QCOMPARE(selection.trackSelectionModel()->selectedCount(), 0);

    selection.select(label, selectCurrent);
    QCOMPARE(selection.selectionType(), SelectionModel::ST_Label);
    QCOMPARE(selection.currentItem(), static_cast<QObject *>(label));
    QCOMPARE(selection.selectedCount(), 1);
    QVERIFY(selection.labelSelectionModel()->isItemSelected(label));

    selection.select(key, selectCurrent);
    QCOMPARE(selection.selectionType(), SelectionModel::ST_KeySignature);
    QCOMPARE(selection.currentItem(), static_cast<QObject *>(key));
    QCOMPARE(selection.selectedCount(), 1);
    QVERIFY(selection.keySignatureSelectionModel()->isItemSelected(key));

    selection.select(clip, selectCurrent);
    QCOMPARE(selection.selectionType(), SelectionModel::ST_Clip);
    QCOMPARE(selection.currentItem(), static_cast<QObject *>(clip));
    QCOMPARE(selection.selectedCount(), 1);
    QCOMPARE(selection.clipSelectionModel()->selectedSingingClipCount(), 1);
    QCOMPARE(selection.clipSelectionModel()->selectedAudioClipCount(), 0);
    QCOMPARE(selection.clipSelectionModel()->clipSequencesWithSelectedItems().size(), 1);
    QVERIFY(selection.clipSelectionModel()->clipSequencesWithSelectedItems().contains(track->clips()));

    selection.select(note, selectCurrent);
    QCOMPARE(selection.selectionType(), SelectionModel::ST_Note);
    QCOMPARE(selection.currentItem(), static_cast<QObject *>(note));
    QCOMPARE(selection.selectedCount(), 1);
    QCOMPARE(selection.noteSelectionModel()->noteSequenceWithSelectedItems(), clip->notes());
    QVERIFY(selection.noteSelectionModel()->isItemSelected(note));

    selection.select(note, SelectionModel::Toggle);
    QCOMPARE(selection.selectionType(), SelectionModel::ST_Note);
    QCOMPARE(selection.currentItem(), static_cast<QObject *>(note));
    QCOMPARE(selection.selectedCount(), 0);
    QCOMPARE(selection.noteSelectionModel()->noteSequenceWithSelectedItems(), clip->notes());
}

void SelectionModelTest::topLevelSignalsAndSafeDispatch()
{
    OrmTestContext context;
    SelectionModel selection(&context.model);

    Track *track = nullptr;
    SingingClip *clip = nullptr;

    context.withTransaction([&] {
        track = context.model.createTrack();
        clip = context.model.createSingingClip();

        QVERIFY(context.model.tracks()->insertItem(0, track));
        QVERIFY(track->clips()->insertItem(clip));
    });

    QList<QObject *> observedCurrentItems;
    connect(&selection, &SelectionModel::currentItemChanged, &selection, [&] {
        observedCurrentItems.append(selection.currentItem());
    });

    selection.select(track, SelectionModel::Select | SelectionModel::SetCurrentItem);
    QCOMPARE(observedCurrentItems.last(), static_cast<QObject *>(track));

    QSignalSpy typeSpy(&selection, &SelectionModel::selectionTypeChanged);
    QSignalSpy currentSpy(&selection, &SelectionModel::currentItemChanged);

    selection.select(track, SelectionModel::Select | SelectionModel::SetCurrentItem);
    QCOMPARE(typeSpy.count(), 0);
    QCOMPARE(currentSpy.count(), 0);

    selection.select(clip, SelectionModel::Select | SelectionModel::SetCurrentItem);
    QCOMPARE(selection.selectionType(), SelectionModel::ST_Clip);
    QCOMPARE(selection.currentItem(), static_cast<QObject *>(clip));
    QCOMPARE(observedCurrentItems.last(), static_cast<QObject *>(clip));

    QObject foreign;
    selection.select(&foreign, SelectionModel::Select, SelectionModel::ST_Note, &foreign);
    QCOMPARE(selection.selectionType(), SelectionModel::ST_None);
    QCOMPARE(selection.currentItem(), nullptr);
}

void SelectionModelTest::clipTrackConnectionsArePerItem()
{
    OrmTestContext context;
    SelectionModel selection(&context.model);

    Track *track = nullptr;
    SingingClip *clip1 = nullptr;
    SingingClip *clip2 = nullptr;

    context.withTransaction([&] {
        track = context.model.createTrack();
        clip1 = context.model.createSingingClip();
        clip2 = context.model.createSingingClip();

        QVERIFY(context.model.tracks()->insertItem(0, track));
        QVERIFY(track->clips()->insertItem(clip1));
        QVERIFY(track->clips()->insertItem(clip2));
    });

    selection.select(clip1, SelectionModel::Select);
    selection.select(clip2, SelectionModel::Select);
    QCOMPARE(selection.clipSelectionModel()->selectedCount(), 2);

    selection.select(clip1, SelectionModel::Deselect);
    QCOMPARE(selection.clipSelectionModel()->selectedCount(), 1);
    QVERIFY(selection.clipSelectionModel()->isItemSelected(clip2));

    context.withTransaction([&] {
        QVERIFY(context.model.tracks()->removeItem(0));
    });

    QCOMPARE(selection.clipSelectionModel()->selectedCount(), 0);
    QVERIFY(!selection.clipSelectionModel()->isItemSelected(clip2));
}

void SelectionModelTest::clipMoveRebindsTrackConnection()
{
    OrmTestContext context;
    SelectionModel selection(&context.model);

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

    selection.select(clip, SelectionModel::Select);
    QCOMPARE(selection.clipSelectionModel()->clipSequencesWithSelectedItems().size(), 1);
    QVERIFY(selection.clipSelectionModel()->clipSequencesWithSelectedItems().contains(track1->clips()));

    context.withTransaction([&] {
        QVERIFY(track1->clips()->moveItem(clip, track2->clips()));
    });

    QCOMPARE(selection.clipSelectionModel()->selectedCount(), 1);
    QCOMPARE(selection.clipSelectionModel()->clipSequencesWithSelectedItems().size(), 1);
    QVERIFY(selection.clipSelectionModel()->clipSequencesWithSelectedItems().contains(track2->clips()));

    context.withTransaction([&] {
        QVERIFY(context.model.tracks()->removeItem(1));
    });

    QCOMPARE(selection.clipSelectionModel()->selectedCount(), 0);
}

void SelectionModelTest::noteInvalidationEmitsSignals()
{
    OrmTestContext context;
    SelectionModel selection(&context.model);

    Track *track = nullptr;
    SingingClip *clip = nullptr;
    Note *note = nullptr;

    context.withTransaction([&] {
        track = context.model.createTrack();
        clip = context.model.createSingingClip();
        note = context.model.createNote();

        QVERIFY(context.model.tracks()->insertItem(0, track));
        QVERIFY(track->clips()->insertItem(clip));
        QVERIFY(clip->notes()->insertItem(note));
    });

    selection.select(note, SelectionModel::Select | SelectionModel::SetCurrentItem);
    QCOMPARE(selection.noteSelectionModel()->selectedCount(), 1);

    QSignalSpy selectedItemsSpy(selection.noteSelectionModel(), &NoteSelectionModel::selectedItemsChanged);
    QSignalSpy selectedCountSpy(selection.noteSelectionModel(), &NoteSelectionModel::selectedCountChanged);
    QSignalSpy sequenceSpy(selection.noteSelectionModel(), &NoteSelectionModel::noteSequenceWithSelectedItemsChanged);

    context.withTransaction([&] {
        QVERIFY(context.model.tracks()->removeItem(0));
    });

    QCOMPARE(selection.noteSelectionModel()->selectedCount(), 0);
    QCOMPARE(selection.noteSelectionModel()->currentItem(), nullptr);
    QCOMPARE(selection.noteSelectionModel()->noteSequenceWithSelectedItems(), nullptr);
    QCOMPARE(selectedItemsSpy.count(), 1);
    QCOMPARE(selectedCountSpy.count(), 1);
    QCOMPARE(sequenceSpy.count(), 1);
}

void SelectionModelTest::invalidNoteAfterClearStillEmits()
{
    OrmTestContext context;
    SelectionModel selection(&context.model);

    Track *track = nullptr;
    SingingClip *clip = nullptr;
    Note *selectedNote = nullptr;
    Note *orphanNote = nullptr;

    context.withTransaction([&] {
        track = context.model.createTrack();
        clip = context.model.createSingingClip();
        selectedNote = context.model.createNote();
        orphanNote = context.model.createNote();

        QVERIFY(context.model.tracks()->insertItem(0, track));
        QVERIFY(track->clips()->insertItem(clip));
        QVERIFY(clip->notes()->insertItem(selectedNote));
    });

    selection.select(selectedNote, SelectionModel::Select | SelectionModel::SetCurrentItem);
    QCOMPARE(selection.noteSelectionModel()->selectedCount(), 1);

    QSignalSpy selectedItemsSpy(selection.noteSelectionModel(), &NoteSelectionModel::selectedItemsChanged);
    QSignalSpy selectedCountSpy(selection.noteSelectionModel(), &NoteSelectionModel::selectedCountChanged);

    selection.select(orphanNote, SelectionModel::ClearPreviousSelection | SelectionModel::SetCurrentItem);

    QCOMPARE(selection.noteSelectionModel()->selectedCount(), 0);
    QCOMPARE(selection.noteSelectionModel()->currentItem(), nullptr);
    QCOMPARE(selectedItemsSpy.count(), 1);
    QCOMPARE(selectedCountSpy.count(), 1);
}

void SelectionModelTest::anchorNodeSelectionCommands()
{
    OrmTestContext context;
    SelectionModel selection(&context.model);

    Parameter *parameter = nullptr;
    AnchorNode *anchor1 = nullptr;
    AnchorNode *anchor2 = nullptr;

    context.withTransaction([&] {
        parameter = context.model.createParameter();
        anchor1 = context.model.createAnchorNode();
        anchor2 = context.model.createAnchorNode();
        anchor1->setX(0);
        anchor2->setX(120);

        QVERIFY(parameter->anchorTransform()->insertItem(anchor1));
        QVERIFY(parameter->anchorTransform()->insertItem(anchor2));
    });

    QSignalSpy topItemSelectedSpy(&selection, &SelectionModel::itemSelected);
    QSignalSpy anchorItemSelectedSpy(selection.anchorNodeSelectionModel(), &AnchorNodeSelectionModel::itemSelected);

    selection.select(anchor1, SelectionModel::Select | SelectionModel::SetCurrentItem);
    QCOMPARE(selection.selectionType(), SelectionModel::ST_AnchorNode);
    QCOMPARE(selection.currentItem(), static_cast<QObject *>(anchor1));
    QCOMPARE(selection.selectedCount(), 1);
    QVERIFY(selection.isItemSelected(anchor1));
    QCOMPARE(selection.anchorNodeSelectionModel()->anchorNodeSequenceWithSelectedItems(), parameter->anchorTransform());
    QCOMPARE(topItemSelectedSpy.count(), 1);
    QCOMPARE(anchorItemSelectedSpy.count(), 1);

    selection.select(anchor2, SelectionModel::Select);
    QCOMPARE(selection.anchorNodeSelectionModel()->selectedCount(), 2);
    QVERIFY(selection.anchorNodeSelectionModel()->isItemSelected(anchor2));

    selection.select(anchor1, SelectionModel::Deselect);
    QCOMPARE(selection.anchorNodeSelectionModel()->selectedCount(), 1);
    QVERIFY(!selection.anchorNodeSelectionModel()->isItemSelected(anchor1));

    selection.select(anchor2, SelectionModel::Toggle);
    QCOMPARE(selection.anchorNodeSelectionModel()->selectedCount(), 0);
    QCOMPARE(selection.anchorNodeSelectionModel()->currentItem(), anchor1);
    QCOMPARE(selection.anchorNodeSelectionModel()->anchorNodeSequenceWithSelectedItems(), parameter->anchorTransform());
}

void SelectionModelTest::anchorNodeSequenceConstraintAndEmptyContext()
{
    OrmTestContext context;
    SelectionModel selection(&context.model);

    Parameter *parameter1 = nullptr;
    Parameter *parameter2 = nullptr;
    AnchorNode *anchor1 = nullptr;
    AnchorNode *anchor2 = nullptr;
    AnchorNode *anchor3 = nullptr;
    Track *track = nullptr;

    context.withTransaction([&] {
        parameter1 = context.model.createParameter();
        parameter2 = context.model.createParameter();
        anchor1 = context.model.createAnchorNode();
        anchor2 = context.model.createAnchorNode();
        anchor3 = context.model.createAnchorNode();
        track = context.model.createTrack();
        anchor1->setX(0);
        anchor2->setX(120);
        anchor3->setX(240);

        QVERIFY(parameter1->anchorTransform()->insertItem(anchor1));
        QVERIFY(parameter1->anchorTransform()->insertItem(anchor2));
        QVERIFY(parameter2->anchorEdited()->insertItem(anchor3));
        QVERIFY(context.model.tracks()->insertItem(0, track));
    });

    selection.select(anchor1, SelectionModel::Select | SelectionModel::SetCurrentItem);
    selection.select(anchor2, SelectionModel::Select);
    QCOMPARE(selection.anchorNodeSelectionModel()->selectedCount(), 2);
    QCOMPARE(selection.anchorNodeSelectionModel()->anchorNodeSequenceWithSelectedItems(), parameter1->anchorTransform());

    selection.select(anchor3, SelectionModel::Select | SelectionModel::SetCurrentItem);
    QCOMPARE(selection.selectionType(), SelectionModel::ST_AnchorNode);
    QCOMPARE(selection.anchorNodeSelectionModel()->selectedCount(), 1);
    QCOMPARE(selection.anchorNodeSelectionModel()->currentItem(), anchor3);
    QVERIFY(selection.anchorNodeSelectionModel()->isItemSelected(anchor3));
    QVERIFY(!selection.anchorNodeSelectionModel()->isItemSelected(anchor1));
    QCOMPARE(selection.anchorNodeSelectionModel()->anchorNodeSequenceWithSelectedItems(), parameter2->anchorEdited());

    selection.select(nullptr, SelectionModel::ClearPreviousSelection | SelectionModel::SetCurrentItem,
                     SelectionModel::ST_AnchorNode, parameter1->anchorTransform());
    QCOMPARE(selection.selectionType(), SelectionModel::ST_AnchorNode);
    QCOMPARE(selection.anchorNodeSelectionModel()->selectedCount(), 0);
    QCOMPARE(selection.anchorNodeSelectionModel()->currentItem(), nullptr);
    QCOMPARE(selection.anchorNodeSelectionModel()->anchorNodeSequenceWithSelectedItems(), parameter1->anchorTransform());

    selection.select(anchor1, SelectionModel::Select);
    QCOMPARE(selection.anchorNodeSelectionModel()->selectedCount(), 1);
    selection.select(track, SelectionModel::Select | SelectionModel::SetCurrentItem);
    QCOMPARE(selection.selectionType(), SelectionModel::ST_Track);
    QCOMPARE(selection.anchorNodeSelectionModel()->selectedCount(), 0);
}

void SelectionModelTest::anchorNodeInvalidationEmitsSignals()
{
    OrmTestContext context;
    SelectionModel selection(&context.model);

    Parameter *parameter = nullptr;
    AnchorNode *anchor1 = nullptr;
    AnchorNode *anchor2 = nullptr;

    context.withTransaction([&] {
        parameter = context.model.createParameter();
        anchor1 = context.model.createAnchorNode();
        anchor2 = context.model.createAnchorNode();
        anchor1->setX(0);
        anchor2->setX(120);

        QVERIFY(parameter->anchorTransform()->insertItem(anchor1));
        QVERIFY(parameter->anchorEdited()->insertItem(anchor2));
    });

    selection.select(anchor1, SelectionModel::Select | SelectionModel::SetCurrentItem);
    QCOMPARE(selection.anchorNodeSelectionModel()->selectedCount(), 1);

    QSignalSpy selectedItemsSpy(selection.anchorNodeSelectionModel(), &AnchorNodeSelectionModel::selectedItemsChanged);
    QSignalSpy selectedCountSpy(selection.anchorNodeSelectionModel(), &AnchorNodeSelectionModel::selectedCountChanged);
    QSignalSpy currentSpy(selection.anchorNodeSelectionModel(), &AnchorNodeSelectionModel::currentItemChanged);

    context.withTransaction([&] {
        QVERIFY(parameter->anchorTransform()->removeItem(anchor1));
    });

    QCOMPARE(selection.anchorNodeSelectionModel()->selectedCount(), 0);
    QCOMPARE(selection.anchorNodeSelectionModel()->currentItem(), nullptr);
    QCOMPARE(selection.anchorNodeSelectionModel()->anchorNodeSequenceWithSelectedItems(), parameter->anchorTransform());
    QCOMPARE(selectedItemsSpy.count(), 1);
    QCOMPARE(selectedCountSpy.count(), 1);
    QCOMPARE(currentSpy.count(), 1);

    selection.select(anchor2, SelectionModel::Select | SelectionModel::SetCurrentItem);
    QCOMPARE(selection.anchorNodeSelectionModel()->selectedCount(), 1);

    context.withTransaction([&] {
        QVERIFY(parameter->anchorEdited()->moveItem(anchor2, parameter->anchorTransform()));
    });

    QCOMPARE(selection.anchorNodeSelectionModel()->selectedCount(), 0);
    QCOMPARE(selection.anchorNodeSelectionModel()->currentItem(), nullptr);
}

QTEST_MAIN(SelectionModelTest)

#include "tst_selectionmodel.moc"
