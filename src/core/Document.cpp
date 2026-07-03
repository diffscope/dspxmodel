#include "Document.h"

#include "Document_p.h"
#include "Schema.h"
#include <dini/schema.h>
#include <dini/transaction.h>

namespace dspx {

    namespace {

        const dini::EngineSchema &documentSchema() {
            static const auto schema = Schema::schemaBuilder()->freeze();
            return schema;
        }

    }

    DocumentPrivate::DocumentPrivate()
        : engine(std::make_unique<dini::DocumentEngine>(documentSchema())) {
        auto transaction = engine->beginTransaction(dini::TransactionOptions {.undoable = false});
        transaction.insert(Schema::modelTable(), {});
        transaction.commit();
    }

    Document::Document(QObject *parent)
        : QObject(parent), d_ptr(new DocumentPrivate) {
    }

    Document::~Document() = default;

    dini::DocumentEngine *Document::engine() const {
        Q_D(const Document);
        return d->engine.get();
    }

    dini::Transaction *Document::transaction() const {
        Q_D(const Document);
        return d->transaction;
    }

    void Document::setTransaction(dini::Transaction *transaction) {
        Q_D(Document);
        d->transaction = transaction;
    }

}
