#ifndef DSPXMODEL_AUDIOCLIP_H
#define DSPXMODEL_AUDIOCLIP_H

#include <QString>
#include <QVariant>

#include <dspxmodelORM/Clip.h>

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

        QString absoluteDir;
        QString relativeDir;
        QString fileName;
        QString formatEntryClassName;
        QVariant userData;
        QString sha512;
    };

    class AudioClipPrivate;

    /**
     * @brief Audio clip.
     */
    class DSPXMODEL_ORM_EXPORT AudioClip : public Clip {
        Q_OBJECT
        Q_PROPERTY(AudioPathInfo path READ path WRITE setPath NOTIFY pathChanged)
    public:
        ~AudioClip() override;

        /**
         * @brief Gets path.
         */
        AudioPathInfo path() const;
        /**
         * @brief Sets path.
         * @post path() == path.
         */
        void setPath(const AudioPathInfo &path);

    signals:
        void pathChanged(const AudioPathInfo &path);

    private:
        explicit AudioClip(Handle handle, Model *model);

        QScopedPointer<AudioClipPrivate> d_ptr;
    };

}

Q_DECLARE_METATYPE(dspx::AudioPathInfo)

#endif // DSPXMODEL_AUDIOCLIP_H
