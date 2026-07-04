#ifndef DSPXMODEL_OpenDSPXCONVERSION_H
#define DSPXMODEL_OpenDSPXCONVERSION_H

#include <dspxmodelORM/DSPXModelORMGlobal.h>

namespace dspx {
    class Model;
    class Track;
}

namespace opendspx {
    struct Model;
    struct Track;
}

namespace dspx {

    /**
     * @brief Converts between dspx::Model and opendspx::Model.
     */
    class DSPXMODEL_ORM_EXPORT OpenDSPXModelConversionDelegate {
    public:
        virtual ~OpenDSPXModelConversionDelegate() = default;

        /**
         * @brief Imports data from an OpenDSPX model into an ORM model.
         *
         * @pre model != nullptr.
         * @pre model->document()->transaction() != nullptr && model->document()->transaction()->state() == dini::TransactionState::Active.
         * @pre source must be validated.
         */
        virtual void fromOpenDSPX(Model *model, const opendspx::Model &source) = 0;

        /**
         * @brief Exports data from an ORM model into an OpenDSPX model.
         *
         * @pre model != nullptr.
         * @pre target must be valid.
         */
        virtual void toOpenDSPX(const Model *model, opendspx::Model &target) = 0;
    };

    /**
     * @brief Converts between dspx::Track and opendspx::Track.
     */
    class DSPXMODEL_ORM_EXPORT OpenDSPXTrackConversionDelegate {
    public:
        virtual ~OpenDSPXTrackConversionDelegate() = default;

        /**
         * @brief Imports data from an OpenDSPX track into an ORM track.
         *
         * @pre track != nullptr.
         * @pre track->model()->document()->transaction() != nullptr && track->model()->document()->transaction()->state() == dini::TransactionState::Active.
         * @pre source must be valid.
         */
        virtual void fromOpenDSPX(Track *track, const opendspx::Track &source) = 0;

        /**
         * @brief Exports data from an ORM track into an OpenDSPX track.
         *
         * @pre track != nullptr.
         * @pre target must be valid.
         */
        virtual void toOpenDSPX(const Track *track, opendspx::Track &target) = 0;
    };

    /**
     * @brief Global registry and dispatch point for custom OpenDSPX conversion delegates.
     */
    class DSPXMODEL_ORM_EXPORT OpenDSPXConversion {
    public:
        OpenDSPXConversion() = delete;

        /**
         * @brief Registers a model conversion delegate.
         *
         * Ownership of delegate transfers to the global registry.
         */
        static void addModelConversionDelegate(OpenDSPXModelConversionDelegate *delegate);

        /**
         * @brief Registers a track conversion delegate.
         *
         * Ownership of delegate transfers to the global registry.
         */
        static void addTrackConversionDelegate(OpenDSPXTrackConversionDelegate *delegate);

        /**
         * @brief Applies all registered model delegates to import OpenDSPX data.
         *
         * @pre model != nullptr.
         * @pre model->document()->transaction() != nullptr && model->document()->transaction()->state() == dini::TransactionState::Active.
         * @pre source must be valid.
         */
        static void convertModelFromOpenDSPX(Model *model, const opendspx::Model &source);

        /**
         * @brief Applies all registered model delegates to export OpenDSPX data.
         *
         * @pre model != nullptr.
         * @pre target must be valid.
         */
        static void convertModelToOpenDSPX(const Model *model, opendspx::Model &target);

        /**
         * @brief Applies all registered track delegates to import OpenDSPX data.
         *
         * @pre track != nullptr.
         * @pre track->model()->document()->transaction() != nullptr && track->model()->document()->transaction()->state() == dini::TransactionState::Active.
         * @pre source must be valid.
         */
        static void convertTrackFromOpenDSPX(Track *track, const opendspx::Track &source);

        /**
         * @brief Applies all registered track delegates to export OpenDSPX data.
         *
         * @pre track != nullptr.
         * @pre target must be valid.
         */
        static void convertTrackToOpenDSPX(const Track *track, opendspx::Track &target);

    };

}

#endif // DSPXMODEL_OpenDSPXCONVERSION_H
