#ifndef DSPXMODEL_DOCUMENT_H
#define DSPXMODEL_DOCUMENT_H

#include <QObject>
#include <QScopedPointer>

#include <dspxmodelCore/DSPXModelCoreGlobal.h>

namespace dini {
    class DocumentEngine;
    class Transaction;
}

namespace dspx {

    class DocumentPrivate;

    class DSPXMODEL_CORE_EXPORT Document : public QObject {
        Q_OBJECT
        Q_DECLARE_PRIVATE(Document)
    public:
        explicit Document(QObject *parent = nullptr);
        ~Document() override;

        dini::DocumentEngine *engine() const;

        dini::Transaction *transaction() const;
        void setTransaction(dini::Transaction *transaction);

    private:
        QScopedPointer<DocumentPrivate> d_ptr;
    };

}

#endif // DSPXMODEL_DOCUMENT_H
