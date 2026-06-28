#ifndef DSPXMODEL_SINGLESINGER_H
#define DSPXMODEL_SINGLESINGER_H

#include <QString>

#include <dspxmodelORM/Singer.h>

namespace dspx {

    class SingleSingerPrivate;

    /**
     * @brief Single singer.
     */
    class DSPXMODEL_ORM_EXPORT SingleSinger : public Singer {
        Q_OBJECT
        Q_PROPERTY(QString id READ id WRITE setId NOTIFY idChanged)
    public:
        ~SingleSinger() override;

        /**
         * @brief Gets id.
         */
        QString id() const;
        /**
         * @brief Sets id.
         * @post id() == id.
         */
        void setId(const QString &id);

    signals:
        void idChanged(const QString &id);

    private:
        explicit SingleSinger(Handle handle, Model *model);

        QScopedPointer<SingleSingerPrivate> d_ptr;
    };

}

#endif // DSPXMODEL_SINGLESINGER_H
