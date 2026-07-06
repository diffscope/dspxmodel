#ifndef DSPXMODEL_AUDIOCLIP_H
#define DSPXMODEL_AUDIOCLIP_H

#include <QString>
#include <QVariant>

#include <dspxmodelORM/Clip.h>

namespace opendspx {
    struct AudioClip;
}

namespace dspx {

    /**
     * @brief Audio path information.
     */
    struct AudioPathInfo {
        Q_GADGET
        Q_PROPERTY(QString absoluteDir MEMBER absoluteDir)
        Q_PROPERTY(QString relativeDir MEMBER relativeDir)
        Q_PROPERTY(QString fileName MEMBER fileName)
        Q_PROPERTY(QString formatEntryClassName MEMBER formatEntryClassName)
        Q_PROPERTY(QVariant userData MEMBER userData)
        Q_PROPERTY(QString sha512 MEMBER sha512)
    public:
        QString absoluteDir;
        QString relativeDir;
        QString fileName;
        QString formatEntryClassName;
        QVariant userData;
        QString sha512;

        bool operator==(const AudioPathInfo &other) const = default;
        bool operator!=(const AudioPathInfo &other) const = default;
    };

    class AudioClipPrivate;

    /**
     * @brief Audio clip.
     */
    class DSPXMODEL_ORM_EXPORT AudioClip : public Clip {
        Q_OBJECT
        Q_DECLARE_PRIVATE(AudioClip)
        Q_PROPERTY(AudioPathInfo path READ path WRITE setPath NOTIFY pathChanged)
    public:
        /**
         * @brief Gets path.
         */
        AudioPathInfo path() const;
        /**
         * @brief Sets path.
         * @post path() == path.
         */
        void setPath(const AudioPathInfo &path);

        /**
         * @brief Converts to OpenDSPX audio clip.
         */
        opendspx::AudioClip toOpenDSPX() const;
        /**
         * @brief Converts from OpenDSPX audio clip.
         * @note Typically, this method SHOULD only be called on a newly created object.
         * @pre model()->document()->transaction() != nullptr && model()->document()->transaction()->state() == dini::TransactionState::Active.
         * @pre clip must be valid.
         */
        void fromOpenDSPX(const opendspx::AudioClip &clip);

    signals:
        void pathChanged(const AudioPathInfo &path);

    private:
        ~AudioClip() override;

        explicit AudioClip(Handle handle, Model *model);

        QScopedPointer<AudioClipPrivate> d_ptr;
    };

}

Q_DECLARE_METATYPE(dspx::AudioPathInfo)
Q_DECLARE_TYPEINFO(dspx::AudioPathInfo, Q_RELOCATABLE_TYPE);

#endif // DSPXMODEL_AUDIOCLIP_H
