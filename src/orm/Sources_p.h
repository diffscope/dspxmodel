#ifndef DSPXMODEL_SOURCES_P_H
#define DSPXMODEL_SOURCES_P_H

#include <dspxmodelORM/Sources.h>

#include <dspxmodelORM/private/ORMUtils_p.h>

namespace dspx {

    class SourcesPrivate {
        Q_DECLARE_PUBLIC(Sources)
    public:
        explicit SourcesPrivate(Sources *q);

        DSPXMODEL_DECLARE_GET(Sources)
        DSPXMODEL_FORWARD_CONSTRUCTOR(Sources)

        Sources *q_ptr = nullptr;
        QString category;
        SingerList *singers = nullptr;
        DynamicMixingAnchorSequence *dynamicMixingAnchors = nullptr;
        SingingClip *singingClip = nullptr;
    };

}

#endif // DSPXMODEL_SOURCES_P_H
