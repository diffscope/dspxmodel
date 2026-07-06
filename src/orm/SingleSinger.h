#ifndef DSPXMODEL_SINGLESINGER_H
#define DSPXMODEL_SINGLESINGER_H

#include <QString>

#include <dspxmodelORM/Singer.h>

namespace opendspx {
    struct SingleSinger;
}

namespace dspx {

    class SingleSingerPrivate;

    /**
     * @brief Single singer.
     */
    class DSPXMODEL_ORM_EXPORT SingleSinger : public Singer {
        Q_OBJECT
        Q_DECLARE_PRIVATE(SingleSinger)
        Q_PROPERTY(QString id READ id WRITE setId NOTIFY idChanged)
    public:
        /**
         * @brief Gets id.
         */
        QString id() const;
        /**
         * @brief Sets id.
         * @post id() == id.
         */
        void setId(const QString &id);

        /**
         * @brief Converts to OpenDSPX single singer.
         */
        opendspx::SingleSinger toOpenDSPX() const;
        /**
         * @brief Converts from OpenDSPX single singer.
         * @note Typically, this method SHOULD only be called on a newly created object.
         * @pre model()->document()->transaction() != nullptr && model()->document()->transaction()->state() == dini::TransactionState::Active.
         * @pre singer must be valid.
         */
        void fromOpenDSPX(const opendspx::SingleSinger &singer);

    signals:
        void idChanged(const QString &id);

    private:
        ~SingleSinger() override;

        explicit SingleSinger(Handle handle, Model *model);

        QScopedPointer<SingleSingerPrivate> d_ptr;
    };

}

#endif // DSPXMODEL_SINGLESINGER_H
