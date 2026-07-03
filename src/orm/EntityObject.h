#ifndef DSPXMODEL_ENTITYOBJECT_H
#define DSPXMODEL_ENTITYOBJECT_H

#include <QObject>
#include <QScopedPointer>

#include <dspxmodelORM/Handle.h>

namespace dspx {

    class Model;

    class EntityObjectPrivate;

    class DSPXMODEL_ORM_EXPORT EntityObject : public QObject {
        Q_OBJECT
        Q_DECLARE_PRIVATE(EntityObject)
        Q_PROPERTY(Model *model READ model CONSTANT)
    public:
        Handle handle() const;

        Model *model() const;

    protected:
        explicit EntityObject(Handle handle, Model *model, QObject *parent = nullptr);
        ~EntityObject() override;

    private:
        QScopedPointer<EntityObjectPrivate> d_ptr;
    };

}

#endif // DSPXMODEL_ENTITYOBJECT_H
