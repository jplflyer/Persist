#pragma once

#include "DataModel.h"
#include "UnitTesting.h"

/**
 * Unit testing of DataModel.
 */
class TestDataModel: public CppUnit::TestFixture
{
    CPPUNIT_TEST_SUITE(TestDataModel);
    CPPUNIT_TEST(testBasic);
    CPPUNIT_TEST_SUITE_END();

public:
    void testBasic();
};

