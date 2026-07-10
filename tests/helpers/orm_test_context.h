#ifndef DSPXMODEL_TESTS_ORM_TEST_CONTEXT_H
#define DSPXMODEL_TESTS_ORM_TEST_CONTEXT_H

#include <functional>
#include <utility>

#include <QtTest/QtTest>

#include <dini/engine.h>
#include <dini/transaction.h>

#include <dspxmodelCore/Document.h>
#include <dspxmodelORM/EntityObject.h>
#include <dspxmodelORM/Model.h>

class OrmTestContext {
public:
    dspx::Document document;
    dspx::Model model {&document};

    template <typename Func>
    void withTransaction(Func &&func) {
        auto transaction = document.engine()->beginTransaction();
        document.setTransaction(&transaction);
        try {
            std::invoke(std::forward<Func>(func));
            transaction.commit();
            document.setTransaction(nullptr);
        } catch (...) {
            document.setTransaction(nullptr);
            throw;
        }
    }

    void verifyEntity(dspx::EntityObject *entity) {
        QVERIFY(entity != nullptr);
        QCOMPARE(entity->model(), &model);
        QVERIFY(entity->handle());
    }
};

#endif // DSPXMODEL_TESTS_ORM_TEST_CONTEXT_H
