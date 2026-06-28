#ifndef DSPXMODEL_SINGER_H
#define DSPXMODEL_SINGER_H

#include <QJsonValue>

#include <dspxmodelORM/EntityObject.h>

namespace dspx {

    class SingerList;

    class SingerPrivate;

    /**
     * @brief Singer.
     */
    class DSPXMODEL_ORM_EXPORT Singer : public EntityObject {
        Q_OBJECT
        Q_PROPERTY(SingerType type READ type CONSTANT)
        Q_PROPERTY(QJsonValue extra READ extra WRITE setExtra NOTIFY extraChanged)
        Q_PROPERTY(SingerList *singerList READ singerList NOTIFY singerListChanged)
    public:
        /**
         * @brief Singer type.
         */
        enum SingerType {
            Single,
            Mixed,
        };
        Q_ENUM(SingerType)

        ~Singer() override;

        /**
         * @brief Gets type.
         */
        SingerType type() const;

        /**
         * @brief Gets extra.
         */
        QJsonValue extra() const;
        /**
         * @brief Sets extra.
         * @post extra() == extra.
         */
        void setExtra(const QJsonValue &extra);

        /**
         * @brief Gets singer list.
         */
        SingerList *singerList() const;

    signals:
        void extraChanged(const QJsonValue &extra);
        void singerListChanged(SingerList *singerList);

    protected:
        explicit Singer(Handle handle, Model *model);

        QScopedPointer<SingerPrivate> d_ptr;
    };

}

#endif // DSPXMODEL_SINGER_H
