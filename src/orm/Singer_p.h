#ifndef DSPXMODEL_SINGER_P_H
#define DSPXMODEL_SINGER_P_H

#include <dspxmodelORM/Singer.h>

#include <dini/types.h>

#include <dspxmodelORM/private/ORMUtils_p.h>

namespace dspx {

    class SingerPrivate {
        Q_DECLARE_PUBLIC(Singer)
    public:
        explicit SingerPrivate(Singer *q);

        DSPXMODEL_DECLARE_GET(Singer)
        DSPXMODEL_FORWARD_CONSTRUCTOR(Singer)

        void setSingerList(SingerList *list, bool notify);
        dini::ByteArray workspace() const;
        void setWorkspace(dini::ByteArray workspace);

        Singer *q_ptr = nullptr;
        Singer::SingerType type = Singer::Single;
        QJsonValue extra;
        dini::ByteArray workspaceData;
        SingerList *singerList = nullptr;
    };

}

#endif // DSPXMODEL_SINGER_P_H
