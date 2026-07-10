#include "Document.h"

#include "Document_p.h"
#include "Schema.h"
#include <dini/change.h>
#include <dini/errors.h>
#include <dini/schema.h>
#include <dini/transaction.h>

#include <QByteArray>
#include <QDebug>
#include <QFileDevice>
#include <QIODevice>

#include <cstring>
#include <limits>
#include <memory>
#include <optional>
#include <string>

namespace dspx {

    namespace {

        constexpr char snapshotMagic[] = {'D', 'S', 'P', 'X', 'S', 'N', 'A', 'P'};
        constexpr char commitLogMagic[] = {'D', 'S', 'P', 'X', 'L', 'O', 'G', '1'};
        constexpr qsizetype magicSize = 8;
        constexpr quint32 wrapperFormatVersion = 1;

        const dini::EngineSchema &documentSchema() {
            static const auto schema = Schema::schemaBuilder()->freeze();
            return schema;
        }

        dini::ByteArray bytesFromQByteArray(const QByteArray &bytes) {
            if (bytes.isEmpty()) {
                return {};
            }
            const auto *data = reinterpret_cast<const std::uint8_t *>(bytes.constData());
            return dini::ByteArray(data, data + bytes.size());
        }

        QByteArray qByteArrayFromBytes(const dini::ByteArray &bytes) {
            if (bytes.empty()) {
                return {};
            }
            if (bytes.size() > static_cast<std::size_t>(std::numeric_limits<qsizetype>::max())) {
                throw dini::LogError("document persistence payload is too large");
            }
            return QByteArray(reinterpret_cast<const char *>(bytes.data()), static_cast<qsizetype>(bytes.size()));
        }

        void appendUInt32(QByteArray &bytes, quint32 value) {
            for (int i = 0; i < 4; ++i) {
                bytes.append(static_cast<char>((value >> (i * 8)) & 0xff));
            }
        }

        void appendUInt64(QByteArray &bytes, quint64 value) {
            for (int i = 0; i < 8; ++i) {
                bytes.append(static_cast<char>((value >> (i * 8)) & 0xff));
            }
        }

        quint32 readUInt32(const QByteArray &bytes) {
            quint32 value = 0;
            for (int i = 0; i < 4; ++i) {
                value |= static_cast<quint32>(static_cast<unsigned char>(bytes.at(i))) << (i * 8);
            }
            return value;
        }

        quint64 readUInt64(const QByteArray &bytes) {
            quint64 value = 0;
            for (int i = 0; i < 8; ++i) {
                value |= static_cast<quint64>(static_cast<unsigned char>(bytes.at(i))) << (i * 8);
            }
            return value;
        }

        bool writeAll(QIODevice *device, const QByteArray &bytes) {
            qsizetype offset = 0;
            while (offset < bytes.size()) {
                const auto written = device->write(bytes.constData() + offset, bytes.size() - offset);
                if (written <= 0) {
                    return false;
                }
                offset += written;
            }
            return true;
        }

        QByteArray readExactly(QIODevice *device, quint64 size, const char *message) {
            if (size > static_cast<quint64>(std::numeric_limits<qsizetype>::max())) {
                throw dini::RecoveryError(message);
            }
            QByteArray result;
            result.reserve(static_cast<qsizetype>(size));
            while (static_cast<quint64>(result.size()) < size) {
                const auto remaining = size - static_cast<quint64>(result.size());
                const auto chunk = device->read(static_cast<qint64>(remaining));
                if (chunk.isEmpty()) {
                    throw dini::RecoveryError(message);
                }
                result.append(chunk);
            }
            return result;
        }

        void ensureWritableDevice(QIODevice *device, const char *message) {
            if (!device || !device->isOpen() || !device->isWritable()) {
                throw dini::LogError(message);
            }
        }

        void ensureReadableDevice(QIODevice *device, const char *message) {
            if (!device || !device->isOpen() || !device->isReadable()) {
                throw dini::RecoveryError(message);
            }
        }

        QByteArray persistenceHeader(const char *magic, bool includeLength, quint64 length = 0) {
            QByteArray header;
            header.reserve(magicSize + 4 + (includeLength ? 8 : 0));
            header.append(magic, magicSize);
            appendUInt32(header, wrapperFormatVersion);
            if (includeLength) {
                appendUInt64(header, length);
            }
            return header;
        }

        void verifyHeaderMagic(const QByteArray &magic, const char *expected, const char *message) {
            if (magic.size() != magicSize || std::memcmp(magic.constData(), expected, magicSize) != 0) {
                throw dini::RecoveryError(message);
            }
        }

        void verifyHeaderVersion(const QByteArray &versionBytes, const char *message) {
            if (readUInt32(versionBytes) != wrapperFormatVersion) {
                throw dini::RecoveryError(message);
            }
        }

        void writeSnapshotPayload(QIODevice *device, const QByteArray &payload) {
            ensureWritableDevice(device, "snapshot device is not writable");
            const auto header = persistenceHeader(snapshotMagic, true, static_cast<quint64>(payload.size()));
            if (!writeAll(device, header) || !writeAll(device, payload)) {
                throw dini::LogError("failed to write document snapshot");
            }
        }

        QByteArray readSnapshotPayload(QIODevice *device) {
            ensureReadableDevice(device, "snapshot device is not readable");
            verifyHeaderMagic(readExactly(device, magicSize, "incomplete document snapshot header"),
                              snapshotMagic,
                              "invalid document snapshot header");
            verifyHeaderVersion(readExactly(device, 4, "incomplete document snapshot version"),
                                "unsupported document snapshot wrapper version");
            const auto payloadSize = readUInt64(readExactly(device, 8, "incomplete document snapshot length"));
            return readExactly(device, payloadSize, "incomplete document snapshot payload");
        }

        void writeCommitLogHeader(QIODevice *device) {
            ensureWritableDevice(device, "commit log device is not writable");
            if (!writeAll(device, persistenceHeader(commitLogMagic, false))) {
                throw dini::LogError("failed to write document commit log header");
            }
        }

        void readCommitLogHeader(QIODevice *device) {
            ensureReadableDevice(device, "commit log device is not readable");
            verifyHeaderMagic(readExactly(device, magicSize, "incomplete document commit log header"),
                              commitLogMagic,
                              "invalid document commit log header");
            verifyHeaderVersion(readExactly(device, 4, "incomplete document commit log version"),
                                "unsupported document commit log wrapper version");
        }

        std::optional<quint64> readCommitLogRecordSize(QIODevice *device) {
            const auto first = device->read(1);
            if (first.isEmpty()) {
                if (device->atEnd()) {
                    return std::nullopt;
                }
                throw dini::RecoveryError("failed to read document commit log record length");
            }
            const auto rest = readExactly(device, 7, "incomplete document commit log record length");
            return readUInt64(first + rest);
        }

        void replayCommitLog(dini::DocumentEngine *engine, QIODevice *device) {
            if (!device) {
                return;
            }
            readCommitLogHeader(device);
            while (const auto recordSize = readCommitLogRecordSize(device)) {
                const auto payload = readExactly(device, *recordSize, "incomplete document commit log record");
                try {
                    engine->replayChangeSet(dini::ChangeSet::deserialize(bytesFromQByteArray(payload)));
                } catch (const dini::DiniError &error) {
                    throw dini::RecoveryError(std::string("invalid document commit log record: ") + error.what());
                }
            }
        }

        bool flushDeviceIfPossible(QIODevice *device) {
            if (auto *fileDevice = dynamic_cast<QFileDevice *>(device)) {
                return fileDevice->flush();
            }
            return true;
        }

    }

    DocumentPrivate::DocumentPrivate(InitializationMode mode)
        : engine(std::make_unique<dini::DocumentEngine>(documentSchema())) {
        if (mode == InitializationMode::EmptyForRestore) {
            return;
        }
        auto transaction = engine->beginTransaction(dini::TransactionOptions {.undoable = false});
        transaction.insert(Schema::modelTable(), {});
        transaction.commit();
    }

    DocumentPrivate::~DocumentPrivate() {
        commitLogSubscription.disconnect();
    }

    void DocumentPrivate::writeCommitLogEvent(const dini::EngineEvent &event) {
        if (event.kind != dini::EventKind::AfterCommit) {
            return;
        }
        auto *device = commitLogDevice.data();
        if (!device) {
            qCritical() << "Failed to write document commit log: device was destroyed";
            return;
        }
        if (!device->isOpen() || !device->isWritable()) {
            qCritical() << "Failed to write document commit log: device is not writable";
            return;
        }

        try {
            const auto payload = qByteArrayFromBytes(event.changeSet.serialize());
            QByteArray header;
            header.reserve(8);
            appendUInt64(header, static_cast<quint64>(payload.size()));
            if (!writeAll(device, header) || !writeAll(device, payload)) {
                qCritical() << "Failed to write document commit log:" << device->errorString();
                return;
            }
            if (!flushDeviceIfPossible(device)) {
                qCritical() << "Failed to flush document commit log:" << device->errorString();
            }
        } catch (const std::exception &error) {
            qCritical() << "Failed to write document commit log:" << error.what();
        } catch (...) {
            qCritical() << "Failed to write document commit log: unknown error";
        }
    }

    Document::Document(DocumentPrivate *d, QObject *parent)
        : QObject(parent), d_ptr(d) {
    }

    Document::Document(QObject *parent)
        : Document(new DocumentPrivate, parent) {
    }

    Document::~Document() = default;

    dini::DocumentEngine *Document::engine() const {
        Q_D(const Document);
        return d->engine.get();
    }

    void Document::writeSnapshot(QIODevice *device) const {
        Q_D(const Document);
        writeSnapshotPayload(device, qByteArrayFromBytes(d->engine->createSnapshot()));
    }

    void Document::setCommitLogDevice(QIODevice *device) {
        Q_D(Document);
        d->commitLogSubscription.disconnect();
        d->commitLogDevice.clear();
        if (!device) {
            return;
        }
        writeCommitLogHeader(device);
        d->commitLogDevice = device;
        d->commitLogSubscription = d->engine->subscribe([d](const dini::EngineEvent &event) {
            d->writeCommitLogEvent(event);
        });
    }

    QIODevice *Document::commitLogDevice() const {
        Q_D(const Document);
        return d->commitLogDevice.data();
    }

    Document *Document::restore(QIODevice *snapshotDevice, QIODevice *commitLogDevice, QObject *parent) {
        auto d = std::make_unique<DocumentPrivate>(DocumentPrivate::InitializationMode::EmptyForRestore);
        d->engine->restoreSnapshot(bytesFromQByteArray(readSnapshotPayload(snapshotDevice)));
        replayCommitLog(d->engine.get(), commitLogDevice);

        auto document = std::unique_ptr<Document>(new Document(d.release(), parent));
        return document.release();
    }

    dini::Transaction *Document::transaction() const {
        Q_D(const Document);
        return d->transaction;
    }

    void Document::setTransaction(dini::Transaction *transaction) {
        Q_D(Document);
        d->transaction = transaction;
    }

}
