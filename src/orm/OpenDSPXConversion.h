#ifndef DSPXMODEL_OpenDSPXCONVERSION_H
#define DSPXMODEL_OpenDSPXCONVERSION_H

#include <dspxmodelORM/DSPXModelORMGlobal.h>

namespace dspx {
    class Clip;
    class Model;
    class Note;
    class Track;
}

namespace opendspx {
    struct Clip;
    struct Model;
    struct Note;
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
     * @brief Converts custom clip data between dspx::Clip and opendspx::Clip.
     */
    class DSPXMODEL_ORM_EXPORT OpenDSPXClipConversionDelegate {
    public:
        virtual ~OpenDSPXClipConversionDelegate() = default;

        /**
         * @brief Imports data from an OpenDSPX clip into an ORM clip.
         *
         * @pre clip != nullptr.
         * @pre clip->model()->document()->transaction() != nullptr && clip->model()->document()->transaction()->state() == dini::TransactionState::Active.
         * @pre source must be valid.
         */
        virtual void fromOpenDSPX(Clip *clip, const opendspx::Clip &source) = 0;

        /**
         * @brief Exports data from an ORM clip into an OpenDSPX clip.
         *
         * @pre clip != nullptr.
         * @pre target must be valid.
         */
        virtual void toOpenDSPX(const Clip *clip, opendspx::Clip &target) = 0;
    };

    /**
     * @brief Converts custom note data between dspx::Note and opendspx::Note.
     */
    class DSPXMODEL_ORM_EXPORT OpenDSPXNoteConversionDelegate {
    public:
        virtual ~OpenDSPXNoteConversionDelegate() = default;

        /**
         * @brief Imports data from an OpenDSPX note into an ORM note.
         *
         * @pre note != nullptr.
         * @pre note->model()->document()->transaction() != nullptr && note->model()->document()->transaction()->state() == dini::TransactionState::Active.
         * @pre source must be valid.
         */
        virtual void fromOpenDSPX(Note *note, const opendspx::Note &source) = 0;

        /**
         * @brief Exports data from an ORM note into an OpenDSPX note.
         *
         * @pre note != nullptr.
         * @pre target must be valid.
         */
        virtual void toOpenDSPX(const Note *note, opendspx::Note &target) = 0;
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
         * @brief Registers a clip conversion delegate.
         *
         * Ownership of delegate transfers to the global registry.
         */
        static void addClipConversionDelegate(OpenDSPXClipConversionDelegate *delegate);

        /**
         * @brief Registers a note conversion delegate.
         *
         * Ownership of delegate transfers to the global registry.
         */
        static void addNoteConversionDelegate(OpenDSPXNoteConversionDelegate *delegate);

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

        /**
         * @brief Applies all registered clip delegates to import OpenDSPX data.
         *
         * @pre clip != nullptr.
         * @pre clip->model()->document()->transaction() != nullptr && clip->model()->document()->transaction()->state() == dini::TransactionState::Active.
         * @pre source must be valid.
         */
        static void convertClipFromOpenDSPX(Clip *clip, const opendspx::Clip &source);

        /**
         * @brief Applies all registered clip delegates to export OpenDSPX data.
         *
         * @pre clip != nullptr.
         * @pre target must be valid.
         */
        static void convertClipToOpenDSPX(const Clip *clip, opendspx::Clip &target);

        /**
         * @brief Applies all registered note delegates to import OpenDSPX data.
         *
         * @pre note != nullptr.
         * @pre note->model()->document()->transaction() != nullptr && note->model()->document()->transaction()->state() == dini::TransactionState::Active.
         * @pre source must be valid.
         */
        static void convertNoteFromOpenDSPX(Note *note, const opendspx::Note &source);

        /**
         * @brief Applies all registered note delegates to export OpenDSPX data.
         *
         * @pre note != nullptr.
         * @pre target must be valid.
         */
        static void convertNoteToOpenDSPX(const Note *note, opendspx::Note &target);

    };

}

#endif // DSPXMODEL_OpenDSPXCONVERSION_H
