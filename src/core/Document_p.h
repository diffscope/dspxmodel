#ifndef DSPXMODEL_DOCUMENT_P_H
#define DSPXMODEL_DOCUMENT_P_H

#include <memory>

#include <QPointer>

#include <dini/engine.h>
#include <dini/event.h>

class QIODevice;

namespace dini {
    class Transaction;
}

namespace dspx {

    class DocumentPrivate {
    public:
        enum class InitializationMode {
            CreateInitialModel,
            EmptyForRestore,
        };

        explicit DocumentPrivate(InitializationMode mode = InitializationMode::CreateInitialModel);
        ~DocumentPrivate();

        void writeCommitLogEvent(const dini::EngineEvent &event);

        std::unique_ptr<dini::DocumentEngine> engine;
        QPointer<QIODevice> commitLogDevice;
        dini::Subscription commitLogSubscription;
        dini::Transaction *transaction = nullptr;
    };

}

#endif // DSPXMODEL_DOCUMENT_P_H
