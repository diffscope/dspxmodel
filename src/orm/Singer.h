#ifndef DSPXMODEL_SINGER_H
#define DSPXMODEL_SINGER_H

#include <memory>

#include <QJsonValue>
#include <qqmlintegration.h>

#include <dspxmodelORM/EntityObject.h>

namespace opendspx {
    struct Singer;
    struct SingleSinger;
    struct MixedSinger;
}

namespace dspx {

    class SingerList;

    class SingerPrivate;

    /**
     * @brief Singer.
     */
    class DSPXMODEL_ORM_EXPORT Singer : public EntityObject {
        Q_OBJECT
        QML_ELEMENT
        QML_UNCREATABLE("")
        Q_DECLARE_PRIVATE(Singer)
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

        /**
         * @brief Converts to OpenDSPX singer.
         */
        std::shared_ptr<opendspx::Singer> toOpenDSPX() const;
        /**
         * @brief Converts from OpenDSPX singer.
         * @note Typically, this method SHOULD only be called on a newly created object.
         * @pre model()->document()->transaction() != nullptr && model()->document()->transaction()->state() == dini::TransactionState::Active.
         * @pre singer must be valid and have the same type as this object.
         */
        void fromOpenDSPX(const std::shared_ptr<opendspx::Singer> &singer);

    signals:
        void extraChanged(const QJsonValue &extra);
        void singerListChanged(SingerList *singerList);
        void extraChangedAfterCommit(const QJsonValue &extra);
        void singerListChangedAfterCommit(SingerList *singerList);

    protected:
        ~Singer() override;

        explicit Singer(Handle handle, Model *model);

        void fromOpenDSPXBase(const opendspx::Singer &singer);
        void toOpenDSPXBase(opendspx::Singer &singer) const;

        QScopedPointer<SingerPrivate> d_ptr;
    };

}

#endif // DSPXMODEL_SINGER_H
