#ifndef DSPXMODEL_NOTEPROPERTYMAPPER_H
#define DSPXMODEL_NOTEPROPERTYMAPPER_H

#include <QObject>
#include <QVariant>
#include <QScopedPointer>
#include <qqmlintegration.h>

#include <dspxmodelPropertyMapper/DSPXModelPropertyMapperGlobal.h>

namespace dspx {
    class SelectionModel;
    class NotePropertyMapperPrivate;

    class DSPXMODEL_PROPERTYMAPPER_EXPORT NotePropertyMapper : public QObject {
        Q_OBJECT
        QML_ELEMENT
        Q_DECLARE_PRIVATE(NotePropertyMapper)

        Q_PROPERTY(dspx::SelectionModel *selectionModel READ selectionModel WRITE setSelectionModel NOTIFY selectionModelChanged)
        Q_PROPERTY(QVariant centShift READ centShift WRITE setCentShift NOTIFY centShiftChanged)
        Q_PROPERTY(QVariant keyNumber READ keyNumber WRITE setKeyNumber NOTIFY keyNumberChanged)
        Q_PROPERTY(QVariant language READ language WRITE setLanguage NOTIFY languageChanged)
        Q_PROPERTY(QVariant length READ length WRITE setLength NOTIFY lengthChanged)
        Q_PROPERTY(QVariant lyric READ lyric WRITE setLyric NOTIFY lyricChanged)
        Q_PROPERTY(QVariant position READ position WRITE setPosition NOTIFY positionChanged)
        Q_PROPERTY(QVariant originalPronunciation READ originalPronunciation WRITE setOriginalPronunciation NOTIFY originalPronunciationChanged)
        Q_PROPERTY(QVariant editedPronunciation READ editedPronunciation WRITE setEditedPronunciation NOTIFY editedPronunciationChanged)
        Q_PROPERTY(QVariant singingClip READ singingClip NOTIFY singingClipChanged)
        Q_PROPERTY(QVariant editedPhonemes READ editedPhonemes NOTIFY editedPhonemesChanged)
        Q_PROPERTY(QVariant originalPhonemes READ originalPhonemes NOTIFY originalPhonemesChanged)

    public:
        explicit NotePropertyMapper(QObject *parent = nullptr);
        ~NotePropertyMapper() override;

        dspx::SelectionModel *selectionModel() const;
        void setSelectionModel(dspx::SelectionModel *selectionModel);

        QVariant centShift() const;
        void setCentShift(const QVariant &centShift);

        QVariant keyNumber() const;
        void setKeyNumber(const QVariant &keyNumber);

        QVariant language() const;
        void setLanguage(const QVariant &language);

        QVariant length() const;
        void setLength(const QVariant &length);

        QVariant lyric() const;
        void setLyric(const QVariant &lyric);

        QVariant position() const;
        void setPosition(const QVariant &position);

        QVariant originalPronunciation() const;
        void setOriginalPronunciation(const QVariant &originalPronunciation);

        QVariant editedPronunciation() const;
        void setEditedPronunciation(const QVariant &editedPronunciation);

        QVariant singingClip() const;

        QVariant editedPhonemes() const;
        QVariant originalPhonemes() const;

    signals:
        void selectionModelChanged();
        void centShiftChanged();
        void keyNumberChanged();
        void languageChanged();
        void lengthChanged();
        void lyricChanged();
        void positionChanged();
        void originalPronunciationChanged();
        void editedPronunciationChanged();
        void singingClipChanged();
        void editedPhonemesChanged();
        void originalPhonemesChanged();

    private:
        QScopedPointer<NotePropertyMapperPrivate> d_ptr;
    };
}

#endif // DSPXMODEL_NOTEPROPERTYMAPPER_H
