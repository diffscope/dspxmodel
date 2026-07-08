#ifndef DSPXMODEL_ANCHORNODESEQUENCE_H
#define DSPXMODEL_ANCHORNODESEQUENCE_H

#include <QList>
#include <QObject>
#include <QScopedPointer>
#include <qqmlintegration.h>

#include <dspxmodelORM/DSPXModelORMGlobal.h>

namespace dspx {

    class AnchorNode;
    class Parameter;

    class AnchorNodeSequencePrivate;

    /**
     * @brief Anchor node sequence.
     */
    class DSPXMODEL_ORM_EXPORT AnchorNodeSequence : public QObject {
        Q_OBJECT
        QML_ELEMENT
        QML_UNCREATABLE("")
        Q_DECLARE_PRIVATE(AnchorNodeSequence)
        Q_PROPERTY(int size READ size NOTIFY sizeChanged)
        Q_PROPERTY(AnchorNode *firstItem READ firstItem NOTIFY firstItemChanged)
        Q_PROPERTY(AnchorNode *lastItem READ lastItem NOTIFY lastItemChanged)
        Q_PROPERTY(AnchorNodeRole role READ role CONSTANT)
        Q_PROPERTY(Parameter *parameter READ parameter CONSTANT)
    public:
        /**
         * @brief Anchor node sequence role.
         */
        enum AnchorNodeRole {
            Transform,
            Edited,
        };
        Q_ENUM(AnchorNodeRole)

        /**
         * @brief Gets size.
         * @post size() >= 0.
         */
        int size() const;
        /**
         * @brief Gets first item.
         */
        AnchorNode *firstItem() const;
        /**
         * @brief Gets last item.
         */
        AnchorNode *lastItem() const;

        /**
         * @brief Gets slice.
         * @pre position >= 0.
         * @pre length >= 0.
         * @returns Items of which the x property is in the range [position, position + length).
         */
        Q_INVOKABLE QList<AnchorNode *> slice(int position, int length) const;
        /**
         * @brief Gets whether item is contained.
         */
        Q_INVOKABLE bool contains(AnchorNode *item) const;
        /**
         * @brief Inserts item.
         * @pre item is not null.
         * @pre No other item in this sequence has item->x().
         * @post If successful, item is contained in this sequence.
         * @returns true if successful, false if item is already contained in this sequence or another sequence.
         */
        Q_INVOKABLE bool insertItem(AnchorNode *item);
        /**
         * @brief Removes item.
         * @pre item is not null.
         * @post If successful, item is not contained in this sequence.
         * @returns true if successful, false if item is not contained in this sequence.
         */
        Q_INVOKABLE bool removeItem(AnchorNode *item);
        /**
         * @brief Moves item.
         * @pre item is not null.
         * @pre sequence is not null.
         * @pre No other item in sequence has item->x().
         * @post If successful, item is contained in sequence.
         * @returns true if successful, false if item is not contained in this sequence, or item is already contained in
         * the target sequence or another sequence.
         */
        Q_INVOKABLE bool moveItem(AnchorNode *item, AnchorNodeSequence *sequence);

        /**
         * @brief Gets role.
         */
        AnchorNodeRole role() const;

        /**
         * @brief Gets parameter.
         * @post parameter() != nullptr.
         */
        Parameter *parameter() const;

    signals:
        void sizeChanged(int size);
        void firstItemChanged(AnchorNode *firstItem);
        void lastItemChanged(AnchorNode *lastItem);
        void itemAboutToInsert(AnchorNode *item, AnchorNodeSequence *sequenceFromWhichMoved = nullptr);
        void itemInserted(AnchorNode *item, AnchorNodeSequence *sequenceFromWhichMoved = nullptr);
        void itemAboutToRemove(AnchorNode *item, AnchorNodeSequence *sequenceToWhichMoved = nullptr);
        void itemRemoved(AnchorNode *item, AnchorNodeSequence *sequenceToWhichMoved = nullptr);

    private:
        ~AnchorNodeSequence() override;

        explicit AnchorNodeSequence(Parameter *parameter, AnchorNodeRole role);

        QScopedPointer<AnchorNodeSequencePrivate> d_ptr;
    };

}

#endif // DSPXMODEL_ANCHORNODESEQUENCE_H
