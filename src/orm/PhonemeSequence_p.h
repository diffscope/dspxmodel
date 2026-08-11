#ifndef DSPXMODEL_PHONEMESEQUENCE_P_H
#define DSPXMODEL_PHONEMESEQUENCE_P_H

#include <map>
#include <utility>

#include <QMetaObject>

#include <dspxmodelORM/PhonemeSequence.h>

#include <dini/value.h>

#include <dspxmodelORM/private/ORMUtils_p.h>
#include <dspxmodelORM/private/JSIterable_p.h>

namespace dspx {

    class PhonemeSequencePrivate {
        Q_DECLARE_PUBLIC(PhonemeSequence)
    public:
        PhonemeSequencePrivate(PhonemeSequence *q, Note *note, PhonemeSequence::PhonemeRole role);

        DSPXMODEL_DECLARE_GET(PhonemeSequence)
        DSPXMODEL_FORWARD_CONSTRUCTOR(PhonemeSequence)

        void refresh(bool notify);
        Handle relationHandle() const;
        dini::Value associationValue() const;
        void addOriginalItem(Phoneme *item);
        void removeOriginalItem(Phoneme *item);
        void originalStartChanged(Phoneme *item);
        void refreshOriginal(bool notify);
        void destroyOriginalItems();
        void clearOriginalItemsForModelDestruction();

        PhonemeSequence *q_ptr = nullptr;
        Note *note = nullptr;
        PhonemeSequence::PhonemeRole role = PhonemeSequence::Original;
        int size = 0;
        Phoneme *first = nullptr;
        Phoneme *last = nullptr;
        std::map<std::pair<int, quint64>, Phoneme *> originalItems;
        std::map<Phoneme *, QMetaObject::Connection> originalStartConnections;

        JSIterable *jsIterable = nullptr;
    };

}

#endif // DSPXMODEL_PHONEMESEQUENCE_P_H
