#include <algorithm>
#include <cstdint>
#include <functional>
#include <iterator>
#include <random>
#include <utility>
#include <vector>

#include <QtCore/QDateTime>
#include <QtCore/QFile>
#include <QtCore/QJsonArray>
#include <QtCore/QJsonDocument>
#include <QtCore/QJsonObject>
#include <QtGui/QUndoStack>
#include <QtTest/QtTest>

#include <dini/engine.h>
#include <dini/transaction.h>

#include <dspxmodelCore/Document.h>
#include <dspxmodelORM/ClipSequence.h>
#include <dspxmodelORM/FreeValueDataArray.h>
#include <dspxmodelORM/Model.h>
#include <dspxmodelORM/Note.h>
#include <dspxmodelORM/NoteSequence.h>
#include <dspxmodelORM/Parameter.h>
#include <dspxmodelORM/ParameterMap.h>
#include <dspxmodelORM/SingingClip.h>
#include <dspxmodelORM/Track.h>
#include <dspxmodelORM/TrackList.h>
#include <dspxmodelORM/VibratoPointDataArray.h>

namespace {

constexpr int initialValueCount = 4096;
constexpr int fuzzStepCount = 10000;
constexpr std::uint32_t fuzzSeed = 0x46564544U;
constexpr int maximumGeneratedSpliceLength = 96;
constexpr int maximumUndoBatchSize = 64;

class TestObject {
public:
    virtual ~TestObject() = default;

    virtual int size() const = 0;
    virtual QList<int> items() const = 0;

    virtual void splice(int index, int length, const QList<int> &values) = 0;
    virtual void rotate(int leftIndex, int middleIndex, int rightIndex) = 0;

    virtual int step() const = 0;
    virtual void undo() = 0;
    virtual void redo() = 0;
    virtual void destroyAndUndo() = 0;
};

class FreeValueDataArrayTestObject : public TestObject {
public:
    FreeValueDataArrayTestObject() {
        withTransaction([&] {
            m_track = m_model.createTrack();
            m_model.tracks()->insertItem(0, m_track);

            m_clip = m_model.createSingingClip();
            m_clip->setPosition(0);
            m_clip->setClipStart(0);
            m_track->clips()->insertItem(m_clip);

            auto *parameter = m_model.createParameter();
            m_clip->parameters()->insertItem(QStringLiteral("pitch"), parameter);
            m_array = parameter->freeEdited();
        });
        m_document.engine()->clearUndoHistory();
    }

    int size() const override {
        return m_array->size();
    }

    QList<int> items() const override {
        QList<int> result;
        const auto source = m_array->items();
        result.reserve(source.size());
        for (const auto &value : source) {
            result.append(value.isValid() ? value.toInt() : 0);
        }
        return result;
    }

    void splice(int index, int length, const QList<int> &values) override {
        QList<QVariant> variants;
        variants.reserve(values.size());
        for (const int value : values) {
            variants.append(QVariant(value));
        }
        withTransaction([&] {
            if (!m_array->splice(index, length, variants)) {
                qCritical() << "free_value: failed to splice array" << index << length << variants.size() << m_array->size();
            }
        });
    }

    void rotate(int leftIndex, int middleIndex, int rightIndex) override {
        withTransaction([&] {
            if (!m_array->rotate(leftIndex, middleIndex, rightIndex)) {
                qCritical() << "free_value: failed to rotate array" << leftIndex << middleIndex << rightIndex << m_array->size();
            }
        });
    }

    int step() const override {
        return static_cast<int>(m_document.engine()->undoHistory().size());
    }

    void undo() override {
        m_document.engine()->undo();
    }

    void redo() override {
        m_document.engine()->redo();
    }

    void destroyAndUndo() override {
        const auto clipHandle = m_clip->handle();
        const auto parameterHandle = m_array->parameter()->handle();
        withTransaction([&] {
            m_track->clips()->removeItem(m_clip);
            m_model.destroyItem(m_clip);
        });
        m_document.engine()->undo();

        m_clip = m_model.find<dspx::SingingClip>(clipHandle);
        auto *parameter = m_model.find<dspx::Parameter>(parameterHandle);
        Q_ASSERT(m_clip);
        Q_ASSERT(parameter);
        m_array = parameter->freeEdited();
        Q_ASSERT(m_array);
    }

private:
    dspx::Document m_document;
    dspx::Model m_model {&m_document};
    dspx::Track *m_track = nullptr;
    dspx::SingingClip *m_clip = nullptr;
    dspx::FreeValueDataArray *m_array = nullptr;

    template <typename Func>
    void withTransaction(Func &&func) {
        auto transaction = m_document.engine()->beginTransaction();
        m_document.setTransaction(&transaction);
        try {
            std::invoke(std::forward<Func>(func));
            transaction.commit();
            m_document.setTransaction(nullptr);
        } catch (...) {
            m_document.setTransaction(nullptr);
            throw;
        }
    }
};

class VibratoPointDataArrayTestObject : public TestObject {
public:
    VibratoPointDataArrayTestObject() {
        withTransaction([&] {
            m_track = m_model.createTrack();
            m_model.tracks()->insertItem(0, m_track);

            m_clip = m_model.createSingingClip();
            m_clip->setPosition(0);
            m_clip->setClipStart(0);
            m_track->clips()->insertItem(m_clip);

            auto *note = m_model.createNote();
            note->setPosition(0);
            note->setLength(480);
            note->setKeyNumber(60);
            m_clip->notes()->insertItem(note);
            m_array = note->vibratoAmplitudeControlPoints();
        });
        m_document.engine()->clearUndoHistory();
    }

    int size() const override {
        return m_array->size();
    }

    QList<int> items() const override {
        QList<int> result;
        const auto source = m_array->items();
        result.reserve(source.size());
        for (const auto &point : source) {
            result.append(static_cast<int>(point.x()));
        }
        return result;
    }

    void splice(int index, int length, const QList<int> &values) override {
        QList<QPointF> points;
        points.reserve(values.size());
        for (const int value : values) {
            points.append(QPointF(static_cast<qreal>(value), 0.0));
        }
        withTransaction([&] {
            if (!m_array->splice(index, length, points)) {
                qCritical() << "vibrato_point: failed to splice array" << index << length << points.size() << m_array->size();
            }
        });
    }

    void rotate(int leftIndex, int middleIndex, int rightIndex) override {
        withTransaction([&] {
            if (!m_array->rotate(leftIndex, middleIndex, rightIndex)) {
                qCritical() << "vibrato_point: failed to rotate array" << leftIndex << middleIndex << rightIndex << m_array->size();
            }
        });
    }

    int step() const override {
        return static_cast<int>(m_document.engine()->undoHistory().size());
    }

    void undo() override {
        m_document.engine()->undo();
    }

    void redo() override {
        m_document.engine()->redo();
    }

    void destroyAndUndo() override {
        const auto clipHandle = m_clip->handle();
        const auto noteHandle = m_array->note()->handle();
        withTransaction([&] {
            m_track->clips()->removeItem(m_clip);
            m_model.destroyItem(m_clip);
        });
        m_document.engine()->undo();

        m_clip = m_model.find<dspx::SingingClip>(clipHandle);
        auto *note = m_model.find<dspx::Note>(noteHandle);
        Q_ASSERT(m_clip);
        Q_ASSERT(note);
        m_array = note->vibratoAmplitudeControlPoints();
        Q_ASSERT(m_array);
    }

private:
    dspx::Document m_document;
    dspx::Model m_model {&m_document};
    dspx::Track *m_track = nullptr;
    dspx::SingingClip *m_clip = nullptr;
    dspx::VibratoPointDataArray *m_array = nullptr;

    template <typename Func>
    void withTransaction(Func &&func) {
        auto transaction = m_document.engine()->beginTransaction();
        m_document.setTransaction(&transaction);
        try {
            std::invoke(std::forward<Func>(func));
            transaction.commit();
            m_document.setTransaction(nullptr);
        } catch (...) {
            m_document.setTransaction(nullptr);
            throw;
        }
    }
};

class OracleSpliceCommand;
class OracleRotateCommand;
class OracleTestObject : public TestObject {
public:
    OracleTestObject() = default;

    int size() const override {
        return m_items.size();
    }

    QList<int> items() const override {
        return m_items;
    }

    void splice(int index, int length, const QList<int> &values) override;
    void rotate(int leftIndex, int middleIndex, int rightIndex) override;

    int step() const override {
        return m_undoStack.index();
    }

    void undo() override {
        m_undoStack.undo();
    }

    void redo() override {
        m_undoStack.redo();
    }

    void destroyAndUndo() override {
        // Match a committed destruction followed immediately by its undo. The command
        // has no list-data effect, but pushing it correctly discards an existing redo tail.
        m_undoStack.push(new QUndoCommand);
        m_undoStack.undo();
    }

private:
    friend class OracleSpliceCommand;
    friend class OracleRotateCommand;

    void applySplice(int index, int length, const QList<int> &values) {
        const auto insertedCount = static_cast<qsizetype>(values.size());
        if (insertedCount < length) {
            m_items.remove(index + insertedCount, length - insertedCount);
        } else if (insertedCount > length) {
            const auto oldSize = m_items.size();
            m_items.resize(oldSize + insertedCount - length);
            std::ranges::move_backward(m_items.begin() + index + length,
                                       m_items.begin() + oldSize,
                                       m_items.end());
        }
        if (insertedCount > 0) {
            std::ranges::copy_n(values.cbegin(), insertedCount, m_items.begin() + index);
        }
    }

    void applyRotate(int leftIndex, int middleIndex, int rightIndex) {
        std::rotate(m_items.begin() + leftIndex,
                    m_items.begin() + middleIndex,
                    m_items.begin() + rightIndex);
    }

    void applyRotateInverse(int leftIndex, int middleIndex, int rightIndex) {
        std::rotate(m_items.begin() + leftIndex,
                    m_items.begin() + leftIndex + (rightIndex - middleIndex),
                    m_items.begin() + rightIndex);
    }

    QList<int> m_items;
    QUndoStack m_undoStack;
};

class OracleSpliceCommand : public QUndoCommand {
public:
    OracleSpliceCommand(OracleTestObject &owner, int index, int length, const QList<int> &values)
        : m_owner(owner), m_index(index), m_length(length), m_values(values) {
        m_removed = owner.m_items.mid(index, length);
    }

    void redo() override {
        m_owner.applySplice(m_index, m_length, m_values);
    }

    void undo() override {
        m_owner.applySplice(m_index, static_cast<int>(m_values.size()), m_removed);
    }

private:
    OracleTestObject &m_owner;
    int m_index;
    int m_length;
    QList<int> m_values;
    QList<int> m_removed;
};

class OracleRotateCommand : public QUndoCommand {
public:
    OracleRotateCommand(OracleTestObject &owner, int leftIndex, int middleIndex, int rightIndex)
        : m_owner(owner), m_left(leftIndex), m_middle(middleIndex), m_right(rightIndex) {
    }

    void redo() override {
        m_owner.applyRotate(m_left, m_middle, m_right);
    }

    void undo() override {
        m_owner.applyRotateInverse(m_left, m_middle, m_right);
    }

private:
    OracleTestObject &m_owner;
    int m_left;
    int m_middle;
    int m_right;
};

void OracleTestObject::splice(int index, int length, const QList<int> &values) {
    // Dini does not create an undo step for an empty transaction.
    if (length == 0 && values.isEmpty()) {
        return;
    }
    auto *command = new OracleSpliceCommand(*this, index, length, values);
    m_undoStack.push(command);
}

void OracleTestObject::rotate(int leftIndex, int middleIndex, int rightIndex) {
    auto *command = new OracleRotateCommand(*this, leftIndex, middleIndex, rightIndex);
    m_undoStack.push(command);
}

struct ComparisonTarget {
    QString name;
    TestObject *object = nullptr;
};

int randomInteger(std::mt19937 &rng, int minimum, int maximum) {
    return std::uniform_int_distribution<int>(minimum, maximum)(rng);
}

int randomValue(std::mt19937 &rng) {
    // Keep values exactly representable both as QVariant integers and QPointF coordinates.
    const auto bucket = randomInteger(rng, 0, 99);
    if (bucket < 10) {
        return 0;
    }
    if (bucket < 12) {
        return -1000000;
    }
    if (bucket < 14) {
        return 1000000;
    }
    return randomInteger(rng, -1000000, 1000000);
}

QList<int> randomValues(std::mt19937 &rng, int count) {
    QList<int> result;
    result.reserve(count);
    for (int i = 0; i < count; ++i) {
        result.append(randomValue(rng));
    }
    return result;
}

QJsonArray valuesToJson(const QList<int> &values) {
    QJsonArray result;
    for (const auto value : values) {
        result.append(value);
    }
    return result;
}

QString writeLogAndCompare(QFile &logFile,
                           const QJsonObject &operation,
                           const std::vector<ComparisonTarget> &targets) {
    const auto operationType = operation.value(QStringLiteral("type")).toString();
    const auto operationFailure = [&](const QString &message) {
        return QStringLiteral("Failure after operation %1: %2").arg(operationType, message);
    };

    QJsonObject data;
    std::vector<QList<int>> snapshots;
    snapshots.reserve(targets.size());
    for (const auto &target : targets) {
        snapshots.push_back(target.object->items());
        data.insert(target.name, valuesToJson(snapshots.back()));
    }

    const QJsonObject record {
        {QStringLiteral("op"), operation},
        {QStringLiteral("data"), data},
    };
    auto line = QJsonDocument(record).toJson(QJsonDocument::Compact);
    line.append('\n');
    if (logFile.write(line) != line.size() || !logFile.flush()) {
        return operationFailure(
            QStringLiteral("failed to write fuzz log: %1").arg(logFile.errorString()));
    }

    const auto &oracle = snapshots.front();
    const auto oracleStep = targets.front().object->step();
    for (std::size_t i = 1; i < targets.size(); ++i) {
        if (snapshots[i] != oracle) {
            qsizetype differenceIndex = 0;
            const auto commonSize = qMin(oracle.size(), snapshots[i].size());
            while (differenceIndex < commonSize
                   && oracle[differenceIndex] == snapshots[i][differenceIndex]) {
                ++differenceIndex;
            }
            return operationFailure(
                QStringLiteral("differential mismatch for %1 at index %2 "
                               "(oracle size %3, actual size %4)")
                    .arg(targets[i].name)
                    .arg(differenceIndex)
                    .arg(oracle.size())
                    .arg(snapshots[i].size()));
        }
        if (targets[i].object->size() != oracle.size()) {
            return operationFailure(
                QStringLiteral("size mismatch for %1 (oracle %2, actual %3)")
                    .arg(targets[i].name)
                    .arg(oracle.size())
                    .arg(targets[i].object->size()));
        }
        if (targets[i].object->step() != oracleStep) {
            return operationFailure(
                QStringLiteral("undo step mismatch for %1 (oracle %2, actual %3)")
                    .arg(targets[i].name)
                    .arg(oracleStep)
                    .arg(targets[i].object->step()));
        }
    }
    return {};
}

template <typename Func>
QString invokeLogAndCompare(QFile &logFile,
                            const QJsonObject &operation,
                            const std::vector<ComparisonTarget> &targets,
                            Func &&func) {
    for (const auto &target : targets) {
        std::invoke(func, *target.object);
    }
    return writeLogAndCompare(logFile, operation, targets);
}

QJsonObject spliceOperation(int index, int length, const QList<int> &values) {
    return {
        {QStringLiteral("type"), QStringLiteral("splice")},
        {QStringLiteral("index"), index},
        {QStringLiteral("length"), length},
        {QStringLiteral("values"), valuesToJson(values)},
    };
}

QJsonObject rotateOperation(int leftIndex, int middleIndex, int rightIndex) {
    return {
        {QStringLiteral("type"), QStringLiteral("rotate")},
        {QStringLiteral("left_index"), leftIndex},
        {QStringLiteral("middle_index"), middleIndex},
        {QStringLiteral("right_index"), rightIndex},
    };
}

QJsonObject historyOperation(const QString &type, int undoCount, int redoCount, int ordinal) {
    return {
        {QStringLiteral("type"), type},
        {QStringLiteral("n"), undoCount},
        {QStringLiteral("m"), redoCount},
        {QStringLiteral("ordinal"), ordinal},
    };
}

} // anonymous namespace

class FreeValueEditingFuzzTest : public QObject {
    Q_OBJECT

private slots:
    void fuzzTest() {
        std::mt19937 rng(fuzzSeed);

        OracleTestObject oracle;
        FreeValueDataArrayTestObject freeValue;
        VibratoPointDataArrayTestObject vibratoPoint;

        // Adding another implementation here automatically adds it to logging and comparison.
        const std::vector<ComparisonTarget> targets {
            {QStringLiteral("oracle"), &oracle},
            {QStringLiteral("free_value"), &freeValue},
            {QStringLiteral("vibrato_point"), &vibratoPoint},
        };

        const auto logFileName = QDateTime::currentDateTimeUtc().toString(
                                     QStringLiteral("yyyyMMdd'T'HHmmss'Z'"))
                                 + QStringLiteral(".log");
        QFile logFile(logFileName);
        QVERIFY2(logFile.open(QIODevice::WriteOnly | QIODevice::Text),
                 qPrintable(QStringLiteral("Failed to open fuzz log %1: %2")
                                .arg(logFileName, logFile.errorString())));

        const auto initialValues = randomValues(rng, initialValueCount);
        auto error = invokeLogAndCompare(
            logFile,
            spliceOperation(0, 0, initialValues),
            targets,
            [&](TestObject &target) { target.splice(0, 0, initialValues); });
        if (!error.isEmpty()) {
            QFAIL(qPrintable(error));
        }

        for (int fuzzStep = 0; fuzzStep < fuzzStepCount; ++fuzzStep) {
            const auto operationBucket = randomInteger(rng, 0, 99);

            if (operationBucket < 45) {
                const auto size = oracle.size();
                const auto indexBucket = randomInteger(rng, 0, 99);
                const auto index = indexBucket < 15 ? 0
                    : indexBucket < 30             ? size
                                                   : randomInteger(rng, 0, size);
                const auto availableLength = size - index;

                const auto lengthBucket = randomInteger(rng, 0, 99);
                const auto length = lengthBucket < 25 ? 0
                    : lengthBucket < 35               ? availableLength
                    : randomInteger(rng, 0,
                                    qMin(availableLength, maximumGeneratedSpliceLength));

                const auto valueCountBucket = randomInteger(rng, 0, 99);
                const auto valueCount = valueCountBucket < 25
                    ? 0
                    : randomInteger(rng, 1, maximumGeneratedSpliceLength);
                const auto values = randomValues(rng, valueCount);

                error = invokeLogAndCompare(
                    logFile,
                    spliceOperation(index, length, values),
                    targets,
                    [&](TestObject &target) { target.splice(index, length, values); });
            } else if (operationBucket < 75) {
                const auto size = oracle.size();
                int leftIndex;
                int middleIndex;
                int rightIndex;
                if (randomInteger(rng, 0, 99) < 20) {
                    leftIndex = randomInteger(rng, 0, size);
                    middleIndex = leftIndex;
                    rightIndex = leftIndex;
                } else {
                    leftIndex = randomInteger(rng, 0, size);
                    rightIndex = randomInteger(rng, leftIndex, size);
                    middleIndex = randomInteger(rng, leftIndex, rightIndex);
                }

                error = invokeLogAndCompare(
                    logFile,
                    rotateOperation(leftIndex, middleIndex, rightIndex),
                    targets,
                    [&](TestObject &target) {
                        target.rotate(leftIndex, middleIndex, rightIndex);
                    });
            } else if (operationBucket < 85) {
                error = invokeLogAndCompare(
                    logFile,
                    {{QStringLiteral("type"), QStringLiteral("destroy_and_undo")}},
                    targets,
                    [](TestObject &target) { target.destroyAndUndo(); });
            } else {
                const auto undoableCount = oracle.step();
                const auto nBucket = randomInteger(rng, 0, 99);
                const auto undoCount = undoableCount == 0 || nBucket < 20 ? 0
                    : nBucket < 30 ? undoableCount
                                   : randomInteger(rng, 1,
                                                   qMin(undoableCount,
                                                        maximumUndoBatchSize));
                const auto mBucket = randomInteger(rng, 0, 99);
                const auto redoCount = undoCount == 0 || mBucket < 20 ? 0
                    : mBucket < 40 ? undoCount
                                   : randomInteger(rng, 0, undoCount);

                for (int i = 0; i < undoCount; ++i) {
                    error = invokeLogAndCompare(
                        logFile,
                        historyOperation(QStringLiteral("undo"), undoCount, redoCount, i + 1),
                        targets,
                        [](TestObject &target) { target.undo(); });
                    if (!error.isEmpty()) {
                        break;
                    }
                }
                for (int i = 0; error.isEmpty() && i < redoCount; ++i) {
                    error = invokeLogAndCompare(
                        logFile,
                        historyOperation(QStringLiteral("redo"), undoCount, redoCount, i + 1),
                        targets,
                        [](TestObject &target) { target.redo(); });
                }
            }

            if (!error.isEmpty()) {
                qInfo().noquote() << QStringLiteral("Step %1").arg(fuzzStep + 2);
                QFAIL(qPrintable(error));
            }
        }
    }
};

QTEST_GUILESS_MAIN(FreeValueEditingFuzzTest)

#include "tst_free_value_editing.moc"
