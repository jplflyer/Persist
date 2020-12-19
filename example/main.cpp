#include <iostream>

#include <pqxx/pqxx>

#include <faker/Name.h>

#include "DB_Foo.h"
#include "DB_Bar.h"

using std::cout;
using std::cerr;
using std::endl;
using std::string;

int main(int, char **) {
    try {
        pqxx::connection c;

        Foo foo;
        foo.setUsername(Faker::Name::name());
        DB_Foo::update(c, foo);

        cout << "After inserting a Foo, ID: " << foo.getId() << ". Username: " << foo.getUsername() << endl;

        foo.setUsername(Faker::Name::name());
        DB_Foo::update(c, foo);

        Foo foo2;
        foo2.setUsername(Faker::Name::name());
        DB_Foo::update(c, foo2);

        Foo::Vector allFoo = DB_Foo::readAll(c);
        cout << "Number of Foos: " << allFoo.size() << endl;
        int idToDel = 0;
        for (Foo::Pointer foo: allFoo) {
            cout << "Id: " << foo->getId() << " == " << foo->getUsername() << endl;
            if (idToDel == 0) {
                idToDel = foo->getId();
            }
        }

        if (idToDel != 0) {
            DB_Foo::deleteWithId(c, idToDel);
        }
        allFoo = DB_Foo::readAll(c);
        cout << "Number of Foos after delete: " << allFoo.size() << endl;

        Bar::Vector allBar = DB_Bar::readAll(c);
    }
    catch (std::exception const &e)
    {
        cerr << e.what() << endl;
        return 1;
    }

}
