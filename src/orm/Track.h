#ifndef DSPXMODEL_TRACK_H
#define DSPXMODEL_TRACK_H

#include <QScopedPointer>
#include <QString>

#include <dspxmodelORM/EntityObject.h>

namespace opendspx {
    struct Track;
}

namespace dspx {

    class ClipSequence;
    class TrackList;

    class TrackPrivate;

    /**
     * @brief Track.
     */
    class DSPXMODEL_ORM_EXPORT Track : public EntityObject {
        Q_OBJECT
        Q_DECLARE_PRIVATE(Track)
        Q_PROPERTY(int colorId READ colorId WRITE setColorId NOTIFY colorIdChanged)
        Q_PROPERTY(double height READ height WRITE setHeight NOTIFY heightChanged)
        Q_PROPERTY(QString name READ name WRITE setName NOTIFY nameChanged)
        Q_PROPERTY(double gain READ gain WRITE setGain NOTIFY gainChanged)
        Q_PROPERTY(double pan READ pan WRITE setPan NOTIFY panChanged)
        Q_PROPERTY(bool mute READ mute WRITE setMute NOTIFY muteChanged)
        Q_PROPERTY(bool solo READ solo WRITE setSolo NOTIFY soloChanged)
        Q_PROPERTY(bool record READ record WRITE setRecord NOTIFY recordChanged)
        Q_PROPERTY(ClipSequence *clips READ clips CONSTANT)
        Q_PROPERTY(TrackList *trackList READ trackList NOTIFY trackListChanged)
    public:
        /**
         * @brief Gets color id.
         */
        int colorId() const;
        /**
         * @brief Sets color id.
         * @pre model()->document()->transaction() != nullptr && model()->document()->transaction()->state() == dini::TransactionState::Active.
         * @post colorId() == colorId.
         */
        void setColorId(int colorId);

        /**
         * @brief Gets height.
         */
        double height() const;
        /**
         * @brief Sets height.
         * @pre model()->document()->transaction() != nullptr && model()->document()->transaction()->state() == dini::TransactionState::Active.
         * @post height() == height.
         */
        void setHeight(double height);

        /**
         * @brief Gets name.
         */
        QString name() const;
        /**
         * @brief Sets name.
         * @pre model()->document()->transaction() != nullptr && model()->document()->transaction()->state() == dini::TransactionState::Active.
         * @post name() == name.
         */
        void setName(const QString &name);

        /**
         * @brief Gets gain.
         * @post gain() >= 0.0.
         */
        double gain() const;
        /**
         * @brief Sets gain.
         * @pre model()->document()->transaction() != nullptr && model()->document()->transaction()->state() == dini::TransactionState::Active.
         * @pre gain >= 0.0.
         * @post gain() == gain.
         */
        void setGain(double gain);

        /**
         * @brief Gets pan.
         * @post pan() >= -1.0 && pan() <= 1.0.
         */
        double pan() const;
        /**
         * @brief Sets pan.
         * @pre model()->document()->transaction() != nullptr && model()->document()->transaction()->state() == dini::TransactionState::Active.
         * @pre pan >= -1.0 && pan <= 1.0.
         * @post pan() == pan.
         */
        void setPan(double pan);

        /**
         * @brief Gets mute.
         */
        bool mute() const;
        /**
         * @brief Sets mute.
         * @pre model()->document()->transaction() != nullptr && model()->document()->transaction()->state() == dini::TransactionState::Active.
         * @post mute() == mute.
         */
        void setMute(bool mute);

        /**
         * @brief Gets solo.
         */
        bool solo() const;
        /**
         * @brief Sets solo.
         * @pre model()->document()->transaction() != nullptr && model()->document()->transaction()->state() == dini::TransactionState::Active.
         * @post solo() == solo.
         */
        void setSolo(bool solo);

        /**
         * @brief Gets record.
         */
        bool record() const;
        /**
         * @brief Sets record.
         * @pre model()->document()->transaction() != nullptr && model()->document()->transaction()->state() == dini::TransactionState::Active.
         * @post record() == record.
         */
        void setRecord(bool record);

        /**
         * @brief Gets clips.
         * @post clips() != nullptr.
         */
        ClipSequence *clips() const;

        /**
         * @brief Gets track list.
         */
        TrackList *trackList() const;

        /**
         * @brief Converts to OpenDSPX track.
         */
        opendspx::Track toOpenDSPX() const;
        /**
         * @brief Converts from OpenDSPX track.
         * @pre model()->document()->transaction() != nullptr && model()->document()->transaction()->state() == dini::TransactionState::Active.
         * @pre track must be valid.
         */
        void fromOpenDSPX(const opendspx::Track &track);

    signals:
        void colorIdChanged(int colorId);
        void heightChanged(double height);
        void nameChanged(const QString &name);
        void gainChanged(double gain);
        void panChanged(double pan);
        void muteChanged(bool mute);
        void soloChanged(bool solo);
        void recordChanged(bool record);
        void trackListChanged(TrackList *trackList);

    private:
        ~Track() override;

        explicit Track(Handle handle, Model *model);

        QScopedPointer<TrackPrivate> d_ptr;
    };

}

#endif // DSPXMODEL_TRACK_H
