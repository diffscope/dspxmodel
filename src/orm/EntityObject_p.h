#ifndef DSPXMODEL_ENTITYOBJECT_P_H
#define DSPXMODEL_ENTITYOBJECT_P_H

#include <dspxmodelORM/EntityObject.h>

#include <dspxmodelORM/Handle.h>

namespace dspx {

    class EntityObject;
    class Model;

    class EntityObjectPrivate {
        Q_DECLARE_PUBLIC(EntityObject)
    public:
        EntityObjectPrivate(EntityObject *q, Handle handle, Model *model);

        EntityObject *q_ptr = nullptr;
        Handle handle;
        Model *model = nullptr;
    };

}

#endif // DSPXMODEL_ENTITYOBJECT_P_H
