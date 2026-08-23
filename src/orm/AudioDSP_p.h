#ifndef DSPXMODEL_AUDIODSP_P_H
#define DSPXMODEL_AUDIODSP_P_H

#include <dspxmodelORM/AudioDSP.h>

#include <dspxmodelORM/private/ORMUtils_p.h>

namespace dspx {

    class AudioDSPPrivate {
        Q_DECLARE_PUBLIC(AudioDSP)
    public:
        explicit AudioDSPPrivate(AudioDSP *q);

        DSPXMODEL_DECLARE_GET(AudioDSP)
        DSPXMODEL_FORWARD_CONSTRUCTOR(AudioDSP)

        void setAudioDSPList(AudioDSPList *list, bool notify);

        AudioDSP *q_ptr = nullptr;
        AudioDSPList *list = nullptr;
        QString id;
        QJsonValue data;
        bool enabled = false;
    };

}

#endif // DSPXMODEL_AUDIODSP_P_H
