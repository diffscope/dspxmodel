#ifndef DSPXMODEL_CLIP_H
#define DSPXMODEL_CLIP_H

#include <QString>

#include <dspxmodelORM/EntityObject.h>

namespace dspx {

    class ClipSequence;

    class ClipPrivate;

    /**
     * @brief Clip.
     */
    class DSPXMODEL_ORM_EXPORT Clip : public EntityObject {
        Q_OBJECT
        Q_PROPERTY(QString name READ name WRITE setName NOTIFY nameChanged)
        Q_PROPERTY(double pan READ pan WRITE setPan NOTIFY panChanged)
        Q_PROPERTY(bool mute READ mute WRITE setMute NOTIFY muteChanged)
        Q_PROPERTY(int position READ position WRITE setPosition NOTIFY positionChanged)
        Q_PROPERTY(int length READ length WRITE setLength NOTIFY lengthChanged)
        Q_PROPERTY(int clipStart READ clipStart WRITE setClipStart NOTIFY clipStartChanged)
        Q_PROPERTY(int clipLen READ clipLen WRITE setClipLen NOTIFY clipLenChanged)
        Q_PROPERTY(ClipType type READ type CONSTANT)
        Q_PROPERTY(int start READ start NOTIFY startChanged)
        Q_PROPERTY(Clip *previousItem READ previousItem NOTIFY previousItemChanged)
        Q_PROPERTY(Clip *nextItem READ nextItem NOTIFY nextItemChanged)
        Q_PROPERTY(bool overlapped READ overlapped NOTIFY overlappedChanged)
        Q_PROPERTY(ClipSequence *clipSequence READ clipSequence NOTIFY clipSequenceChanged)
    public:
        /**
         * @brief Clip type.
         */
        enum ClipType {
            Audio,
            Singing,
        };
        Q_ENUM(ClipType)

        ~Clip() override;

        /**
         * @brief Gets name.
         */
        QString name() const;
        /**
         * @brief Sets name.
         * @post name() == name.
         */
        void setName(const QString &name);

        /**
         * @brief Gets pan.
         * @post pan() >= -1 && pan() <= 1.
         */
        double pan() const;
        /**
         * @brief Sets pan.
         * @pre pan >= -1 && pan <= 1.
         * @post pan() == pan.
         */
        void setPan(double pan);

        /**
         * @brief Gets mute.
         */
        bool mute() const;
        /**
         * @brief Sets mute.
         * @post mute() == mute.
         */
        void setMute(bool mute);

        /**
         * @brief Gets position.
         *
         * This property is the position of the clip in the clip sequence.
         *
         * @post position() >= 0.
         */
        int position() const;
        /**
         * @brief Sets position.
         * @pre position >= 0.
         * @post position() == position.
         */
        void setPosition(int position);

        /**
         * @brief Gets length.
         * @post length() >= 0.
         */
        int length() const;
        /**
         * @brief Sets length.
         * @pre length >= 0.
         * @post length() == length.
         */
        void setLength(int length);

        /**
         * @brief Gets clip start.
         * @post clipStart() >= 0.
         */
        int clipStart() const;
        /**
         * @brief Sets clip start.
         * @pre clipStart >= 0.
         * @post clipStart() == clipStart.
         */
        void setClipStart(int clipStart);

        /**
         * @brief Gets clip length.
         * @post clipLen() >= 0.
         */
        int clipLen() const;
        /**
         * @brief Sets clip length.
         * @pre clipLen >= 0.
         * @post clipLen() == clipLen.
         */
        void setClipLen(int clipLen);

        /**
         * @brief Gets type.
         */
        ClipType type() const;

        /**
         * @brief Gets start.
         *
         * This property is computed as position() - clipStart().
         *
         * @post start() == position() - clipStart().
         */
        int start() const;

        /**
         * @brief Gets previous item.
         */
        Clip *previousItem() const;
        /**
         * @brief Gets next item.
         */
        Clip *nextItem() const;

        /**
         * @brief Gets whether this clip overlaps another clip in the sequence.
         */
        bool overlapped() const;

        /**
         * @brief Gets clip sequence.
         */
        ClipSequence *clipSequence() const;

    signals:
        void nameChanged(const QString &name);
        void panChanged(double pan);
        void muteChanged(bool mute);
        void positionChanged(int position);
        void lengthChanged(int length);
        void clipStartChanged(int clipStart);
        void clipLenChanged(int clipLen);
        void startChanged(int start);
        void previousItemChanged(Clip *previousItem);
        void nextItemChanged(Clip *nextItem);
        void overlappedChanged(bool overlapped);
        void clipSequenceChanged(ClipSequence *clipSequence);

    protected:
        explicit Clip(Handle handle, Model *model);

        QScopedPointer<ClipPrivate> d_ptr;
    };

}

#endif // DSPXMODEL_CLIP_H
