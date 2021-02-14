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

    createTable("universe", "id serial primary key, name text");
    createTable("series", "id serial primary key, name text, universe_id int references universe(id)");
    createTable("date_table", "id serial primary key, name text, start_date date");
}

void
TestDatabase::createTable(const string & name, const string & columnDef) {
    pqxx::work work {*connection};
    string sql =
        string{"SELECT EXISTS ( SELECT FROM information_schema.tables WHERE table_schema = 'public' AND table_name = '"}
        + name + "')";

    pqxx::result results = work.exec(sql);
    work.commit();

    bool exists = results[0][0].as<bool>();
    if (!exists) {
        pqxx::work create {*connection};
        sql = string{"CREATE TABLE "} + name + "(" + columnDef + ")";
        create.exec(sql);
        create.commit();
    }

}

void
TestDatabase::testString() {
    connect();
    string sql { "INSERT INTO universe(name) VALUES ($1) RETURNING id" };
    string filled { "This has a value" };
    string empty;

    pqxx::work workFilled {*connection};
    pqxx::result results = workFilled.exec_params(sql, filled.length() > 0 ? filled.c_str() : nullptr);
    workFilled.commit();

    pqxx::work workEmpty {*connection};
    results = workEmpty.exec_params(sql, empty.length() > 0 ? empty.c_str() : nullptr);
    workEmpty.commit();
}

void
TestDatabase::testDate() {
    connect();
    string sql { "INSERT INTO date_table(name, start_date) VALUES ($1, $2) RETURNING id" };

    pqxx::work work {*connection};
    pqxx::result results = work.exec_params(sql, "has date", "2021-02-14 12:00:00");
    work.commit();

    string emptyDate = "";
    pqxx::work workNull {*connection};
    results = workNull.exec_params(sql, "no date", emptyDate.length() > 0 ? emptyDate.c_str() : nullptr);
    workNull.commit();
}

/**
 * This test uses non-null foreign key references.
 */
void
TestDatabase::testGoodForeignKeys() {
    connect();
    string sqlUniverse { "INSERT INTO universe(name) VALUES ($1) RETURNING id" };
    string sqlSeries { "INSERT INTO series(name, universe_id) VALUES ($1, $2) RETURNING id" };

    pqxx::work populateUniverse {*connection};
    pqxx::result results = populateUniverse.exec_params(sqlUniverse, "Test Universe");
    populateUniverse.commit();

    int universeId = results[0][0].as<int>();

    pqxx::work populateSeries {*connection};
    results = populateSeries.exec_params(sqlSeries, "Test Series", universeId);
    populateSeries.commit();
}

void
TestDatabase::testNullForeignKeys() {
    connect();
    string sqlSeries { "INSERT INTO series(name, universe_id) VALUES ($1, nullif($2, 0)) RETURNING id" };

    int universeId = 0;

    pqxx::work populateSeries {*connection};
    pqxx::result results = populateSeries.exec_params(sqlSeries, "Test Series", universeId);
    populateSeries.commit();
}
