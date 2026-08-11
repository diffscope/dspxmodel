#ifndef DSPXMODEL_CLIPWATCHER_H
#define DSPXMODEL_CLIPWATCHER_H

#include <QObject>
#include <QScopedPointer>
#include <qqmlintegration.h>

#include <dspxmodelPiece/ClipChange.h>
#include <dspxmodelPiece/DSPXModelPieceGlobal.h>

namespace dspx {

    class ClipWatcherPrivate;
    class SingingClip;

    /**
     * @brief Manually extracts net content changes from a SingingClip.
     *
     * The watcher connects directly to ORM signals but has no knowledge of
     * document transactions. Signals only mark incremental candidates. takeChanges()
     * compares those candidates with the previous extraction baseline, returns
     * one aggregated ClipChange, advances the baseline and consumes the result.
     * Edits restored before extraction normally cancel out.
     *
     * Note-owned changes cover the union of the old and new note body. Free
     * parameter points affect their adjacent linear segments. Anchor parameters
     * follow OpenDSPX None/Linear/Hermite dependencies, including the outer
     * reference points used by Hermite slopes. Returned intervals are integer,
     * clip-relative and half-open; zero-length notes and isolated points occupy
     * one tick, and intervals are never clipped to the clip length.
     */
    class DSPXMODEL_PIECE_EXPORT ClipWatcher : public QObject {
        Q_OBJECT
        QML_ELEMENT
        Q_DECLARE_PRIVATE(ClipWatcher)
        Q_PROPERTY(SingingClip *singingClip READ singingClip WRITE setSingingClip NOTIFY singingClipChanged)

    public:
        /** @brief Creates an unbound watcher. */
        explicit ClipWatcher(QObject *parent = nullptr);

        /** @brief Destroys the watcher and disconnects all observed objects. */
        ~ClipWatcher() override;

        /** @brief Gets the observed clip, or nullptr. */
        SingingClip *singingClip() const;

        /**
         * @brief Binds a clip and establishes a fresh, empty change baseline.
         * @param singingClip Clip to observe, or nullptr to reset the watcher.
         */
        void setSingingClip(SingingClip *singingClip);

        /**
         * @brief Returns and consumes all net changes since the previous call.
         * @returns A normalized change set; an unchanged final state returns an
         * empty ClipChange.
         */
        Q_INVOKABLE ClipChange takeChanges();

    signals:
        /** @brief Emitted after the observed clip and baseline are replaced. */
        void singingClipChanged(SingingClip *singingClip);

    private:
        QScopedPointer<ClipWatcherPrivate> d_ptr;
    };

}

#endif // DSPXMODEL_CLIPWATCHER_H
