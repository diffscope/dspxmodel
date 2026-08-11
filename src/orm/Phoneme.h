#ifndef DSPXMODEL_PHONEME_H
#define DSPXMODEL_PHONEME_H

#include <QScopedPointer>
#include <QString>
#include <qqmlintegration.h>

#include <dspxmodelORM/EntityObject.h>

namespace opendspx {
    struct Phoneme;
}

namespace dspx {

    class PhonemeSequence;

    class PhonemePrivate;

    /**
     * @brief Phoneme.
     */
    class DSPXMODEL_ORM_EXPORT Phoneme : public EntityObject {
        Q_OBJECT
        QML_ELEMENT
        QML_UNCREATABLE("")
        Q_DECLARE_PRIVATE(Phoneme)
        Q_PROPERTY(PhonemeRole role READ role CONSTANT)
        Q_PROPERTY(QString language READ language WRITE setLanguage NOTIFY languageChanged)
        Q_PROPERTY(int start READ start WRITE setStart NOTIFY startChanged)
        Q_PROPERTY(QString token READ token WRITE setToken NOTIFY tokenChanged)
        Q_PROPERTY(bool onset READ onset WRITE setOnset NOTIFY onsetChanged)
        Q_PROPERTY(Phoneme *previousItem READ previousItem NOTIFY previousItemChanged)
        Q_PROPERTY(Phoneme *nextItem READ nextItem NOTIFY nextItemChanged)
        Q_PROPERTY(PhonemeSequence *phonemeSequence READ phonemeSequence NOTIFY phonemeSequenceChanged)
    public:
        /**
         * @brief Phoneme role.
         */
        enum PhonemeRole {
            Original,
            Edited,
        };
        Q_ENUM(PhonemeRole)

        /**
         * @brief Gets role.
         */
        PhonemeRole role() const;

        /**
         * @brief Gets language.
         */
        QString language() const;
        /**
         * @brief Sets language.
         * @pre role() == Original || model()->document()->transaction() is active.
         * @post language() == language.
         */
        void setLanguage(const QString &language);

        /**
         * @brief Gets start.
         *
         * This property is the start position in milliseconds from note on.
         *
         * This property is the position index of the phoneme in the phoneme sequence.
         *
         */
        int start() const;
        /**
         * @brief Sets start.
         * @pre role() == Original || model()->document()->transaction() is active.
         * @post start() == start.
         */
        void setStart(int start);

        /**
         * @brief Gets token.
         */
        QString token() const;
        /**
         * @brief Sets token.
         * @pre role() == Original || model()->document()->transaction() is active.
         * @post token() == token.
         */
        void setToken(const QString &token);

        /**
         * @brief Gets onset.
         */
        bool onset() const;
        /**
         * @brief Sets onset.
         * @pre role() == Original || model()->document()->transaction() is active.
         * @post onset() == onset.
         */
        void setOnset(bool onset);

        /**
         * @brief Gets previous item.
         */
        Phoneme *previousItem() const;
        /**
         * @brief Gets next item.
         */
        Phoneme *nextItem() const;

        /**
         * @brief Gets phoneme sequence.
         */
        PhonemeSequence *phonemeSequence() const;

        /**
         * @brief Converts to OpenDSPX phoneme.
         */
        opendspx::Phoneme toOpenDSPX() const;
        /**
         * @brief Converts from OpenDSPX phoneme.
         * @note Typically, this method SHOULD only be called on a newly created object.
         * @pre role() == Original || model()->document()->transaction() is active.
         */
        void fromOpenDSPX(const opendspx::Phoneme &phoneme);

    signals:
        void languageChanged(const QString &language);
        void startChanged(int start);
        void tokenChanged(const QString &token);
        void onsetChanged(bool onset);
        void previousItemChanged(Phoneme *previousItem);
        void nextItemChanged(Phoneme *nextItem);
        void phonemeSequenceChanged(PhonemeSequence *phonemeSequence);

    private:
        ~Phoneme() override;

        explicit Phoneme(Handle handle, Model *model, PhonemeRole role);

        QScopedPointer<PhonemePrivate> d_ptr;
    };

}

#endif // DSPXMODEL_PHONEME_H
