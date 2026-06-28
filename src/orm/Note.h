#ifndef DSPXMODEL_NOTE_H
#define DSPXMODEL_NOTE_H

#include <QString>

#include <dspxmodelORM/EntityObject.h>

namespace dspx {

    class NoteSequence;
    class PhonemeSequence;
    class VibratoPointDataArray;

    class NotePrivate;

    /**
     * @brief Note.
     */
    class DSPXMODEL_ORM_EXPORT Note : public EntityObject {
        Q_OBJECT
        Q_PROPERTY(int centShift READ centShift WRITE setCentShift NOTIFY centShiftChanged)
        Q_PROPERTY(int keyNum READ keyNum WRITE setKeyNum NOTIFY keyNumChanged)
        Q_PROPERTY(QString language READ language WRITE setLanguage NOTIFY languageChanged)
        Q_PROPERTY(int length READ length WRITE setLength NOTIFY lengthChanged)
        Q_PROPERTY(QString lyric READ lyric WRITE setLyric NOTIFY lyricChanged)
        Q_PROPERTY(int pos READ pos WRITE setPos NOTIFY posChanged)
        Q_PROPERTY(QString originalPronunciation READ originalPronunciation WRITE setOriginalPronunciation NOTIFY originalPronunciationChanged)
        Q_PROPERTY(QString editedPronunciation READ editedPronunciation WRITE setEditedPronunciation NOTIFY editedPronunciationChanged)
        Q_PROPERTY(bool overlapped READ overlapped NOTIFY overlappedChanged)
        Q_PROPERTY(Note *previousItem READ previousItem NOTIFY previousItemChanged)
        Q_PROPERTY(Note *nextItem READ nextItem NOTIFY nextItemChanged)
        Q_PROPERTY(int vibratoAmplitude READ vibratoAmplitude WRITE setVibratoAmplitude NOTIFY vibratoAmplitudeChanged)
        Q_PROPERTY(double vibratoEnd READ vibratoEnd WRITE setVibratoEnd NOTIFY vibratoEndChanged)
        Q_PROPERTY(double vibratoFrequency READ vibratoFrequency WRITE setVibratoFrequency NOTIFY vibratoFrequencyChanged)
        Q_PROPERTY(int vibratoOffset READ vibratoOffset WRITE setVibratoOffset NOTIFY vibratoOffsetChanged)
        Q_PROPERTY(double vibratoPhase READ vibratoPhase WRITE setVibratoPhase NOTIFY vibratoPhaseChanged)
        Q_PROPERTY(double vibratoStart READ vibratoStart WRITE setVibratoStart NOTIFY vibratoStartChanged)
        Q_PROPERTY(PhonemeSequence *editedPhonemes READ editedPhonemes CONSTANT)
        Q_PROPERTY(PhonemeSequence *originalPhonemes READ originalPhonemes CONSTANT)
        Q_PROPERTY(VibratoPointDataArray *vibratoAmplitudeControlPoints READ vibratoAmplitudeControlPoints CONSTANT)
        Q_PROPERTY(VibratoPointDataArray *vibratoFrequencyControlPoints READ vibratoFrequencyControlPoints CONSTANT)
        Q_PROPERTY(NoteSequence *noteSequence READ noteSequence NOTIFY noteSequenceChanged)
    public:
        ~Note() override;

        /**
         * @brief Gets cent shift.
         * @post centShift() >= -50 && centShift() <= 50.
         */
        int centShift() const;
        /**
         * @brief Sets cent shift.
         * @pre centShift >= -50 && centShift <= 50.
         * @post centShift() == centShift.
         */
        void setCentShift(int centShift);

        /**
         * @brief Gets key number.
         * @post keyNum() >= 0 && keyNum() <= 127.
         */
        int keyNum() const;
        /**
         * @brief Sets key number.
         * @pre keyNum >= 0 && keyNum <= 127.
         * @post keyNum() == keyNum.
         */
        void setKeyNum(int keyNum);

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
         * @brief Gets lyric.
         */
        QString lyric() const;
        /**
         * @brief Sets lyric.
         * @post lyric() == lyric.
         */
        void setLyric(const QString &lyric);

        /**
         * @brief Gets pos.
         *
         * This property is the position of the note in the note sequence.
         *
         * @post pos() >= 0.
         */
        int pos() const;
        /**
         * @brief Sets pos.
         * @pre pos >= 0.
         * @post pos() == pos.
         */
        void setPos(int pos);

        /**
         * @brief Gets original pronunciation.
         */
        QString originalPronunciation() const;
        /**
         * @brief Sets original pronunciation.
         * @post originalPronunciation() == originalPronunciation.
         */
        void setOriginalPronunciation(const QString &originalPronunciation);

        /**
         * @brief Gets edited pronunciation.
         */
        QString editedPronunciation() const;
        /**
         * @brief Sets edited pronunciation.
         * @post editedPronunciation() == editedPronunciation.
         */
        void setEditedPronunciation(const QString &editedPronunciation);

        /**
         * @brief Gets whether this note overlaps another note in the sequence.
         */
        bool overlapped() const;

        /**
         * @brief Gets previous item.
         */
        Note *previousItem() const;
        /**
         * @brief Gets next item.
         */
        Note *nextItem() const;

        /**
         * @brief Gets vibrato amplitude.
         * @post vibratoAmplitude() >= 0.
         */
        int vibratoAmplitude() const;
        /**
         * @brief Sets vibrato amplitude.
         * @pre vibratoAmplitude >= 0.
         * @post vibratoAmplitude() == vibratoAmplitude.
         */
        void setVibratoAmplitude(int vibratoAmplitude);

        /**
         * @brief Gets vibrato end.
         * @post vibratoEnd() >= 0.0 && vibratoEnd() <= 1.0.
         */
        double vibratoEnd() const;
        /**
         * @brief Sets vibrato end.
         * @pre vibratoEnd >= 0.0 && vibratoEnd <= 1.0.
         * @post vibratoEnd() == vibratoEnd.
         */
        void setVibratoEnd(double vibratoEnd);

        /**
         * @brief Gets vibrato frequency.
         * @post vibratoFrequency() >= 0.0.
         */
        double vibratoFrequency() const;
        /**
         * @brief Sets vibrato frequency.
         * @pre vibratoFrequency >= 0.0.
         * @post vibratoFrequency() == vibratoFrequency.
         */
        void setVibratoFrequency(double vibratoFrequency);

        /**
         * @brief Gets vibrato offset.
         */
        int vibratoOffset() const;
        /**
         * @brief Sets vibrato offset.
         * @post vibratoOffset() == vibratoOffset.
         */
        void setVibratoOffset(int vibratoOffset);

        /**
         * @brief Gets vibrato phase.
         * @post vibratoPhase() >= 0.0 && vibratoPhase() <= 1.0.
         */
        double vibratoPhase() const;
        /**
         * @brief Sets vibrato phase.
         * @pre vibratoPhase >= 0.0 && vibratoPhase <= 1.0.
         * @post vibratoPhase() == vibratoPhase.
         */
        void setVibratoPhase(double vibratoPhase);

        /**
         * @brief Gets vibrato start.
         * @post vibratoStart() >= 0.0 && vibratoStart() <= 1.0.
         */
        double vibratoStart() const;
        /**
         * @brief Sets vibrato start.
         * @pre vibratoStart >= 0.0 && vibratoStart <= 1.0.
         * @post vibratoStart() == vibratoStart.
         */
        void setVibratoStart(double vibratoStart);

        /**
         * @brief Gets edited phoneme sequence.
         * @post editedPhonemes() != nullptr.
         */
        PhonemeSequence *editedPhonemes() const;
        /**
         * @brief Gets original phoneme sequence.
         * @post originalPhonemes() != nullptr.
         */
        PhonemeSequence *originalPhonemes() const;
        /**
         * @brief Gets vibrato amplitude control points.
         * @post vibratoAmplitudeControlPoints() != nullptr.
         */
        VibratoPointDataArray *vibratoAmplitudeControlPoints() const;
        /**
         * @brief Gets vibrato frequency control points.
         * @post vibratoFrequencyControlPoints() != nullptr.
         */
        VibratoPointDataArray *vibratoFrequencyControlPoints() const;

        /**
         * @brief Gets note sequence.
         */
        NoteSequence *noteSequence() const;

    signals:
        void centShiftChanged(int centShift);
        void keyNumChanged(int keyNum);
        void languageChanged(const QString &language);
        void lengthChanged(int length);
        void lyricChanged(const QString &lyric);
        void posChanged(int pos);
        void originalPronunciationChanged(const QString &originalPronunciation);
        void editedPronunciationChanged(const QString &editedPronunciation);
        void overlappedChanged(bool overlapped);
        void previousItemChanged(Note *previousItem);
        void nextItemChanged(Note *nextItem);
        void vibratoAmplitudeChanged(int vibratoAmplitude);
        void vibratoEndChanged(double vibratoEnd);
        void vibratoFrequencyChanged(double vibratoFrequency);
        void vibratoCentOffsetChanged(int vibratoCentOffset);
        void vibratoPhaseChanged(double vibratoPhase);
        void vibratoStartChanged(double vibratoStart);
        void noteSequenceChanged(NoteSequence *noteSequence);

    private:
        explicit Note(Handle handle, Model *model);

        QScopedPointer<NotePrivate> d_ptr;
    };

}

#endif // DSPXMODEL_NOTE_H
