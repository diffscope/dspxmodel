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

    namespace orm {

        void syncAudioClipColumns(AudioClip *item, const dini::ItemSnapshot &snapshot, bool notify);
        bool applyAudioClipColumn(AudioClip *item, const dini::ColumnHandle &column, const dini::Value &value, bool notify);

        dini::Value valueFromAudioPathInfo(const AudioPathInfo &path);
        AudioPathInfo audioPathInfoFromValue(const dini::Value &value);

    }

}

#endif // DSPXMODEL_AUDIOCLIP_P_H
