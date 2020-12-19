#include <iostream>

#include <pqxx/pqxx>

#include "DB_Foo.h"
#include "DB_Bar.h"

using std::cout;
using std::cerr;
using std::endl;
using std::string;

int main(int, char **) {
    try {
        pqxx::connection c;

        // This code shows how to do some of this manually.
        // It's here to help me write the DB generator.cpp.
        //pqxx::work work {c};
        //string username = "jpl10";
        //pqxx::result results = work.exec_params("INSERT INTO foo (username) VALUES ($1) RETURNING id", username);
        //work.commit();
        //pqxx::row row = results[0];
        //cout << "Inserted ID: " << row[0].as<int>() << endl;

        //pqxx::work work {c};
        //pqxx::result results = work.exec("SELECT id, username FROM foo ORDER BY id");
        //work.commit();
        //for (pqxx::row row: results) {
        //    cout << "Got a row ID: " << row[0].as<int>() << ". Username: " << row[1].as<string>() << endl;
        //}

        Foo foo;
        foo.setUsername("test");
        DB_Foo::update(c, foo);

        cout << "After inserting a Foo, ID: " << foo.getId() << ". Username: " << foo.getUsername() << endl;

        foo.setUsername("test again");
        cout << "After updating a Foo, ID: " << foo.getId() << ". Username: " << foo.getUsername() << endl;
        DB_Foo::update(c, foo);

        Foo::Vector allFoo = DB_Foo::readAll(c);
        cout << "Number of Foos: " << allFoo.size() << endl;
        for (Foo::Pointer foo: allFoo) {
            cout << "Id: " << foo->getId() << " == " << foo->getUsername() << endl;
        }

        Bar::Vector allBar = DB_Bar::readAll(c);
    }
    catch (std::exception const &e)
    {
        cerr << e.what() << endl;
        return 1;
    }

}
