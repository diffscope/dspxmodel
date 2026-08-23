#ifndef DSPXMODEL_AUDIOCLIP_P_H
#define DSPXMODEL_AUDIOCLIP_P_H

#include <dspxmodelORM/AudioClip.h>

#include <dspxmodelORM/private/ORMUtils_p.h>

namespace dspx {

    class AudioClipPrivate {
        Q_DECLARE_PUBLIC(AudioClip)
    public:
        explicit AudioClipPrivate(AudioClip *q);

        DSPXMODEL_DECLARE_GET(AudioClip)
        DSPXMODEL_FORWARD_CONSTRUCTOR(AudioClip)

        AudioClip *q_ptr = nullptr;
        AudioPathInfo path;
    };

}

#endif // DSPXMODEL_AUDIOCLIP_P_H
