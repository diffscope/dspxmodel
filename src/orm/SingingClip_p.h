#ifndef DSPXMODEL_SINGINGCLIP_P_H
#define DSPXMODEL_SINGINGCLIP_P_H

#include <dspxmodelORM/SingingClip.h>

#include <dspxmodelORM/private/ORMUtils_p.h>

namespace dspx {

    class SingingClipPrivate {
        Q_DECLARE_PUBLIC(SingingClip)
    public:
        explicit SingingClipPrivate(SingingClip *q);

        DSPXMODEL_DECLARE_GET(SingingClip)
        DSPXMODEL_FORWARD_CONSTRUCTOR(SingingClip)

        SingingClip *q_ptr = nullptr;
        Sources *sources = nullptr;
        bool sourcesResolved = false;
        NoteSequence *notes = nullptr;
        ParameterMap *parameters = nullptr;
    };

}

#endif // DSPXMODEL_SINGINGCLIP_P_H
