#ifndef DSPXMODEL_SOURCES_H
#define DSPXMODEL_SOURCES_H

#include <QString>

#include <dspxmodelORM/EntityObject.h>

namespace opendspx {
    struct Sources;
}

namespace dspx {

    class DynamicMixingAnchorSequence;
    class SingingClip;
    class SingerList;

    class SourcesPrivate;

    /**
     * @brief Sources.
     */
    class DSPXMODEL_ORM_EXPORT Sources : public EntityObject {
        Q_OBJECT
        Q_DECLARE_PRIVATE(Sources)
        Q_PROPERTY(QString category READ category WRITE setCategory NOTIFY categoryChanged)
        Q_PROPERTY(SingerList *singers READ singers CONSTANT)
        Q_PROPERTY(DynamicMixingAnchorSequence *dynamicMixingAnchors READ dynamicMixingAnchors CONSTANT)
        Q_PROPERTY(SingingClip *singingClip READ singingClip NOTIFY singingClipChanged)
    public:
        /**
         * @brief Gets category.
         */
        QString category() const;
        /**
         * @brief Sets category.
         * @post category() == category.
         */
        void setCategory(const QString &category);

        /**
         * @brief Gets singer list.
         * @post singers() != nullptr.
         */
        SingerList *singers() const;
        /**
         * @brief Gets dynamic mixing anchor sequence.
         * @post dynamicMixingAnchors() != nullptr.
         */
        DynamicMixingAnchorSequence *dynamicMixingAnchors() const;

        /**
         * @brief Gets singing clip.
         */
        SingingClip *singingClip() const;

        /**
         * @brief Converts to OpenDSPX sources.
         */
        opendspx::Sources toOpenDSPX() const;
        /**
         * @brief Converts from OpenDSPX sources.
         * @note Typically, this method SHOULD only be called on a newly created object.
         * @pre model()->document()->transaction() != nullptr && model()->document()->transaction()->state() == dini::TransactionState::Active.
         */
        void fromOpenDSPX(const opendspx::Sources &sources);

    signals:
        void categoryChanged(const QString &category);
        void singingClipChanged(SingingClip *singingClip);

    private:
        ~Sources() override;

        explicit Sources(Handle handle, Model *model);

        QScopedPointer<SourcesPrivate> d_ptr;
    };

}

#endif // DSPXMODEL_SOURCES_H
