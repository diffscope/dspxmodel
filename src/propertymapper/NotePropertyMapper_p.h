#ifndef DSPXMODEL_NOTEPROPERTYMAPPER_P_H
#define DSPXMODEL_NOTEPROPERTYMAPPER_P_H

#include "NotePropertyMapper.h"

#include <dspxmodelORM/Note.h>
#include <dspxmodelORM/NoteSequence.h>
#include <dspxmodelORM/SingingClip.h>
#include <dspxmodelSelectionModel/NoteSelectionModel.h>
#include <dspxmodelSelectionModel/SelectionModel.h>
#include <dspxmodelORM/PhonemeSequence.h>

#include "PropertyMapperData_p.h"

namespace dspx {
    class NotePropertyMapperPrivate : public PropertyMapperData<
        NotePropertyMapper,
        NotePropertyMapperPrivate,
        dspx::Note,
        PropertyMetadata<dspx::Note, &dspx::Note::centShift, &dspx::Note::setCentShift, decltype(&dspx::Note::centShiftChanged)>,
        PropertyMetadata<dspx::Note, &dspx::Note::keyNumber, &dspx::Note::setKeyNumber, decltype(&dspx::Note::keyNumberChanged)>,
        PropertyMetadata<dspx::Note, &dspx::Note::language, &dspx::Note::setLanguage, decltype(&dspx::Note::languageChanged)>,
        PropertyMetadata<dspx::Note, &dspx::Note::length, &dspx::Note::setLength, decltype(&dspx::Note::lengthChanged)>,
        PropertyMetadata<dspx::Note, &dspx::Note::lyric, &dspx::Note::setLyric, decltype(&dspx::Note::lyricChanged)>,
        PropertyMetadata<dspx::Note, &dspx::Note::position, &dspx::Note::setPosition, decltype(&dspx::Note::positionChanged)>,
        PropertyMetadata<dspx::Note, &dspx::Note::originalPronunciation, &dspx::Note::setOriginalPronunciation, decltype(&dspx::Note::originalPronunciationChanged)>,
        PropertyMetadata<dspx::Note, &dspx::Note::editedPronunciation, &dspx::Note::setEditedPronunciation, decltype(&dspx::Note::editedPronunciationChanged)>,
        PropertyMetadata<dspx::Note,
            [](const dspx::Note *note) { return note->noteSequence() ? note->noteSequence()->singingClip() : nullptr; },
            nullptr,
            decltype(&dspx::Note::noteSequenceChanged)
        >,
        PropertyMetadata<dspx::Note, &dspx::Note::editedPhonemes, nullptr, std::nullptr_t>,
        PropertyMetadata<dspx::Note, &dspx::Note::originalPhonemes, nullptr, std::nullptr_t>
    > {
        Q_DECLARE_PUBLIC(NotePropertyMapper)
    public:
        NotePropertyMapperPrivate() : PropertyMapperData(
            {&dspx::Note::centShiftChanged},
            {&dspx::Note::keyNumberChanged},
            {&dspx::Note::languageChanged},
            {&dspx::Note::lengthChanged},
            {&dspx::Note::lyricChanged},
            {&dspx::Note::positionChanged},
            {&dspx::Note::originalPronunciationChanged},
            {&dspx::Note::editedPronunciationChanged},
            {&dspx::Note::noteSequenceChanged},
            {nullptr},
            {nullptr}
        ) {}

        dspx::SelectionModel *selectionModel = nullptr;
        dspx::NoteSelectionModel *noteSelectionModel = nullptr;

        void setSelectionModel(dspx::SelectionModel *selectionModel_);
        void attachSelectionModel();
        void detachSelectionModel();

        enum {
            CentShiftProperty = 0,
            KeyNumberProperty = 1,
            LanguageProperty = 2,
            LengthProperty = 3,
            LyricProperty = 4,
            PositionProperty = 5,
            OriginalPronunciationProperty = 6,
            EditedPronunciationProperty = 7,
            SingingClipProperty = 8,
            EditedPhonemesProperty = 9,
            OriginalPhonemesProperty = 10,
        };

        template<int i>
        void notifyValueChange() {
            Q_Q(NotePropertyMapper);
            if constexpr (i == CentShiftProperty) {
                q->centShiftChanged();
            } else if constexpr (i == KeyNumberProperty) {
                q->keyNumberChanged();
            } else if constexpr (i == LanguageProperty) {
                q->languageChanged();
            } else if constexpr (i == LengthProperty) {
                q->lengthChanged();
            } else if constexpr (i == LyricProperty) {
                q->lyricChanged();
            } else if constexpr (i == PositionProperty) {
                q->positionChanged();
            } else if constexpr (i == OriginalPronunciationProperty) {
                q->originalPronunciationChanged();
            } else if constexpr (i == EditedPronunciationProperty) {
                q->editedPronunciationChanged();
            } else if constexpr (i == SingingClipProperty) {
                q->singingClipChanged();
            } else if constexpr (i == EditedPhonemesProperty) {
                q->editedPhonemesChanged();
            } else if constexpr (i == OriginalPhonemesProperty) {
                q->originalPhonemesChanged();
            }
        }
    };
}

#endif // DSPXMODEL_NOTEPROPERTYMAPPER_P_H
