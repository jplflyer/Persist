#include <iostream>
#include <iomanip>

#include <pqxx/pqxx>

#include "TestDatabase.h"

using std::cout;
using std::endl;
using std::string;

string TEST_NAME = "TestDataModel";

CPPUNIT_TEST_SUITE_REGISTRATION(TestDatabase);

/**
 * Connect to the database and make sure the test table exists.
 */
void
TestDatabase::connect() {
    cout << std::boolalpha;

    if (connection == nullptr) {
        string url { "postgresql://jpl:nicknick@localhost:5432" };
        connection = std::make_shared<pqxx::connection>(url);
    }
    pqxx::work work {*connection};
    pqxx::result results = work.exec(
        "SELECT EXISTS ( SELECT FROM information_schema.tables WHERE table_schema = 'public' AND table_name = 'test')" );
    work.commit();

    bool exists = results[0][0].as<bool>();
    if (!exists) {
        pqxx::work create {*connection};
        create.exec("CREATE TABLE test (id serial, string_val text, date_val date)");
        create.commit();
    }
}

void
TestDatabase::testString() {
    connect();
    string sql { "INSERT INTO test(string_val) VALUES ($1) RETURNING id" };
    string filled { "This has a value" };
    string empty;

    pqxx::work workFilled {*connection};
    pqxx::result results = workFilled.exec_params(sql, filled.length() > 0 ? filled.c_str() : nullptr);
    workFilled.commit();

    cout << "Created ID " << results[0][0].as<int>() << endl;

    pqxx::work workEmpty {*connection};
    results = workEmpty.exec_params(sql, empty.length() > 0 ? empty.c_str() : nullptr);
    workEmpty.commit();

    cout << "Created ID " << results[0][0].as<int>() << endl;
}

void
TestDatabase::testDate() {
    connect();
    string sql { "INSERT INTO test(string_val, date_val) VALUES ($1, $2) RETURNING id" };

    pqxx::work work {*connection};
    pqxx::result results = work.exec_params(sql, "has date", "2021-02-14 12:00:00");
    work.commit();

    cout << "Created has date -- ID " << results[0][0].as<int>() << endl;

    string emptyDate = "";
    pqxx::work workNull {*connection};
    results = workNull.exec_params(sql, "no date", emptyDate.length() > 0 ? emptyDate.c_str() : nullptr);
    workNull.commit();

    cout << "Created no date -- ID " << results[0][0].as<int>() << endl;
}
