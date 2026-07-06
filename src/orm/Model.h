#ifndef DSPXMODEL_MODEL_H
#define DSPXMODEL_MODEL_H

#include <QScopedPointer>
#include <QString>

#include <dspxmodelORM/EntityObject.h>

namespace opendspx {
    struct Model;
}

namespace dspx {

    class Document;

    class AnchorNode;
    class AudioClip;
    class Clip;
    class DynamicMixingAnchor;
    class KeySignature;
    class KeySignatureSequence;
    class Label;
    class LabelSequence;
    class MixedSinger;
    class Note;
    class Parameter;
    class Phoneme;
    class SingleSinger;
    class SingingClip;
    class Sources;
    class Tempo;
    class TempoSequence;
    class TimeSignature;
    class TimeSignatureSequence;
    class Track;
    class TrackList;

    class ModelPrivate;

    /**
     * @brief Project model.
     */
    class DSPXMODEL_ORM_EXPORT Model : public EntityObject {
        Q_OBJECT
        Q_DECLARE_PRIVATE(Model)
        Q_PROPERTY(QString projectName READ projectName WRITE setProjectName NOTIFY projectNameChanged)
        Q_PROPERTY(QString projectAuthor READ projectAuthor WRITE setProjectAuthor NOTIFY projectAuthorChanged)
        Q_PROPERTY(int globalCentShift READ globalCentShift WRITE setGlobalCentShift NOTIFY globalCentShiftChanged)
        Q_PROPERTY(bool multiChannelOutput READ multiChannelOutput WRITE setMultiChannelOutput NOTIFY multiChannelOutputChanged)
        Q_PROPERTY(double gain READ gain WRITE setGain NOTIFY gainChanged)
        Q_PROPERTY(double pan READ pan WRITE setPan NOTIFY panChanged)
        Q_PROPERTY(bool mute READ mute WRITE setMute NOTIFY muteChanged)
        Q_PROPERTY(bool loopEnabled READ loopEnabled WRITE setLoopEnabled NOTIFY loopEnabledChanged)
        Q_PROPERTY(int loopStart READ loopStart WRITE setLoopStart NOTIFY loopStartChanged)
        Q_PROPERTY(int loopLength READ loopLength WRITE setLoopLength NOTIFY loopLengthChanged)
        Q_PROPERTY(LabelSequence *labels READ labels CONSTANT)
        Q_PROPERTY(KeySignatureSequence *keySignatures READ keySignatures CONSTANT)
        Q_PROPERTY(TempoSequence *tempos READ tempos CONSTANT)
        Q_PROPERTY(TimeSignatureSequence *timeSignatures READ timeSignatures CONSTANT)
        Q_PROPERTY(TrackList *tracks READ tracks CONSTANT)
    public:
        explicit Model(Document *document, QObject *parent = nullptr);
        ~Model() override;

        /**
         * @brief Gets the bound document.
         * @post document() != nullptr.
         */
        Document *document() const;

        /**
         * @brief Gets project name.
         */
        QString projectName() const;
        /**
         * @brief Sets project name.
         * @pre document()->transaction() != nullptr && document()->transaction()->state() == dini::TransactionState::Active.
         * @post projectName() == projectName.
         */
        void setProjectName(const QString &projectName);

        /**
         * @brief Gets project author.
         */
        QString projectAuthor() const;
        /**
         * @brief Sets project author.
         * @pre document()->transaction() != nullptr && document()->transaction()->state() == dini::TransactionState::Active.
         * @post projectAuthor() == projectAuthor.
         */
        void setProjectAuthor(const QString &projectAuthor);

        /**
         * @brief Gets global cent shift.
         * @post globalCentShift() >= -50 && globalCentShift() <= 50.
         */
        int globalCentShift() const;
        /**
         * @brief Sets global cent shift.
         * @pre document()->transaction() != nullptr && document()->transaction()->state() == dini::TransactionState::Active.
         * @pre globalCentShift >= -50 && globalCentShift <= 50.
         * @post globalCentShift() == globalCentShift.
         */
        void setGlobalCentShift(int globalCentShift);

        /**
         * @brief Gets multi-channel output.
         *
         * This property indicates whether all tracks are mixed into two stereo channels (when false) or output
         * separatedly (track 1 to channel 1/2, track 2 to channel 3/4, etc.) (when true).
         */
        bool multiChannelOutput() const;
        /**
         * @brief Sets multi-channel output.
         * @pre document()->transaction() != nullptr && document()->transaction()->state() == dini::TransactionState::Active.
         * @post multiChannelOutput() == multiChannelOutput.
         */
        void setMultiChannelOutput(bool multiChannelOutput);

        /**
         * @brief Gets gain.
         * @post gain() >= 0.0.
         */
        double gain() const;
        /**
         * @brief Sets gain.
         * @pre document()->transaction() != nullptr && document()->transaction()->state() == dini::TransactionState::Active.
         * @pre gain >= 0.0.
         * @post gain() == gain.
         */
        void setGain(double gain);

        /**
         * @brief Gets pan.
         * @post pan() >= -1.0 && pan() <= 1.0.
         */
        double pan() const;
        /**
         * @brief Sets pan.
         * @pre document()->transaction() != nullptr && document()->transaction()->state() == dini::TransactionState::Active.
         * @pre pan >= -1.0 && pan <= 1.0.
         * @post pan() == pan.
         */
        void setPan(double pan);

        /**
         * @brief Gets mute.
         */
        bool mute() const;
        /**
         * @brief Sets mute.
         * @pre document()->transaction() != nullptr && document()->transaction()->state() == dini::TransactionState::Active.
         * @post mute() == mute.
         */
        void setMute(bool mute);

        /**
         * @brief Gets whether loop is enabled.
         */
        bool loopEnabled() const;
        /**
         * @brief Sets whether loop is enabled.
         * @pre document()->transaction() != nullptr && document()->transaction()->state() == dini::TransactionState::Active.
         * @post loopEnabled() == loopEnabled.
         */
        void setLoopEnabled(bool loopEnabled);

        /**
         * @brief Gets loop start.
         * @post loopStart() >= 0.
         */
        int loopStart() const;
        /**
         * @brief Sets loop start.
         * @pre document()->transaction() != nullptr && document()->transaction()->state() == dini::TransactionState::Active.
         * @pre loopStart >= 0.
         * @post loopStart() == loopStart.
         */
        void setLoopStart(int loopStart);

        /**
         * @brief Gets loop length.
         * @post loopLength() > 0.
         */
        int loopLength() const;
        /**
         * @brief Sets loop length.
         * @pre document()->transaction() != nullptr && document()->transaction()->state() == dini::TransactionState::Active.
         * @pre loopLength > 0.
         * @post loopLength() == loopLength.
         */
        void setLoopLength(int loopLength);

        /**
         * @brief Gets label sequence.
         * @post labels() != nullptr.
         */
        LabelSequence *labels() const;
        /**
         * @brief Gets key signature sequence.
         * @post keySignatures() != nullptr.
         */
        KeySignatureSequence *keySignatures() const;
        /**
         * @brief Gets tempo sequence.
         * @post tempos() != nullptr.
         */
        TempoSequence *tempos() const;
        /**
         * @brief Gets time signature sequence.
         * @post timeSignatures() != nullptr.
         */
        TimeSignatureSequence *timeSignatures() const;
        /**
         * @brief Gets track list.
         * @post tracks() != nullptr.
         */
        TrackList *tracks() const;

        /**
         * @brief Converts to OpenDSPX model.
         */
        opendspx::Model toOpenDSPX() const;
        /**
         * @brief Converts from OpenDSPX model.
         * @pre document()->transaction() != nullptr && document()->transaction()->state() == dini::TransactionState::Active.
         * @pre model must be valid.
         */
        void fromOpenDspx(const opendspx::Model &model);

        /**
         * @brief Creates label.
         * @pre document()->transaction() != nullptr && document()->transaction()->state() == dini::TransactionState::Active.
         * @post createLabel() != nullptr.
         */
        Q_INVOKABLE Label *createLabel();
        /**
         * @brief Creates key signature.
         * @pre document()->transaction() != nullptr && document()->transaction()->state() == dini::TransactionState::Active.
         * @post createKeySignature() != nullptr.
         */
        Q_INVOKABLE KeySignature *createKeySignature();
        /**
         * @brief Creates tempo.
         * @pre document()->transaction() != nullptr && document()->transaction()->state() == dini::TransactionState::Active.
         * @post createTempo() != nullptr.
         */
        Q_INVOKABLE Tempo *createTempo();
        /**
         * @brief Creates time signature.
         * @pre document()->transaction() != nullptr && document()->transaction()->state() == dini::TransactionState::Active.
         * @post createTimeSignature() != nullptr.
         */
        Q_INVOKABLE TimeSignature *createTimeSignature();
        /**
         * @brief Creates track.
         * @pre document()->transaction() != nullptr && document()->transaction()->state() == dini::TransactionState::Active.
         * @post createTrack() != nullptr.
         */
        Q_INVOKABLE Track *createTrack();
        /**
         * @brief Creates audio clip.
         * @pre document()->transaction() != nullptr && document()->transaction()->state() == dini::TransactionState::Active.
         * @post createAudioClip() != nullptr.
         */
        Q_INVOKABLE AudioClip *createAudioClip();
        /**
         * @brief Creates singing clip.
         * @pre document()->transaction() != nullptr && document()->transaction()->state() == dini::TransactionState::Active.
         * @post createSingingClip() != nullptr.
         */
        Q_INVOKABLE SingingClip *createSingingClip();
        /**
         * @brief Creates note.
         * @pre document()->transaction() != nullptr && document()->transaction()->state() == dini::TransactionState::Active.
         * @post createNote() != nullptr.
         */
        Q_INVOKABLE Note *createNote();
        /**
         * @brief Creates phoneme.
         * @pre document()->transaction() != nullptr && document()->transaction()->state() == dini::TransactionState::Active.
         * @post createPhoneme() != nullptr.
         */
        Q_INVOKABLE Phoneme *createPhoneme();
        /**
         * @brief Creates parameter.
         * @pre document()->transaction() != nullptr && document()->transaction()->state() == dini::TransactionState::Active.
         * @post createParameter() != nullptr.
         */
        Q_INVOKABLE Parameter *createParameter();
        /**
         * @brief Creates anchor node.
         * @pre document()->transaction() != nullptr && document()->transaction()->state() == dini::TransactionState::Active.
         * @post createAnchorNode() != nullptr.
         */
        Q_INVOKABLE AnchorNode *createAnchorNode();
        /**
         * @brief Creates sources.
         * @pre document()->transaction() != nullptr && document()->transaction()->state() == dini::TransactionState::Active.
         * @post createSources() != nullptr.
         */
        Q_INVOKABLE Sources *createSources();
        /**
         * @brief Creates single singer.
         * @pre document()->transaction() != nullptr && document()->transaction()->state() == dini::TransactionState::Active.
         * @post createSingleSinger() != nullptr.
         */
        Q_INVOKABLE SingleSinger *createSingleSinger();
        /**
         * @brief Creates mixed singer.
         * @pre document()->transaction() != nullptr && document()->transaction()->state() == dini::TransactionState::Active.
         * @post createMixedSinger() != nullptr.
         */
        Q_INVOKABLE MixedSinger *createMixedSinger();
        /**
         * @brief Creates dynamic mixing anchor.
         * @pre document()->transaction() != nullptr && document()->transaction()->state() == dini::TransactionState::Active.
         * @post createDynamicMixingAnchor() != nullptr.
         */
        Q_INVOKABLE DynamicMixingAnchor *createDynamicMixingAnchor();

        /**
         * @brief Destroys an item and schedules its ORM object for deletion.
         * @pre document()->transaction() != nullptr && document()->transaction()->state() == dini::TransactionState::Active.
         * @pre item != nullptr.
         * @post If successful, the item is removed from the document.
         */
        Q_INVOKABLE bool destroyItem(EntityObject *item);

    signals:
        void projectNameChanged(const QString &projectName);
        void projectAuthorChanged(const QString &projectAuthor);
        void globalCentShiftChanged(int globalCentShift);
        void multiChannelOutputChanged(bool multiChannelOutput);
        void gainChanged(double gain);
        void panChanged(double pan);
        void muteChanged(bool mute);
        void loopEnabledChanged(bool loopEnabled);
        void loopStartChanged(int loopStart);
        void loopLengthChanged(int loopLength);

    private:
        QScopedPointer<ModelPrivate> d_ptr;
    };

}

#endif // DSPXMODEL_MODEL_H
