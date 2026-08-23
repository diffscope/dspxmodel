#ifndef DSPXMODEL_AUDIODSP_H
#define DSPXMODEL_AUDIODSP_H

#include <QJsonValue>
#include <QScopedPointer>
#include <QString>
#include <qqmlintegration.h>

#include <stdcorelib/support/json.h>

#include <dspxmodelORM/EntityObject.h>

namespace dspx {

    class AudioDSPList;

    class AudioDSPPrivate;

    /**
     * @brief Audio DSP.
     */
    class DSPXMODEL_ORM_EXPORT AudioDSP : public EntityObject {
        Q_OBJECT
        QML_ELEMENT
        QML_UNCREATABLE("")
        Q_DECLARE_PRIVATE(AudioDSP)
        Q_PROPERTY(QString id READ id WRITE setId NOTIFY idChanged)
        Q_PROPERTY(QJsonValue data READ data WRITE setData NOTIFY dataChanged)
        Q_PROPERTY(bool enabled READ enabled WRITE setEnabled NOTIFY enabledChanged)
        Q_PROPERTY(AudioDSPList *audioDSPList READ audioDSPList NOTIFY audioDSPListChanged)
    public:
        /**
         * @brief Gets id.
         */
        QString id() const;
        /**
         * @brief Sets id.
         * @pre model()->document()->transaction() != nullptr && model()->document()->transaction()->state() == dini::TransactionState::Active.
         * @post id() == id.
         */
        void setId(const QString &id);

        /**
         * @brief Gets data.
         */
        QJsonValue data() const;
        /**
         * @brief Sets data.
         * @pre model()->document()->transaction() != nullptr && model()->document()->transaction()->state() == dini::TransactionState::Active.
         * @post data() == data.
         */
        void setData(const QJsonValue &data);

        /**
         * @brief Gets enabled.
         */
        bool enabled() const;
        /**
         * @brief Sets enabled.
         * @pre model()->document()->transaction() != nullptr && model()->document()->transaction()->state() == dini::TransactionState::Active.
         * @post enabled() == enabled.
         */
        void setEnabled(bool enabled);

        /**
         * @brief Gets audio DSP list.
         */
        AudioDSPList *audioDSPList() const;

        /**
         * @brief Converts to OpenDSPX audio DSP.
         */
        stdc::JsonObject toOpenDSPX() const;
        /**
         * @brief Converts from OpenDSPX audio DSP.
         * @note Typically, this method SHOULD only be called on a newly created object.
         * @pre model()->document()->transaction() != nullptr && model()->document()->transaction()->state() == dini::TransactionState::Active.
         * @pre audioDSP must be valid.
         */
        void fromOpenDSPX(const stdc::JsonObject &audioDSP);

    signals:
        void idChanged(const QString &id);
        void dataChanged(const QJsonValue &data);
        void enabledChanged(bool enabled);
        void audioDSPListChanged(AudioDSPList *audioDSPList);

    private:
        ~AudioDSP() override;

        explicit AudioDSP(Handle handle, Model *model);

        QScopedPointer<AudioDSPPrivate> d_ptr;
    };

}

#endif // DSPXMODEL_AUDIODSP_H
