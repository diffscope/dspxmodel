#include "OpenDSPXConversion.h"

#include <memory>
#include <vector>

#include <QGlobalStatic>
#include <QtGlobal>

namespace dspx {

    namespace {
        std::vector<std::unique_ptr<OpenDSPXModelConversionDelegate>> s_modelDelegates;
        std::vector<std::unique_ptr<OpenDSPXTrackConversionDelegate>> s_trackDelegates;
        std::vector<std::unique_ptr<OpenDSPXClipConversionDelegate>> s_clipDelegates;
        std::vector<std::unique_ptr<OpenDSPXNoteConversionDelegate>> s_noteDelegates;
        std::vector<std::unique_ptr<OpenDSPXSingerConversionDelegate>> s_singerDelegates;
    }

    void OpenDSPXConversion::addModelConversionDelegate(OpenDSPXModelConversionDelegate *delegate) {
        s_modelDelegates.emplace_back(delegate);
    }

    void OpenDSPXConversion::addTrackConversionDelegate(OpenDSPXTrackConversionDelegate *delegate) {
        s_trackDelegates.emplace_back(delegate);
    }

    void OpenDSPXConversion::addClipConversionDelegate(OpenDSPXClipConversionDelegate *delegate) {
        s_clipDelegates.emplace_back(delegate);
    }

    void OpenDSPXConversion::addNoteConversionDelegate(OpenDSPXNoteConversionDelegate *delegate) {
        s_noteDelegates.emplace_back(delegate);
    }

    void OpenDSPXConversion::addSingerConversionDelegate(OpenDSPXSingerConversionDelegate *delegate) {
        s_singerDelegates.emplace_back(delegate);
    }

    void OpenDSPXConversion::convertModelFromOpenDSPX(Model *model, const opendspx::Model &source) {
        Q_ASSERT(model);
        for (const auto &delegate : s_modelDelegates) {
            delegate->fromOpenDSPX(model, source);
        }
    }

    void OpenDSPXConversion::convertModelToOpenDSPX(const Model *model, opendspx::Model &target) {
        Q_ASSERT(model);
        for (const auto &delegate : s_modelDelegates) {
            delegate->toOpenDSPX(model, target);
        }
    }

    void OpenDSPXConversion::convertTrackFromOpenDSPX(Track *track, const opendspx::Track &source) {
        Q_ASSERT(track);
        for (const auto &delegate : s_trackDelegates) {
            delegate->fromOpenDSPX(track, source);
        }
    }

    void OpenDSPXConversion::convertTrackToOpenDSPX(const Track *track, opendspx::Track &target) {
        Q_ASSERT(track);
        for (const auto &delegate : s_trackDelegates) {
            delegate->toOpenDSPX(track, target);
        }
    }

    void OpenDSPXConversion::convertClipFromOpenDSPX(Clip *clip, const opendspx::Clip &source) {
        Q_ASSERT(clip);
        for (const auto &delegate : s_clipDelegates) {
            delegate->fromOpenDSPX(clip, source);
        }
    }

    void OpenDSPXConversion::convertClipToOpenDSPX(const Clip *clip, opendspx::Clip &target) {
        Q_ASSERT(clip);
        for (const auto &delegate : s_clipDelegates) {
            delegate->toOpenDSPX(clip, target);
        }
    }

    void OpenDSPXConversion::convertNoteFromOpenDSPX(Note *note, const opendspx::Note &source) {
        Q_ASSERT(note);
        for (const auto &delegate : s_noteDelegates) {
            delegate->fromOpenDSPX(note, source);
        }
    }

    void OpenDSPXConversion::convertNoteToOpenDSPX(const Note *note, opendspx::Note &target) {
        Q_ASSERT(note);
        for (const auto &delegate : s_noteDelegates) {
            delegate->toOpenDSPX(note, target);
        }
    }

    void OpenDSPXConversion::convertSingerFromOpenDSPX(Singer *singer, const opendspx::Singer &source) {
        Q_ASSERT(singer);
        for (const auto &delegate : s_singerDelegates) {
            delegate->fromOpenDSPX(singer, source);
        }
    }

    void OpenDSPXConversion::convertSingerToOpenDSPX(const Singer *singer, opendspx::Singer &target) {
        Q_ASSERT(singer);
        for (const auto &delegate : s_singerDelegates) {
            delegate->toOpenDSPX(singer, target);
        }
    }

}
