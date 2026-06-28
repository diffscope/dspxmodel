#ifndef DSPXMODEL_KEYSIGNATURESEQUENCE_H
#define DSPXMODEL_KEYSIGNATURESEQUENCE_H

#include <QList>
#include <QObject>

#include <dspxmodelORM/DSPXModelORMGlobal.h>

namespace dspx {

    class KeySignature;
    class Model;

    class KeySignatureSequencePrivate;

    /**
     * @brief Key signature sequence.
     */
    class DSPXMODEL_ORM_EXPORT KeySignatureSequence : public QObject {
        Q_OBJECT
        Q_PROPERTY(int size READ size NOTIFY sizeChanged)
        Q_PROPERTY(KeySignature *firstItem READ firstItem NOTIFY firstItemChanged)
        Q_PROPERTY(KeySignature *lastItem READ lastItem NOTIFY lastItemChanged)
        Q_PROPERTY(Model *model READ model CONSTANT)
    public:
        ~KeySignatureSequence()  override;

        /**
         * @brief Gets size.
         * @post size() >= 0.
         */
        int size() const;
        /**
         * @brief Gets first item.
         */
        KeySignature *firstItem() const;
        /**
         * @brief Gets last item.
         */
        KeySignature *lastItem() const;

        /**
         * @brief Gets slice.
         * @pre position >= 0.
         * @pre length >= 0.
         * @returns Items of which the position index property is in the range [position, position + length).
         */
        Q_INVOKABLE QList<KeySignature *> slice(int position, int length) const;
        /**
         * @brief Gets whether item is contained.
         */
        Q_INVOKABLE bool contains(KeySignature *item) const;
        /**
         * @brief Inserts item.
         * @pre item is not null.
         * @post If successful, item is contained in this sequence.
         * @returns true if successful, false if item is already contained in this sequence or another sequence.
         */
        Q_INVOKABLE bool insertItem(KeySignature *item);
        /**
         * @brief Removes item.
         * @pre item is not null.
         * @post If successful, item is not contained in this sequence.
         * @returns true if successful, false if item is not contained in this sequence.
         */
        Q_INVOKABLE bool removeItem(KeySignature *item);
        /**
         * @brief Moves item.
         * @pre item is not null.
         * @pre sequence is not null.
         * @post If successful, item is contained in sequence.
         * @returns true if successful, false if item is not contained in this sequence, or item is already contained in
         * the target sequence or another sequence.
         */
        Q_INVOKABLE bool moveItem(KeySignature *item, KeySignatureSequence *sequence);

        /**
         * @brief Gets model.
         * @post model() != nullptr.
         */
        Model *model() const;

    signals:
        void sizeChanged(int size);
        void firstItemChanged(KeySignature *firstItem);
        void lastItemChanged(KeySignature *lastItem);
        void itemAboutToInsert(KeySignature *item, KeySignatureSequence *sequenceFromWhichMoved = nullptr);
        void itemInserted(KeySignature *item, KeySignatureSequence *sequenceFromWhichMoved = nullptr);
        void itemAboutToRemove(KeySignature *item, KeySignatureSequence *sequenceToWhichMoved = nullptr);
        void itemRemoved(KeySignature *item, KeySignatureSequence *sequenceToWhichMoved = nullptr);

    private:
        explicit KeySignatureSequence(Model *model);

        QScopedPointer<KeySignatureSequencePrivate> d_ptr;
    };

}

#endif // DSPXMODEL_KEYSIGNATURESEQUENCE_H
