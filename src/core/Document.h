#ifndef DSPXMODEL_DOCUMENT_H
#define DSPXMODEL_DOCUMENT_H

#include <QObject>
#include <QScopedPointer>

#include <dspxmodelCore/DSPXModelCoreGlobal.h>

class QIODevice;

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

        void writeSnapshot(QIODevice *device) const;
        void setCommitLogDevice(QIODevice *device);
        QIODevice *commitLogDevice() const;
        static Document *restore(QIODevice *snapshotDevice, QIODevice *commitLogDevice = nullptr, QObject *parent = nullptr);

        dini::Transaction *transaction() const;
        void setTransaction(dini::Transaction *transaction);

    private:
        explicit Document(DocumentPrivate *d, QObject *parent = nullptr);

        QScopedPointer<DocumentPrivate> d_ptr;
    };

}

#endif // DSPXMODEL_DOCUMENT_H
