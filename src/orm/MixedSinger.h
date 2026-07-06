#ifndef DSPXMODEL_MIXEDSINGER_H
#define DSPXMODEL_MIXEDSINGER_H

#include <QList>

#include <dspxmodelORM/Singer.h>

namespace opendspx {
    struct MixedSinger;
}

namespace dspx {

    class SingerList;

    class MixedSingerPrivate;

    /**
     * @brief Mixed singer.
     */
    class DSPXMODEL_ORM_EXPORT MixedSinger : public Singer {
        Q_OBJECT
        Q_DECLARE_PRIVATE(MixedSinger)
        Q_PROPERTY(QList<double> ratio READ ratio WRITE setRatio NOTIFY ratioChanged)
        Q_PROPERTY(SingerList *singers READ singers CONSTANT)
    public:
        /**
         * @brief Gets ratio.
         *
         * The size of ratio() plus 1 should be equal to the size of singers()->singers().
         * However, this constraint is permitted to be temporarily violated during intermediate states.
         * In this case:
         *   - if the size is bigger, the extra items are ignored;
         *   - if the size is smaller, the missing items are filled with 0.0.
         *
         * @post Each item in ratio() is in the range [0.0, 1.0].
         * @post The sum of items in ratio() is less than or equal to 1.0.
         */
        QList<double> ratio() const;
        /**
         * @brief Sets ratio.
         * @pre Each item in ratio is in the range [0.0, 1.0].
         * @pre The sum of items in ratio is less than or equal to 1.0.
         * @post ratio() == ratio.
         */
        void setRatio(const QList<double> &ratio);

        /**
         * @brief Gets singer list.
         * @post singers() != nullptr.
         */
        SingerList *singers() const;

        /**
         * @brief Converts to OpenDSPX mixed singer.
         */
        opendspx::MixedSinger toOpenDSPX() const;
        /**
         * @brief Converts from OpenDSPX mixed singer.
         * @note Typically, this method SHOULD only be called on a newly created object.
         * @pre model()->document()->transaction() != nullptr && model()->document()->transaction()->state() == dini::TransactionState::Active.
         * @pre singer must be valid.
         */
        void fromOpenDSPX(const opendspx::MixedSinger &singer);

    signals:
        void ratioChanged(const QList<double> &ratio);

    private:
        ~MixedSinger() override;

        explicit MixedSinger(Handle handle, Model *model);

        QScopedPointer<MixedSingerPrivate> d_ptr;
    };

}

#endif // DSPXMODEL_MIXEDSINGER_H
