#include "JSIterable_p.h"

#include <QJSEngine>

namespace dspx {

    JSIterable::JSIterable(QObject *o, ContainerType type, QObject *parent)
        : QObject(parent), o(o), type(type) {
    }

    QJSValue JSIterable::iterable() const {
        if (!iterable_.isUndefined()) {
            return iterable_;
        }
        auto engine = qjsEngine(o);
        if (!engine) {
            qFatal() << "iterable() can only be called from QML";
            return {};
        }

        const char *constructorName = nullptr;
        switch (type) {
            case Sequence: constructorName = "SequenceIterable"; break;
            case List: constructorName = "ListIterable"; break;
            case Map: constructorName = "MapIterable"; break;
            case DataArray: constructorName = "DataArrayIterable"; break;
        }

        auto iterableConstructor = engine->importModule(":/qt/qml/DiffScope/DspxModel/qml/iterable.mjs").property(constructorName);
        Q_ASSERT(iterableConstructor.isCallable());
        auto result = iterableConstructor.callAsConstructor({engine->fromVariant<QJSValue>(QVariant::fromValue(o))});
        if (result.isError()) {
            qFatal() << result.property("message").toString();
            return {};
        }
        iterable_ = result;
        return iterable_;
    }

}
