# Project Persist
This is the start of a C++ based persistence generator. From a data model, different generators can create:

* SQL to create tables and define relationships
* POCOs -- plain old C++ objects
* Database Access Methods

Ultimately, it would be great if there was a driver that could point to an existing database and apply updates to it rather than a simple "create from scratch" script. I'm not prepare to write that yet.

# License
This project uses the MIT license, attached in a separate file.

<div>Icons made by <a href="https://www.flaticon.com/authors/phatplus" title="phatplus">phatplus</a> from <a href="https://www.flaticon.com/" title="Flaticon">www.flaticon.com</a></div>

# Overall Concept
Imagine a typical web application. There's a client that runs on the browser. Most typically that's JavaScript + HTML + CSS. There's a server that handles requests. And there's some sort of database for persistence.

My view is of this:

* Client: C++ with Qt for WebAssembly
* Server: nginx for serving files plus a REST server in C++
* Database: PostgreSQL

# Requirements
This project is pure C++. Dependence upon external libraries will be kept minimal. Compilation should succeed on both Mac and Linux. No promises for Windows.

This is a Makefile-based app. I don't personally want to mess with CMake. If someone comes along who needs it, they can contribute to the project.

To build from source, you'll need showlib: https://github.com/jplflyer/ShowLib.git. Check that out and install per the README:

    make && sudo make install

My IDE is Qt Creator. See Qt.io. I check in the project files, but you can ignore them. I do not depend on any Qt libraries in this project.

I am targeting PostgreSQL. If you wish to target a different database, you can subclass the relevant classes for your own generators. The generated code assumes libpq and libpqxx.

# Build and Install
Assuming you have ShowLib already installed (see Requirements), check out, cd into the top of the directory, and:

    make && sudo make install

DataModeler will be copied into /usr/local/bin.

# Usage
See the example. You can:

    DataModeler --help

The DataModeler takes a JSON file as input:

    DataModeler --model FOO.json

You can create an empty one with:

    DataModeler --model FOO.json --create

You can add a table:

    DataModeler --model FOO.json --table Foo --column id,Serial --pk --column username,varchar(256)

Or you can just edit FOO.json with any other tool you prefer. When modifying the file in this fashion, you can chain as many --table and --column fields as you want, so you could specify the entire structure. Or you can do everything one at a time. Everything is an add-or-update. I haven't implemented a remove-column or remove-table command.

To generate code:

    DataModeler --model FOO.json --srcdir path-to-write-cpp-sources --generate

## Command Line Help

    DataModeler --help
    --create                   -- Create a DataModel file
    --generate                 -- Generate output
    --model fname              -- Specify the input/output data model file
    --srcdir dirname           -- Directory for .cpp files
    --table tablename          -- Create/Update this table
    --column columnname[,type] -- Create/Update this column
    --pk                       -- Mark column as a primery key.
    --notnull                  -- Mark column as NOT NULL.
    --ref table[.col]          -- Mark this foreign key reference.
    --help (-?)                -- Provide this help

Options like pk, notnull, and ref refer to the most recent column. Foreign key references should be something like:

    --table Bar --column foo_id,Integer --ref Foo.id

# What Gets Generated
At this point in time, you'll get Foo.h, Foo.cpp, DB_Foo.h, DB_Foo.cpp, and MODELNAME.sql. The last is the SQL commands to create an empty database. Currently, there's no support for incremental changes, so you might have to cut + paste into PSQL (or some other tool) if updating a live database.

Foo.h / Foo.cpp are plain old C++ objects. They know nothing about persistence. DB_Foo.h / DB_Foo.cpp provides the interface to libpq / libpqxx for you.

This is main.cpp from the example directory:

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

            Foo::Vector allFoo = DB_Foo::readAll(c);
            Bar::Vector allBar = DB_Bar::readAll(c);
        }
        catch (std::exception const &e)
        {
            cerr << e.what() << endl;
            return 1;
        }
    }

# Running the Examples
The example requires the following libs:

    ShowLib: https://github.com/jplflyer/ShowLib
    cpp-faker: https://github.com/jplflyer/cpp-faker

These are easy to build and install. Check them out and then for each:

    make && sudo make install

cpp-faker creates fake data. You don't need it for anything else (but it is kind of cool).

In addition, you'll need libpq and libpqxx.

Once you have the above, and you've also built DataModeler, then you can CD into the example directory and:

    make generate
    psql << db.json
    make
    bin/main

This assumes you know how psql works for connecting to a database. My example program doesn't attempt to do anything clever to connect to the database. It uses the same environment variables that psql does. If psql doesn't land at a SQL prompt, then main will probably fail, too.


# Using the generated code.
The default generators produce:

* Your foo.sql file
* Foo_Base.h and .cpp files in the stubdir
* Foo.h and .cpp in the CPP directory -- concrete subclasses

The two subdirectories can be the same or different.

The idea is that you may regenerate Foo_Base each time you have a model change. So you should put hand-written code into the subclasses and use them everywhere. Thus, you'll have:

    class Foo_Base: public JSON_Serializable { ... }
    class Foo: public Foo_Base { ... }

There is no reason you need to check into your source code system (git or whatever) the stub classes, unless you want to. They can be autogenerated, and they'll also generate identically if there are no model changes.
