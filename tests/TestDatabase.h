#pragma once

#include <memory>
#include <pqxx/pqxx>

#include "UnitTesting.h"

class TestDatabase: public CppUnit::TestFixture
{
    CPPUNIT_TEST_SUITE(TestDatabase);
    CPPUNIT_TEST(testString);
    CPPUNIT_TEST(testDate);
    CPPUNIT_TEST_SUITE_END();

public:
    void testString();
    void testDate();

private:
    void connect();

    std::shared_ptr<pqxx::connection> connection = nullptr;
};

