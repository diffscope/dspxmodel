#ifndef DSPXMODEL_PHONEME_H
#define DSPXMODEL_PHONEME_H

#include <QScopedPointer>
#include <QString>

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
        Q_DECLARE_PRIVATE(Phoneme)
        Q_PROPERTY(QString language READ language WRITE setLanguage NOTIFY languageChanged)
        Q_PROPERTY(int start READ start WRITE setStart NOTIFY startChanged)
        Q_PROPERTY(QString token READ token WRITE setToken NOTIFY tokenChanged)
        Q_PROPERTY(bool onset READ onset WRITE setOnset NOTIFY onsetChanged)
        Q_PROPERTY(Phoneme *previousItem READ previousItem NOTIFY previousItemChanged)
        Q_PROPERTY(Phoneme *nextItem READ nextItem NOTIFY nextItemChanged)
        Q_PROPERTY(PhonemeSequence *phonemeSequence READ phonemeSequence NOTIFY phonemeSequenceChanged)
    public:
        /**
         * @brief Gets language.
         */
        QString language() const;
        /**
         * @brief Sets language.
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
         * @post start() == start.
         */
        void setStart(int start);

        /**
         * @brief Gets token.
         */
        QString token() const;
        /**
         * @brief Sets token.
         * @post token() == token.
         */
        void setToken(const QString &token);

        /**
         * @brief Gets onset.
         */
        bool onset() const;
        /**
         * @brief Sets onset.
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
         * @pre model()->document()->transaction() != nullptr && model()->document()->transaction()->state() == dini::TransactionState::Active.
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

        explicit Phoneme(Handle handle, Model *model);

        QScopedPointer<PhonemePrivate> d_ptr;
    };

}

#endif // DSPXMODEL_PHONEME_H
