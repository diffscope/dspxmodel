#ifndef DSPXMODEL_SINGINGCLIP_H
#define DSPXMODEL_SINGINGCLIP_H

#include <dspxmodelORM/Clip.h>

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
        Q_PROPERTY(Sources *sources READ sources WRITE setSources NOTIFY sourcesChanged)
        Q_PROPERTY(NoteSequence *notes READ notes CONSTANT)
        Q_PROPERTY(ParameterMap *parameters READ parameters CONSTANT)
    public:
        ~SingingClip() override;

        /**
         * @brief Gets sources.
         */
        Sources *sources() const;
        /**
         * @brief Sets sources.
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

    signals:
        void sourcesChanged(Sources *sources);

    private:
        explicit SingingClip(Handle handle, Model *model);

        QScopedPointer<SingingClipPrivate> d_ptr;
    };

}

#endif // DSPXMODEL_SINGINGCLIP_H
