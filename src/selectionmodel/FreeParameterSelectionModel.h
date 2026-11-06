#ifndef DSPXMODEL_FREEPARAMETERSELECTIONMODEL_H
#define DSPXMODEL_FREEPARAMETERSELECTIONMODEL_H

#include <QObject>
#include <QScopedPointer>
#include <QString>
#include <qqmlintegration.h>

#include <dspxmodelORM/FreeValueDataArray.h>
#include <dspxmodelSelectionModel/DSPXModelSelectionModelGlobal.h>

namespace dspx {

    class SelectionModel;
    class SingingClip;
    class FreeParameterSelectionModelPrivate;

    /**
     * @brief Stores a range in an editable free parameter slot.
     *
     * The clip, parameter ID, and role identify the slot independently of its
     * stored values. An empty range retains the slot as the editing context.
     */
    class DSPXMODEL_SELECTIONMODEL_EXPORT FreeParameterSelectionModel : public QObject {
        Q_OBJECT
        QML_ELEMENT
        QML_UNCREATABLE("")
        Q_DECLARE_PRIVATE(FreeParameterSelectionModel)
        Q_MOC_INCLUDE(<dspxmodelORM/SingingClip.h>)
        Q_PROPERTY(SingingClip *singingClip READ singingClip NOTIFY contextChanged)
        Q_PROPERTY(QString parameterId READ parameterId NOTIFY contextChanged)
        Q_PROPERTY(FreeValueDataArray::FreeValueRole role READ role NOTIFY contextChanged)
        Q_PROPERTY(int start READ start NOTIFY rangeChanged)
        Q_PROPERTY(int end READ end NOTIFY rangeChanged)
        Q_PROPERTY(bool hasSelection READ hasSelection NOTIFY hasSelectionChanged)

    public:
        ~FreeParameterSelectionModel() override;

        SingingClip *singingClip() const;
        QString parameterId() const;
        FreeValueDataArray::FreeValueRole role() const;

        /// Returns the inclusive start in clip content ticks.
        int start() const;
        /// Returns the exclusive end in clip content ticks.
        int end() const;
        bool hasSelection() const;

    Q_SIGNALS:
        void contextChanged();
        void rangeChanged();
        void hasSelectionChanged();

    private:
        friend class SelectionModel;
        explicit FreeParameterSelectionModel(SelectionModel *parent);
        QScopedPointer<FreeParameterSelectionModelPrivate> d_ptr;
    };

}

#endif // DSPXMODEL_FREEPARAMETERSELECTIONMODEL_H
