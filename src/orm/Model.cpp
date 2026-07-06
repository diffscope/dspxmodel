#include "Model.h"
#include "Model_p.h"

#include <algorithm>
#include <cstddef>
#include <cstdint>
#include <stdexcept>
#include <utility>
#include <variant>

#include <opendspx/model.h>

#include <dini/engine.h>
#include <dini/transaction.h>

#include <dspxmodelCore/Document.h>
#include <dspxmodelCore/Schema.h>
#include <dspxmodelORM/AnchorNodeSequence.h>
#include <dspxmodelORM/OpenDSPXConversion.h>
#include <dspxmodelORM/PhonemeSequence.h>
#include <dspxmodelORM/VibratoPointDataArray.h>
#include <dspxmodelORM/private/AnchorNode_p.h>
#include <dspxmodelORM/private/AnchorNodeSequence_p.h>
#include <dspxmodelORM/private/ConversionUtils_p.h>
#include <dspxmodelORM/private/Clip_p.h>
#include <dspxmodelORM/private/KeySignature_p.h>
#include <dspxmodelORM/private/KeySignatureSequence_p.h>
#include <dspxmodelORM/private/Label_p.h>
#include <dspxmodelORM/private/LabelSequence_p.h>
#include <dspxmodelORM/private/Note_p.h>
#include <dspxmodelORM/private/ORMBinding_p.h>
#include <dspxmodelORM/private/ORMUtils_p.h>
#include <dspxmodelORM/private/Parameter_p.h>
#include <dspxmodelORM/private/Phoneme_p.h>
#include <dspxmodelORM/private/Tempo_p.h>
#include <dspxmodelORM/private/TempoSequence_p.h>
#include <dspxmodelORM/private/TimeSignature_p.h>
#include <dspxmodelORM/private/TimeSignatureSequence_p.h>
#include <dspxmodelORM/private/Track_p.h>
#include <dspxmodelORM/private/TrackList_p.h>

namespace dspx {

    namespace {

        Handle modelHandleFromDocument(Document *document) {
            if (!document || !document->engine()) {
                throw std::logic_error("Model requires a valid document");
            }
            const auto models = document->engine()->view(Schema::modelTable()).limit(1).toVector();
            if (models.empty()) {
                throw std::logic_error("document has no model row");
            }
            return orm::handleFromId(models.front().id);
        }

        const std::vector<orm::ColumnBinding<Model>> &modelColumnBindings() {
            static const std::vector<orm::ColumnBinding<Model>> bindings {
                orm::stringFieldWithSignal<Model, ModelPrivate>(Schema::modelProjectNameColumn(), &ModelPrivate::projectName, &Model::projectNameChanged),
                orm::stringFieldWithSignal<Model, ModelPrivate>(Schema::modelProjectAuthorColumn(), &ModelPrivate::projectAuthor, &Model::projectAuthorChanged),
                orm::intFieldWithSignal<Model, ModelPrivate>(Schema::modelGlobalCentShiftColumn(), &ModelPrivate::globalCentShift, &Model::globalCentShiftChanged),
                orm::boolFieldWithSignal<Model, ModelPrivate>(Schema::modelMultiChannelOutputColumn(), &ModelPrivate::multiChannelOutput, &Model::multiChannelOutputChanged),
                orm::doubleFieldWithSignal<Model, ModelPrivate>(Schema::modelGainColumn(), &ModelPrivate::gain, &Model::gainChanged),
                orm::doubleFieldWithSignal<Model, ModelPrivate>(Schema::modelPanColumn(), &ModelPrivate::pan, &Model::panChanged),
                orm::boolFieldWithSignal<Model, ModelPrivate>(Schema::modelMuteColumn(), &ModelPrivate::mute, &Model::muteChanged),
                orm::boolFieldWithSignal<Model, ModelPrivate>(Schema::modelLoopEnabledColumn(), &ModelPrivate::loopEnabled, &Model::loopEnabledChanged),
                orm::intFieldWithSignal<Model, ModelPrivate>(Schema::modelLoopStartColumn(), &ModelPrivate::loopStart, &Model::loopStartChanged),
                orm::intFieldWithSignal<Model, ModelPrivate>(Schema::modelLoopLengthColumn(), &ModelPrivate::loopLength, &Model::loopLengthChanged),
                orm::binaryField<Model, ModelPrivate>(Schema::modelWorkspaceColumn(), &ModelPrivate::workspaceData, nullptr),
            };
            return bindings;
        }

    }

    namespace orm {

        dini::DocumentEngine *getModelEngine(ModelPrivate &model) {
            return model.engine;
        }

        const TableBinding &modelTableBinding() {
            static const TableBinding binding {
                .table = Schema::modelTable(),
                .columnUpdated = [](ModelPrivate &model, const dini::ColumnUpdatedChange &change) {
                    if (orm::handleFromId(change.itemId) == model.modelHandle) {
                        model.applyModelColumn(change.column, change.newValue, true);
                    }
                },
            };
            return binding;
        }

    }

    ModelPrivate::ModelPrivate(Model *q, Document *document)
        : q_ptr(q), document(document), engine(document ? document->engine() : nullptr), modelHandle(q ? q->handle() : Handle {}) {
    }

    ModelPrivate::~ModelPrivate() {
        subscription.disconnect();
    }

    void ModelPrivate::initialize() {
        Q_Q(Model);
        labels = LabelSequencePrivate::create(q);
        keySignatures = KeySignatureSequencePrivate::create(q);
        tempos = TempoSequencePrivate::create(q);
        timeSignatures = TimeSignatureSequencePrivate::create(q);
        tracks = TrackListPrivate::create(q);
        tableBindings = {&orm::modelTableBinding(), &orm::anchorNodeTableBinding(), &orm::labelTableBinding(), &orm::keySignatureTableBinding(), &orm::tempoTableBinding(), &orm::timeSignatureTableBinding(), &orm::clipTableBinding(), &orm::dynamicMixingAnchorTableBinding(), &orm::noteTableBinding(), &orm::parameterTableBinding(), &orm::phonemeTableBinding(), &orm::sourcesTableBinding()};
        listBindings = {&orm::trackListBinding(), &orm::singerListBinding(), &orm::freeValueDataArrayBinding(), &orm::vibratoPointDataArrayBinding()};

        syncModel(false);
        refreshContainers(false);

        subscription = engine->subscribe([this](const dini::EngineEvent &event) {
            handleEvent(event);
        });
    }

    void ModelPrivate::syncModel(bool notify) {
        Q_Q(Model);
        const auto snapshot = engine->read(orm::idFromHandle(modelHandle));
        orm::syncColumnBindings(modelColumnBindings(), q, snapshot, notify);
    }

    bool ModelPrivate::applyModelColumn(const dini::ColumnHandle &column, const dini::Value &value, bool notify) {
        return orm::applyColumnBinding(modelColumnBindings(), q_ptr, column, value, notify);
    }

    dini::ByteArray ModelPrivate::workspace() const {
        return workspaceData;
    }

    void ModelPrivate::setWorkspace(dini::ByteArray workspace) {
        update(modelHandle, Schema::modelWorkspaceColumn(), dini::Value(std::move(workspace)));
    }

    void ModelPrivate::refreshContainers(bool notify) {
        LabelSequencePrivate::get(labels)->refresh(notify);
        KeySignatureSequencePrivate::get(keySignatures)->refresh(notify);
        TempoSequencePrivate::get(tempos)->refresh(notify);
        TimeSignatureSequencePrivate::get(timeSignatures)->refresh(notify);
        TrackListPrivate::get(tracks)->refresh(notify, notify);
    }

    bool ModelPrivate::isModelValue(const dini::Value &value) const {
        return orm::handleFromValue(value) == modelHandle;
    }

    dini::Transaction *ModelPrivate::requireTransaction() const {
        auto *transaction = document ? document->transaction() : nullptr;
        if (!transaction || transaction->state() != dini::TransactionState::Active) {
            throw std::logic_error("ORM writes require an existing active document transaction");
        }
        return transaction;
    }

    void ModelPrivate::handleEvent(const dini::EngineEvent &event) {
        if (destroying || event.kind != dini::EventKind::AfterApply) {
            return;
        }
        std::vector<dini::ColumnUpdatedChange> pendingColumnUpdates;
        auto flushColumnUpdates = [&]() {
            if (pendingColumnUpdates.empty()) {
                return;
            }

            std::vector<bool> handled(pendingColumnUpdates.size(), false);
            for (const auto *binding : tableBindings) {
                if (!binding || !binding->columnUpdates) {
                    continue;
                }
                std::vector<dini::ColumnUpdatedChange> bindingUpdates;
                for (std::size_t i = 0; i < pendingColumnUpdates.size(); ++i) {
                    if (pendingColumnUpdates[i].column.containerId() == binding->table.containerId()) {
                        bindingUpdates.push_back(pendingColumnUpdates[i]);
                        handled[i] = true;
                    }
                }
                if (!bindingUpdates.empty()) {
                    binding->columnUpdates(*this, bindingUpdates);
                }
            }

            for (std::size_t i = 0; i < pendingColumnUpdates.size(); ++i) {
                if (!handled[i]) {
                    applyColumnUpdated(pendingColumnUpdates[i]);
                }
            }
            pendingColumnUpdates.clear();
        };

        for (const auto &operation : event.changeSet.operations()) {
            if (operation.kind() == dini::ChangeOperationKind::ColumnUpdated) {
                pendingColumnUpdates.push_back(std::get<dini::ColumnUpdatedChange>(operation.payload()));
                continue;
            }
            flushColumnUpdates();
            applyOperation(operation);
        }
        flushColumnUpdates();
    }

    const orm::TableBinding *ModelPrivate::tableBinding(dini::ContainerId containerId) const {
        const auto it = std::find_if(tableBindings.begin(), tableBindings.end(), [containerId](const auto *binding) {
            return binding && binding->table.containerId() == containerId;
        });
        return it == tableBindings.end() ? nullptr : *it;
    }

    const orm::ListBinding *ModelPrivate::listBinding(dini::ContainerId containerId) const {
        const auto it = std::find_if(listBindings.begin(), listBindings.end(), [containerId](const auto *binding) {
            return binding && binding->list.containerId() == containerId;
        });
        return it == listBindings.end() ? nullptr : *it;
    }

    void ModelPrivate::applyItemInserted(const dini::ItemInsertedChange &change) {
        if (change.item.containerKind == dini::ContainerKind::Table) {
            if (const auto *binding = tableBinding(change.item.containerId); binding && binding->itemInserted) {
                binding->itemInserted(*this, change);
            }
            return;
        }
        if (change.item.containerKind == dini::ContainerKind::List) {
            if (const auto *binding = listBinding(change.item.containerId); binding && binding->itemInserted) {
                binding->itemInserted(*this, change);
            }
        }
    }

    void ModelPrivate::applyItemRemoved(const dini::ItemSnapshot &snapshot, bool cascade) {
        if (snapshot.containerKind == dini::ContainerKind::Table) {
            if (const auto *binding = tableBinding(snapshot.containerId); binding && binding->itemRemoved) {
                binding->itemRemoved(*this, snapshot, cascade);
            }
            return;
        }
        if (snapshot.containerKind == dini::ContainerKind::List) {
            if (const auto *binding = listBinding(snapshot.containerId); binding && binding->itemRemoved) {
                binding->itemRemoved(*this, snapshot, cascade);
            }
        }
    }

    void ModelPrivate::applyListInserted(const dini::ListInsertedChange &change) {
        if (const auto *binding = listBinding(change.list.containerId()); binding && binding->listInserted) {
            binding->listInserted(*this, change);
        }
    }

    void ModelPrivate::applyListRemoved(const dini::ListRemovedChange &change) {
        if (const auto *binding = listBinding(change.list.containerId()); binding && binding->listRemoved) {
            binding->listRemoved(*this, change);
        }
    }

    void ModelPrivate::applyColumnUpdated(const dini::ColumnUpdatedChange &change) {
        if (const auto *binding = tableBinding(change.column.containerId()); binding && binding->columnUpdated) {
            binding->columnUpdated(*this, change);
            return;
        }
        if (const auto *binding = listBinding(change.column.containerId()); binding && binding->columnUpdated) {
            binding->columnUpdated(*this, change);
        }
    }

    void ModelPrivate::applyComputedColumnUpdated(const dini::ComputedColumnUpdatedChange &change) {
        if (const auto *binding = tableBinding(change.column.containerId()); binding && binding->computedColumnUpdated) {
            binding->computedColumnUpdated(*this, change);
            return;
        }
        if (const auto *binding = listBinding(change.column.containerId()); binding && binding->computedColumnUpdated) {
            binding->computedColumnUpdated(*this, change);
        }
    }

    void ModelPrivate::applyListRotated(const dini::ListRotatedChange &change) {
        if (const auto *binding = listBinding(change.list.containerId()); binding && binding->listRotated) {
            binding->listRotated(*this, change);
        }
    }

    void ModelPrivate::applyOperation(const dini::ChangeOperation &operation) {
        std::visit(orm::Overloaded {
                       [this](const dini::ItemInsertedChange &change) { applyItemInserted(change); },
                       [this](const dini::ItemRemovedChange &change) { applyItemRemoved(change.item, change.cascade); },
                       [this](const dini::CascadeRemovedChange &change) { applyItemRemoved(change.item, true); },
                       [this](const dini::ColumnUpdatedChange &change) { applyColumnUpdated(change); },
                       [this](const dini::ComputedColumnUpdatedChange &change) { applyComputedColumnUpdated(change); },
                       [this](const dini::ListInsertedChange &change) { applyListInserted(change); },
                       [this](const dini::ListRemovedChange &change) { applyListRemoved(change); },
                       [this](const dini::ListRotatedChange &change) { applyListRotated(change); },
                   },
                   operation.payload());
    }

    Model::Model(Document *document, QObject *parent)
        : EntityObject(modelHandleFromDocument(document), this, parent), d_ptr(new ModelPrivate(this, document)) {
        d_ptr->initialize();
    }

    Model::~Model() {
        Q_D(Model);
        d->destroying = true;
        d->subscription.disconnect();
    }

    Document *Model::document() const {
        Q_D(const Model);
        return d->document;
    }

    QString Model::projectName() const {
        Q_D(const Model);
        return d->projectName;
    }

    void Model::setProjectName(const QString &projectName) {
        Q_D(Model);
        d->update(handle(), Schema::modelProjectNameColumn(), orm::valueFromString(projectName));
    }

    QString Model::projectAuthor() const {
        Q_D(const Model);
        return d->projectAuthor;
    }

    void Model::setProjectAuthor(const QString &projectAuthor) {
        Q_D(Model);
        d->update(handle(), Schema::modelProjectAuthorColumn(), orm::valueFromString(projectAuthor));
    }

    int Model::globalCentShift() const {
        Q_D(const Model);
        return d->globalCentShift;
    }

    void Model::setGlobalCentShift(int globalCentShift) {
        Q_D(Model);
        d->update(handle(), Schema::modelGlobalCentShiftColumn(), dini::Value(static_cast<std::int64_t>(globalCentShift)));
    }

    bool Model::multiChannelOutput() const {
        Q_D(const Model);
        return d->multiChannelOutput;
    }

    void Model::setMultiChannelOutput(bool multiChannelOutput) {
        Q_D(Model);
        d->update(handle(), Schema::modelMultiChannelOutputColumn(), dini::Value(multiChannelOutput));
    }

    double Model::gain() const {
        Q_D(const Model);
        return d->gain;
    }

    void Model::setGain(double gain) {
        Q_D(Model);
        d->update(handle(), Schema::modelGainColumn(), dini::Value(gain));
    }

    double Model::pan() const {
        Q_D(const Model);
        return d->pan;
    }

    void Model::setPan(double pan) {
        Q_D(Model);
        d->update(handle(), Schema::modelPanColumn(), dini::Value(pan));
    }

    bool Model::mute() const {
        Q_D(const Model);
        return d->mute;
    }

    void Model::setMute(bool mute) {
        Q_D(Model);
        d->update(handle(), Schema::modelMuteColumn(), dini::Value(mute));
    }

    bool Model::loopEnabled() const {
        Q_D(const Model);
        return d->loopEnabled;
    }

    void Model::setLoopEnabled(bool loopEnabled) {
        Q_D(Model);
        d->update(handle(), Schema::modelLoopEnabledColumn(), dini::Value(loopEnabled));
    }

    int Model::loopStart() const {
        Q_D(const Model);
        return d->loopStart;
    }

    void Model::setLoopStart(int loopStart) {
        Q_D(Model);
        d->update(handle(), Schema::modelLoopStartColumn(), dini::Value(static_cast<std::int64_t>(loopStart)));
    }

    int Model::loopLength() const {
        Q_D(const Model);
        return d->loopLength;
    }

    void Model::setLoopLength(int loopLength) {
        Q_D(Model);
        d->update(handle(), Schema::modelLoopLengthColumn(), dini::Value(static_cast<std::int64_t>(loopLength)));
    }

    LabelSequence *Model::labels() const {
        Q_D(const Model);
        return d->labels;
    }

    KeySignatureSequence *Model::keySignatures() const {
        Q_D(const Model);
        return d->keySignatures;
    }

    TempoSequence *Model::tempos() const {
        Q_D(const Model);
        return d->tempos;
    }

    TimeSignatureSequence *Model::timeSignatures() const {
        Q_D(const Model);
        return d->timeSignatures;
    }

    TrackList *Model::tracks() const {
        Q_D(const Model);
        return d->tracks;
    }

    opendspx::Model Model::toOpenDSPX() const {
        Q_D(const Model);
        auto target = opendspx::Model{
            .content = {
                .global = {
                    .author = projectAuthor().toStdString(),
                    .name = projectName().toStdString(),
                    .centShift = globalCentShift(),
                },
                .master = {
                    .control = {
                        .gain = conv::toDecibel(gain()),
                        .pan = pan(),
                        .mute = mute(),
                    }
                },
                .timeline = {
                    .labels = labels()->toOpenDSPX(),
                    .tempos = tempos()->toOpenDSPX(),
                    .timeSignatures = timeSignatures()->toOpenDSPX(),
                },
                .tracks = tracks()->toOpenDSPX(),
            }
        };
        target.content.workspace = conv::deserializeWorkspace(d->workspace());
        auto &diffscope = conv::ensureObject(target.content.workspace["diffscope"]);
        auto &loop = conv::ensureObjectMember(diffscope, "loop");
        loop["enabled"] = loopEnabled();
        loop["start"] = loopStart();
        loop["length"] = loopLength();
        auto &master = conv::ensureObjectMember(diffscope, "master");
        master["multiChannelOutput"] = multiChannelOutput();
        diffscope["keySignatures"] = keySignatures()->toOpenDSPX();
        OpenDSPXConversion::convertModelToOpenDSPX(this, target);
        return target;

    }

    void Model::fromOpenDspx(const opendspx::Model &model) {
        Q_D(Model);
        d->setWorkspace(conv::serializeWorkspace(model.content.workspace));
        setProjectAuthor(QString::fromStdString(model.content.global.author));
        setProjectName(QString::fromStdString(model.content.global.name));
        setGlobalCentShift(model.content.global.centShift);
        setGain(conv::fromDecibel(model.content.master.control.gain));
        setPan(model.content.master.control.pan);
        setMute(model.content.master.control.mute);
        labels()->fromOpenDSPX(model.content.timeline.labels);
        tempos()->fromOpenDSPX(model.content.timeline.tempos);
        timeSignatures()->fromOpenDSPX(model.content.timeline.timeSignatures);
        tracks()->fromOpenDSPX(model.content.tracks);
        if (auto it = model.content.workspace.find("diffscope"); it != model.content.workspace.end()) {
            const auto &workspace = it->second;
            if (auto v = conv::optionalChain(workspace, "loop", "enabled"); v.is_boolean()) {
                setLoopEnabled(v.get<bool>());
            }
            if (auto v = conv::optionalChain(workspace, "loop", "start"); v.is_number_integer() && v.get<int>() >= 0) {
                setLoopStart(v.get<int>());
            }
            if (auto v = conv::optionalChain(workspace, "loop", "length"); v.is_number_integer() && v.get<int>() > 0) {
                setLoopLength(v.get<int>());
            }
            if (auto v = conv::optionalChain(workspace, "master", "multiChannelOutput"); v.is_boolean()) {
                setMultiChannelOutput(v.get<bool>());
            }
            if (auto v = conv::optionalChain(workspace, "keySignatures"); v.is_array()) {
                keySignatures()->fromOpenDSPX(v.get<std::vector<nlohmann::json>>());
            }
        }
        OpenDSPXConversion::convertModelFromOpenDSPX(this, model);
    }

    Label *Model::createLabel() {
        Q_D(Model);
        const auto id = d->requireTransaction()->insert(Schema::labelTable(), {
            dini::ColumnValue {.column = Schema::labelParent().column(), .value = dini::Value::null()},
        });
        return d->ensure<Label>(orm::handleFromId(id));
    }

    KeySignature *Model::createKeySignature() {
        Q_D(Model);
        const auto id = d->requireTransaction()->insert(Schema::keySignatureTable(), {
            dini::ColumnValue {.column = Schema::keySignatureParent().column(), .value = dini::Value::null()},
        });
        return d->ensure<KeySignature>(orm::handleFromId(id));
    }

    Tempo *Model::createTempo() {
        Q_D(Model);
        const auto id = d->requireTransaction()->insert(Schema::tempoTable(), {
            dini::ColumnValue {.column = Schema::tempoParent().column(), .value = dini::Value::null()},
        });
        return d->ensure<Tempo>(orm::handleFromId(id));
    }

    TimeSignature *Model::createTimeSignature() {
        Q_D(Model);
        const auto id = d->requireTransaction()->insert(Schema::timeSignatureTable(), {
            dini::ColumnValue {.column = Schema::timeSignatureParent().column(), .value = dini::Value::null()},
        });
        return d->ensure<TimeSignature>(orm::handleFromId(id));
    }

    Track *Model::createTrack() {
        Q_D(Model);
        const auto id = d->requireTransaction()->insert(Schema::trackList(), dini::Value::null(), 0, {});
        return d->ensure<Track>(orm::handleFromId(id));
    }

    AudioClip *Model::createAudioClip() {
        Q_D(Model);
        const auto id = d->requireTransaction()->insert(Schema::clipTable(), {
            dini::ColumnValue {.column = Schema::clipParent().column(), .value = dini::Value::null()},
        }, Schema::audioClipVariant());
        return d->ensure<AudioClip>(orm::handleFromId(id));
    }

    SingingClip *Model::createSingingClip() {
        Q_D(Model);
        const auto id = d->requireTransaction()->insert(Schema::clipTable(), {
            dini::ColumnValue {.column = Schema::clipParent().column(), .value = dini::Value::null()},
        }, Schema::singingClipVariant());
        return d->ensure<SingingClip>(orm::handleFromId(id));
    }

    Note *Model::createNote() {
        Q_D(Model);
        auto *transaction = d->requireTransaction();
        const auto id = transaction->insert(Schema::noteTable(), {
            dini::ColumnValue {.column = Schema::noteParent().column(), .value = dini::Value::null()},
        });
        const auto noteValue = dini::Value(static_cast<std::uint64_t>(id));
        transaction->insert(Schema::noteVibratoPointRelationTable(), {
            dini::ColumnValue {.column = Schema::noteVibratoPointRelationParent().column(), .value = noteValue},
            dini::ColumnValue {.column = Schema::noteVibratoPointRelationRoleColumn(), .value = dini::Value(static_cast<std::int64_t>(VibratoPointDataArray::Amplitude))},
        });
        transaction->insert(Schema::noteVibratoPointRelationTable(), {
            dini::ColumnValue {.column = Schema::noteVibratoPointRelationParent().column(), .value = noteValue},
            dini::ColumnValue {.column = Schema::noteVibratoPointRelationRoleColumn(), .value = dini::Value(static_cast<std::int64_t>(VibratoPointDataArray::Frequency))},
        });
        transaction->insert(Schema::notePhonemeRelationTable(), {
            dini::ColumnValue {.column = Schema::notePhonemeRelationParent().column(), .value = noteValue},
            dini::ColumnValue {.column = Schema::notePhonemeRelationRoleColumn(), .value = dini::Value(static_cast<std::int64_t>(PhonemeSequence::Original))},
        });
        transaction->insert(Schema::notePhonemeRelationTable(), {
            dini::ColumnValue {.column = Schema::notePhonemeRelationParent().column(), .value = noteValue},
            dini::ColumnValue {.column = Schema::notePhonemeRelationRoleColumn(), .value = dini::Value(static_cast<std::int64_t>(PhonemeSequence::Edited))},
        });
        return d->ensure<Note>(orm::handleFromId(id));
    }

    Phoneme *Model::createPhoneme() {
        Q_D(Model);
        const auto id = d->requireTransaction()->insert(Schema::phonemeTable(), {
            dini::ColumnValue {.column = Schema::phonemeParent().column(), .value = dini::Value::null()},
        });
        return d->ensure<Phoneme>(orm::handleFromId(id));
    }

    Parameter *Model::createParameter() {
        Q_D(Model);
        auto *transaction = d->requireTransaction();
        const auto id = transaction->insert(Schema::parameterTable(), {
            dini::ColumnValue {.column = Schema::parameterParent().column(), .value = dini::Value::null()},
            dini::ColumnValue {.column = Schema::parameterKeyColumn(), .value = dini::Value::null()},
        });
        const auto parameterValue = dini::Value(static_cast<std::uint64_t>(id));
        for (int role = 0; role < 3; ++role) {
            transaction->insert(Schema::parameterFreeValueRelation(), {
                dini::ColumnValue {.column = Schema::freeValueRelationParent().column(), .value = parameterValue},
                dini::ColumnValue {.column = Schema::freeValueRelationRoleColumn(), .value = dini::Value(static_cast<std::int64_t>(role))},
            });
        }
        transaction->insert(Schema::parameterAnchorNodeRelationTable(), {
            dini::ColumnValue {.column = Schema::parameterAnchorNodeRelationParent().column(), .value = parameterValue},
            dini::ColumnValue {.column = Schema::parameterAnchorNodeRelationRoleColumn(), .value = dini::Value(static_cast<std::int64_t>(AnchorNodeSequence::Transform))},
        });
        transaction->insert(Schema::parameterAnchorNodeRelationTable(), {
            dini::ColumnValue {.column = Schema::parameterAnchorNodeRelationParent().column(), .value = parameterValue},
            dini::ColumnValue {.column = Schema::parameterAnchorNodeRelationRoleColumn(), .value = dini::Value(static_cast<std::int64_t>(AnchorNodeSequence::Edited))},
        });
        return d->ensure<Parameter>(orm::handleFromId(id));
    }

    AnchorNode *Model::createAnchorNode() {
        Q_D(Model);
        const auto id = d->requireTransaction()->insert(Schema::anchorNodeTable(), {
            dini::ColumnValue {.column = Schema::anchorNodeParent().column(), .value = dini::Value::null()},
        });
        return d->ensure<AnchorNode>(orm::handleFromId(id));
    }

    Sources *Model::createSources() {
        Q_D(Model);
        const auto id = d->requireTransaction()->insert(Schema::sourcesTable(), {
            dini::ColumnValue {.column = Schema::sourcesParent().column(), .value = dini::Value::null()},
        });
        return d->ensure<Sources>(orm::handleFromId(id));
    }

    SingleSinger *Model::createSingleSinger() {
        Q_D(Model);
        const auto id = d->requireTransaction()->insert(Schema::singerList(),
                                                        dini::Value::null(),
                                                        0,
                                                        {},
                                                        Schema::singleSingerVariant());
        return d->ensure<SingleSinger>(orm::handleFromId(id));
    }

    MixedSinger *Model::createMixedSinger() {
        Q_D(Model);
        const auto id = d->requireTransaction()->insert(Schema::singerList(),
                                                        dini::Value::null(),
                                                        0,
                                                        {},
                                                        Schema::mixedSingerVariant());
        return d->ensure<MixedSinger>(orm::handleFromId(id));
    }

    DynamicMixingAnchor *Model::createDynamicMixingAnchor() {
        Q_D(Model);
        const auto id = d->requireTransaction()->insert(Schema::dynamicMixingAnchorTable(), {
            dini::ColumnValue {.column = Schema::dynamicMixingAnchorParent().column(), .value = dini::Value::null()},
        });
        return d->ensure<DynamicMixingAnchor>(orm::handleFromId(id));
    }

    bool Model::destroyItem(EntityObject *item) {
        Q_D(Model);
        if (!item || item->model() != this || !item->handle()) {
            return false;
        }
        d->requireTransaction()->remove(orm::idFromHandle(item->handle()));
        return true;
    }

}


#include "moc_Model.cpp"
