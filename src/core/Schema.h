#ifndef DSPXMODEL_SCHEMA_H
#define DSPXMODEL_SCHEMA_H

#include <dspxmodelCore/DSPXModelCoreGlobal.h>

namespace dini {
    class ColumnHandle;
    class ListHandle;
    class RelationHandle;
    class SchemaBuilder;
    class TableHandle;
    class VariantHandle;
}

namespace dspx {

    class DSPXMODEL_CORE_EXPORT Schema {
    public:
        static dini::SchemaBuilder *schemaBuilder();

        static dini::TableHandle clipTable();
        static dini::TableHandle dynamicMixingAnchorTable();
        static dini::TableHandle keySignatureTable();
        static dini::TableHandle labelTable();
        static dini::TableHandle mixableTable();
        static dini::TableHandle modelTable();
        static dini::TableHandle noteTable();
        static dini::TableHandle phonemeTable();
        static dini::TableHandle parameterTable();
        static dini::TableHandle sourcesTable();
        static dini::TableHandle tempoTable();
        static dini::TableHandle timeSignatureTable();
        static dini::TableHandle anchorNodeTable();

        static dini::ListHandle singerList();
        static dini::ListHandle trackList();
        static dini::ListHandle freeValueList();
        static dini::ListHandle vibratoPointList();

        static dini::RelationHandle vibratoPointParent();
        static dini::RelationHandle anchorNodeParent();
        static dini::RelationHandle clipParent();
        static dini::RelationHandle dynamicMixingAnchorParent();
        static dini::RelationHandle freeValueParent();
        static dini::RelationHandle keySignatureParent();
        static dini::RelationHandle labelParent();
        static dini::RelationHandle noteParent();
        static dini::RelationHandle phonemeParent();
        static dini::RelationHandle parameterParent();
        static dini::RelationHandle singerParent();
        static dini::RelationHandle sourcesParent();
        static dini::RelationHandle tempoParent();
        static dini::RelationHandle timeSignatureParent();
        static dini::RelationHandle trackParent();

        static dini::VariantHandle audioClipVariant();
        static dini::VariantHandle singingClipVariant();
        static dini::VariantHandle singleSingerVariant();
        static dini::VariantHandle mixedSingerVariant();

        static dini::ColumnHandle vibratoPointRoleColumn();
        static dini::ColumnHandle vibratoPointXColumn();
        static dini::ColumnHandle vibratoPointYColumn();

        static dini::ColumnHandle clipNameColumn();
        static dini::ColumnHandle clipGainColumn();
        static dini::ColumnHandle clipPanColumn();
        static dini::ColumnHandle clipMuteColumn();
        static dini::ColumnHandle clipPositionColumn();
        static dini::ColumnHandle clipLengthColumn();
        static dini::ColumnHandle clipClipStartColumn();
        static dini::ColumnHandle clipClipLengthColumn();
        static dini::ColumnHandle clipPreviousItemColumn();
        static dini::ColumnHandle clipNextItemColumn();
        static dini::ColumnHandle clipOverlappedCountColumn();
        static dini::ColumnHandle audioClipPathColumn();

        static dini::ColumnHandle dynamicMixingAnchorPositionColumn();
        static dini::ColumnHandle dynamicMixingAnchorRatioColumn();
        static dini::ColumnHandle dynamicMixingAnchorPreviousItemColumn();
        static dini::ColumnHandle dynamicMixingAnchorNextItemColumn();

        static dini::ColumnHandle freeValueRoleColumn();
        static dini::ColumnHandle freeValueValueColumn();

        static dini::ColumnHandle labelPositionColumn();
        static dini::ColumnHandle labelTextColumn();
        static dini::ColumnHandle labelPreviousItemColumn();
        static dini::ColumnHandle labelNextItemColumn();

        static dini::ColumnHandle keySignaturePositionColumn();
        static dini::ColumnHandle keySignatureModeColumn();
        static dini::ColumnHandle keySignatureTonalityColumn();
        static dini::ColumnHandle keySignatureAccidentalTypeColumn();
        static dini::ColumnHandle keySignaturePreviousItemColumn();
        static dini::ColumnHandle keySignatureNextItemColumn();

        static dini::ColumnHandle modelProjectNameColumn();
        static dini::ColumnHandle modelProjectAuthorColumn();
        static dini::ColumnHandle modelGlobalCentShiftColumn();
        static dini::ColumnHandle modelMultiChannelOutputColumn();
        static dini::ColumnHandle modelGainColumn();
        static dini::ColumnHandle modelPanColumn();
        static dini::ColumnHandle modelMuteColumn();
        static dini::ColumnHandle modelLoopEnabledColumn();
        static dini::ColumnHandle modelLoopStartColumn();
        static dini::ColumnHandle modelLoopLengthColumn();

        static dini::ColumnHandle mixableSourcesColumn();
        static dini::ColumnHandle mixableMixedSingerColumn();

        static dini::ColumnHandle noteCentShiftColumn();
        static dini::ColumnHandle noteKeyNumberColumn();
        static dini::ColumnHandle noteLanguageColumn();
        static dini::ColumnHandle noteLengthColumn();
        static dini::ColumnHandle noteLyricColumn();
        static dini::ColumnHandle notePositionColumn();
        static dini::ColumnHandle noteOriginalPronunciationColumn();
        static dini::ColumnHandle noteEditedPronunciationColumn();
        static dini::ColumnHandle noteVibratoAmplitudeColumn();
        static dini::ColumnHandle noteVibratoEndColumn();
        static dini::ColumnHandle noteVibratoFrequencyColumn();
        static dini::ColumnHandle noteVibratoOffsetColumn();
        static dini::ColumnHandle noteVibratoPhaseColumn();
        static dini::ColumnHandle noteVibratoStartColumn();
        static dini::ColumnHandle notePreviousItemColumn();
        static dini::ColumnHandle noteNextItemColumn();
        static dini::ColumnHandle noteOverlappedCountColumn();

        static dini::ColumnHandle phonemeRoleColumn();
        static dini::ColumnHandle phonemeLanguageColumn();
        static dini::ColumnHandle phonemeStartColumn();
        static dini::ColumnHandle phonemeTokenColumn();
        static dini::ColumnHandle phonemeOnsetColumn();
        static dini::ColumnHandle phonemePreviousItemColumn();
        static dini::ColumnHandle phonemeNextItemColumn();

        static dini::ColumnHandle parameterKeyColumn();

        static dini::ColumnHandle singerExtraColumn();
        static dini::ColumnHandle singleSingerIdColumn();
        static dini::ColumnHandle mixedSingerRatioColumn();

        static dini::ColumnHandle sourcesCategoryColumn();

        static dini::ColumnHandle tempoPositionColumn();
        static dini::ColumnHandle tempoValueColumn();
        static dini::ColumnHandle tempoPreviousItemColumn();
        static dini::ColumnHandle tempoNextItemColumn();

        static dini::ColumnHandle timeSignatureIndexColumn();
        static dini::ColumnHandle timeSignatureNumeratorColumn();
        static dini::ColumnHandle timeSignatureDenominatorColumn();
        static dini::ColumnHandle timeSignaturePreviousItemColumn();
        static dini::ColumnHandle timeSignatureNextItemColumn();

        static dini::ColumnHandle trackColorIdColumn();
        static dini::ColumnHandle trackHeightColumn();
        static dini::ColumnHandle trackNameColumn();
        static dini::ColumnHandle trackGainColumn();
        static dini::ColumnHandle trackPanColumn();
        static dini::ColumnHandle trackMuteColumn();
        static dini::ColumnHandle trackSoloColumn();
        static dini::ColumnHandle trackRecordColumn();

        static dini::ColumnHandle anchorNodeRoleColumn();
        static dini::ColumnHandle anchorNodeInterpolationModeColumn();
        static dini::ColumnHandle anchorNodeXColumn();
        static dini::ColumnHandle anchorNodeYColumn();
        static dini::ColumnHandle anchorNodePreviousItemColumn();
        static dini::ColumnHandle anchorNodeNextItemColumn();
    };

}

#endif // DSPXMODEL_SCHEMA_H
