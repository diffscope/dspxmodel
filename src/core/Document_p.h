#ifndef DSPXMODEL_DOCUMENT_P_H
#define DSPXMODEL_DOCUMENT_P_H

#include <memory>

#include <dini/engine.h>

namespace dini {
    class Transaction;
}

namespace dspx {

    class DocumentPrivate {
    public:
        DocumentPrivate();

        std::unique_ptr<dini::DocumentEngine> engine;
        dini::Transaction *transaction = nullptr;
    };

}

#endif // DSPXMODEL_DOCUMENT_P_H
