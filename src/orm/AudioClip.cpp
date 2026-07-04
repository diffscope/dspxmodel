#include "AudioClip.h"
#include "AudioClip_p.h"

#include <cstdint>

#include <QByteArray>
#include <QDataStream>
#include <QDir>
#include <QFileInfo>
#include <QIODevice>

#include <dini/transaction.h>
#include <opendspx/audioclip.h>

#include <dspxmodelCore/Schema.h>
#include <dspxmodelORM/OpenDSPXConversion.h>
#include <dspxmodelORM/private/Clip_p.h>
#include <dspxmodelORM/private/ConversionUtils_p.h>
#include <dspxmodelORM/private/Model_p.h>
#include <dspxmodelORM/private/ORMBinding_p.h>
#include <dspxmodelORM/private/ORMUtils_p.h>

namespace dspx {

    namespace {

        QByteArray serializeVariant(const QVariant &value) {
            QByteArray bytes;
            QDataStream stream(&bytes, QIODevice::WriteOnly);
            stream.setVersion(QDataStream::Qt_5_15);
            stream << value;
            return bytes;
        }

        QVariant deserializeVariant(const QByteArray &bytes) {
            QVariant value;
            QDataStream stream(bytes);
            stream.setVersion(QDataStream::Qt_5_15);
            stream >> value;
            return value;
        }

        QString encodeUserData(const QVariant &value) {
            return QString::fromLatin1(serializeVariant(value).toBase64());
        }

        QVariant decodeUserData(const nlohmann::json &value) {
            if (!value.is_string()) {
                return {};
            }
            const auto base64 = QByteArray::fromStdString(value.get<std::string>());
            return deserializeVariant(QByteArray::fromBase64(base64));
        }

        std::vector<std::uint8_t> bytesFromQByteArray(const QByteArray &bytes) {
            std::vector<std::uint8_t> result;
            result.reserve(static_cast<std::size_t>(bytes.size()));
            for (auto byte : bytes) {
                result.push_back(static_cast<std::uint8_t>(byte));
            }
            return result;
        }

        QByteArray qByteArrayFromBytes(const std::vector<std::uint8_t> &bytes) {
            QByteArray result;
            result.resize(static_cast<int>(bytes.size()));
            for (std::size_t i = 0; i < bytes.size(); ++i) {
                result[static_cast<int>(i)] = static_cast<char>(bytes[i]);
            }
            return result;
        }

        const std::vector<orm::ColumnBinding<AudioClip>> &audioClipColumnBindings() {
            static const std::vector<orm::ColumnBinding<AudioClip>> bindings {
                {Schema::audioClipPathColumn(), [](AudioClip *q, const dini::Value &value) {
                     auto *d = AudioClipPrivate::get(q);
                     const auto newValue = orm::audioPathInfoFromValue(value);
                     const bool changed = d->path != newValue;
                     d->path = newValue;
                     return changed;
                 }, [](AudioClip *q) {
                     emit q->pathChanged(AudioClipPrivate::get(q)->path);
                 }},
            };
            return bindings;
        }

    }

    namespace orm {

        dini::Value valueFromAudioPathInfo(const AudioPathInfo &path) {
            QByteArray bytes;
            QDataStream stream(&bytes, QIODevice::WriteOnly);
            stream.setVersion(QDataStream::Qt_5_15);
            stream << path.absoluteDir
                   << path.relativeDir
                   << path.fileName
                   << path.formatEntryClassName
                   << path.userData
                   << path.sha512;
            return dini::Value(bytesFromQByteArray(bytes));
        }

        AudioPathInfo audioPathInfoFromValue(const dini::Value &value) {
            AudioPathInfo path;
            if (value.isNull()) {
                return path;
            }
            QDataStream stream(qByteArrayFromBytes(value.asBinary()));
            stream.setVersion(QDataStream::Qt_5_15);
            stream >> path.absoluteDir
                   >> path.relativeDir
                   >> path.fileName
                   >> path.formatEntryClassName
                   >> path.userData
                   >> path.sha512;
            return path;
        }

        void syncAudioClipColumns(AudioClip *item, const dini::ItemSnapshot &snapshot, bool notify) {
            syncColumnBindings(audioClipColumnBindings(), item, snapshot, notify);
        }

        bool applyAudioClipColumn(AudioClip *item, const dini::ColumnHandle &column, const dini::Value &value, bool notify) {
            return applyColumnBinding(audioClipColumnBindings(), item, column, value, notify);
        }

    }

    AudioClipPrivate::AudioClipPrivate(AudioClip *q) : q_ptr(q) {
    }

    AudioClip::AudioClip(Handle handle, Model *model) : Clip(handle, model), d_ptr(new AudioClipPrivate(this)) {
        ClipPrivate::get(static_cast<Clip *>(this))->type = Clip::Audio;
    }

    AudioClip::~AudioClip() = default;

    AudioPathInfo AudioClip::path() const {
        Q_D(const AudioClip);
        return d->path;
    }

    void AudioClip::setPath(const AudioPathInfo &path) {
        ModelPrivate::get(model())->update(handle(), Schema::audioClipPathColumn(), orm::valueFromAudioPathInfo(path));
    }

    opendspx::AudioClip AudioClip::toOpenDSPX() const {
        opendspx::AudioClip target;
        toOpenDSPXBase(target);

        const auto audioPathInfo = path();
        target.path = QDir(audioPathInfo.absoluteDir).filePath(audioPathInfo.fileName).toStdString();
        target.workspace["diffscope"]["audio"] = nlohmann::json::object({
            {"relativeDir", audioPathInfo.relativeDir.toStdString()},
            {"formatEntryClassName", audioPathInfo.formatEntryClassName.toStdString()},
            {"userData", encodeUserData(audioPathInfo.userData).toStdString()},
            {"sha512", audioPathInfo.sha512.toStdString()},
        });
        OpenDSPXConversion::convertClipToOpenDSPX(this, target);
        return target;
    }

    void AudioClip::fromOpenDSPX(const opendspx::AudioClip &clip) {
        fromOpenDSPXBase(clip);

        AudioPathInfo audioPathInfo;
        const QFileInfo fileInfo(QString::fromStdString(clip.path));
        audioPathInfo.absoluteDir = fileInfo.absolutePath();
        audioPathInfo.fileName = fileInfo.fileName();
        if (auto it = clip.workspace.find("diffscope"); it != clip.workspace.end()) {
            const auto &workspace = it->second;
            if (auto v = conv::optionalChain(workspace, "audio", "relativeDir"); v.is_string()) {
                audioPathInfo.relativeDir = QString::fromStdString(v.get<std::string>());
            }
            if (auto v = conv::optionalChain(workspace, "audio", "formatEntryClassName"); v.is_string()) {
                audioPathInfo.formatEntryClassName = QString::fromStdString(v.get<std::string>());
            }
            if (auto v = conv::optionalChain(workspace, "audio", "userData"); v.is_string()) {
                audioPathInfo.userData = decodeUserData(v);
            }
            if (auto v = conv::optionalChain(workspace, "audio", "sha512"); v.is_string()) {
                audioPathInfo.sha512 = QString::fromStdString(v.get<std::string>());
            }
        }
        setPath(audioPathInfo);
        OpenDSPXConversion::convertClipFromOpenDSPX(this, clip);
    }

}
