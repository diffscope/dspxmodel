#ifndef DSPXMODEL_CLIP_P_H
#define DSPXMODEL_CLIP_P_H

#include <dspxmodelORM/Clip.h>

#include <dspxmodelORM/private/ORMUtils_p.h>

namespace dspx {

    class ClipPrivate {
        Q_DECLARE_PUBLIC(Clip)
    public:
        explicit ClipPrivate(Clip *q);

        DSPXMODEL_DECLARE_GET(Clip)
        DSPXMODEL_FORWARD_CONSTRUCTOR(Clip)

        void setSequence(ClipSequence *sequence, bool notify);

        Clip *q_ptr = nullptr;
        QString name;
        double gain = 1.0;
        double pan = 0.0;
        bool mute = false;
        int position = 0;
        int length = 0;
        int clipStart = 0;
        int clipLength = 0;
        Clip::ClipType type = Clip::Audio;
        Handle previousHandle;
        Handle nextHandle;
        mutable Clip *previous = nullptr;
        mutable Clip *next = nullptr;
        int overlappedCount = 0;
        ClipSequence *sequence = nullptr;
    };

}

#endif // DSPXMODEL_CLIP_P_H
