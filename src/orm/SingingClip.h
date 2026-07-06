#ifndef DSPXMODEL_SINGINGCLIP_H
#define DSPXMODEL_SINGINGCLIP_H

#include <dspxmodelORM/Clip.h>

namespace opendspx {
    struct SingingClip;
}

namespace dspx {

    class NoteSequence;
    class ParameterMap;
    class Sources;

    class SingingClipPrivate;

    /**
     * @brief Singing clip.
     */
    class DSPXMODEL_ORM_EXPORT SingingClip : public Clip {
        Q_OBJECT
        Q_DECLARE_PRIVATE(SingingClip)
        Q_PROPERTY(Sources *sources READ sources WRITE setSources NOTIFY sourcesChanged)
        Q_PROPERTY(NoteSequence *notes READ notes CONSTANT)
        Q_PROPERTY(ParameterMap *parameters READ parameters CONSTANT)
    public:
        /**
         * @brief Gets sources.
         */
        Sources *sources() const;
        /**
         * @brief Sets sources.
         *
         * If sources->singingClip() != nullptr, the old singing clip will be detached.
         *
         * @post sources() == sources.
         */
        void setSources(Sources *sources);

        /**
         * @brief Gets note sequence.
         * @post notes() != nullptr.
         */
        NoteSequence *notes() const;
        /**
         * @brief Gets parameter map.
         * @post parameters() != nullptr.
         */
        ParameterMap *parameters() const;

        /**
         * @brief Converts to OpenDSPX singing clip.
         */
        opendspx::SingingClip toOpenDSPX() const;
        /**
         * @brief Converts from OpenDSPX singing clip.
         * @pre model()->document()->transaction() != nullptr && model()->document()->transaction()->state() == dini::TransactionState::Active.
         * @pre clip must be valid.
         */
        void fromOpenDSPX(const opendspx::SingingClip &clip);

    signals:
        void sourcesChanged(Sources *sources);

    private:
        ~SingingClip() override;

        explicit SingingClip(Handle handle, Model *model);

        QScopedPointer<SingingClipPrivate> d_ptr;
    };

}

#endif // DSPXMODEL_SINGINGCLIP_H
