#include "OpenDSPXConversion.h"

#include <memory>
#include <vector>

#include <QGlobalStatic>
#include <QtGlobal>

namespace dspx {

    namespace {
        std::vector<std::unique_ptr<OpenDSPXModelConversionDelegate>> s_modelDelegates;
        std::vector<std::unique_ptr<OpenDSPXTrackConversionDelegate>> s_trackDelegates;
    }

    void OpenDSPXConversion::addModelConversionDelegate(OpenDSPXModelConversionDelegate *delegate) {
        s_modelDelegates.emplace_back(delegate);
    }

    void OpenDSPXConversion::addTrackConversionDelegate(OpenDSPXTrackConversionDelegate *delegate) {
        s_trackDelegates.emplace_back(delegate);
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

}
