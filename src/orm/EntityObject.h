#ifndef DSPXMODEL_ENTITYOBJECT_H
#define DSPXMODEL_ENTITYOBJECT_H

#include <QObject>

#include <dspxmodelORM/Handle.h>

namespace dspx {

    class Model;

    class EntityObjectPrivate;

    class DSPXMODEL_ORM_EXPORT EntityObject : public QObject {
        Q_OBJECT
        Q_DECLARE_PRIVATE(EntityObject)
        Q_PROPERTY(Model *model READ model CONSTANT)
    public:
        ~EntityObject() override;

        Handle handle() const;

        Model *model() const;

    private:
        explicit EntityObject(QObject *parent = nullptr);

        QScopedPointer<EntityObjectPrivate> d_ptr;
    };

}

#endif // DSPXMODEL_ENTITYOBJECT_H
