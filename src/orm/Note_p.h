#ifndef DSPXMODEL_NOTE_P_H
#define DSPXMODEL_NOTE_P_H

#include <dspxmodelORM/Note.h>

#include <dini/types.h>

#include <dspxmodelORM/private/ORMUtils_p.h>

namespace dspx {

    class NotePrivate {
        Q_DECLARE_PUBLIC(Note)
    public:
        explicit NotePrivate(Note *q);

        DSPXMODEL_DECLARE_GET(Note)
        DSPXMODEL_FORWARD_CONSTRUCTOR(Note)

        void setSequence(NoteSequence *sequence, bool notify);
        dini::ByteArray workspace() const;
        void setWorkspace(dini::ByteArray workspace);

        Note *q_ptr = nullptr;
        int centShift = 0;
        int keyNumber = 60;
        QString language;
        int length = 0;
        QString lyric;
        int position = 0;
        QString originalPronunciation;
        QString editedPronunciation;
        Handle previousHandle;
        Handle nextHandle;
        mutable Note *previous = nullptr;
        mutable Note *next = nullptr;
        int overlappedCount = 0;
        int vibratoAmplitude = 0;
        double vibratoEnd = 0.0;
        double vibratoFrequency = 0.0;
        int vibratoOffset = 0;
        double vibratoPhase = 0.0;
        double vibratoStart = 0.0;
        dini::ByteArray workspaceData;
        PhonemeSequence *editedPhonemes = nullptr;
        PhonemeSequence *originalPhonemes = nullptr;
        VibratoPointDataArray *vibratoAmplitudeControlPoints = nullptr;
        VibratoPointDataArray *vibratoFrequencyControlPoints = nullptr;
        NoteSequence *sequence = nullptr;
    };

}

#endif // DSPXMODEL_NOTE_P_H
