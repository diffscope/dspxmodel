#include "EntityObject.h"
#include "EntityObject_p.h"

#include <dspxmodelORM/Model.h>

namespace dspx {

    EntityObjectPrivate::EntityObjectPrivate(EntityObject *q, Handle handle, Model *model) : q_ptr(q), handle(handle), model(model) {
    }

    EntityObject::EntityObject(Handle handle, Model *model, QObject *parent)
        : QObject(parent), d_ptr(new EntityObjectPrivate(this, handle, model)) {
    }

    EntityObject::~EntityObject() = default;

    Handle EntityObject::handle() const {
        Q_D(const EntityObject);
        return d->handle;
    }

    Model *EntityObject::model() const {
        Q_D(const EntityObject);
        return d->model;
    }

}


#include "moc_EntityObject.cpp"
