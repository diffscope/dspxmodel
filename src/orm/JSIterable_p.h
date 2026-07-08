#ifndef DSPXMODEL_JSITERABLE_P_H
#define DSPXMODEL_JSITERABLE_P_H

#include <QJSValue>
#include <QObject>

namespace dspx {

    class JSIterable : public QObject {
        Q_OBJECT
    public:
        enum ContainerType { Sequence, List, Map, DataArray };
        Q_ENUM(ContainerType)

        explicit JSIterable(QObject *o, ContainerType type, QObject *parent = nullptr);
        QJSValue iterable() const;

    private:
        QObject *o;
        ContainerType type;
        mutable QJSValue iterable_;
    };

}

#endif // DSPXMODEL_JSITERABLE_P_H
