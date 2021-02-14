//
// The code generator for the SQL classes. Like the model objects, we
// create both a base class and a concrete subclass. The subclass will
// be empty, but this is where you could put additional hand-written
// methods.
//
#include <iostream>
#include <fstream>

#include <showlib/StringUtils.h>

#include "CodeGenerator_DB.h"

using namespace ShowLib;

using std::cout;
using std::cerr;
using std::endl;
using std::string;

using Table = DataModel::Table;
using Column = DataModel::Column;
using DataType = DataModel::Column::DataType;

/**
 * Constructor.
 */
CodeGenerator_DB::CodeGenerator_DB(DataModel &m)
    : CodeGenerator("CodeGenerator_DB", m)
{
}

/**
 * Generate the code. We create one DB_Foo class for each table in the model.
 */
void
CodeGenerator_DB::generate() {
    if (outputFileName.length() == 0) {
        cerr << "CodeGenerator_DB::generate() with no output directory specified." << endl;
        exit(2);
    }

    for (const Table::Pointer & table: model.getTables()) {
        generateH(*table);
        generateCPP(*table);
        generateConcreteH(*table);
        generateConcreteCPP(*table);
    }
}

//======================================================================
// Methods for creating the base classes in the stub dir.
//======================================================================

/**
 * Generate the .h file.
 */
void
CodeGenerator_DB::generateH(Table &table) {
    string baseClassName = table.getName();
    string myClassName = string{"DB_"} + baseClassName + "_Base";
    string hName = cppStubDirName + "/" + myClassName + ".h";
    std::ofstream ofs{hName};

    //--------------------------------------------------
    // Header portion.
    //--------------------------------------------------
    ofs << "#pragma once" << endl
        << endl
        << "#include <string>" << endl
        << "#include <vector>" << endl
        << "#include <memory>" << endl
        << endl
        << "#include <pqxx/pqxx>" << endl
        << endl
        << "#include \"" << baseClassName << ".h\"" << endl;
            ;

    //--------------------------------------------------
    // Opening. This defines the beginning of the class
    // plus a bunch of standard methods.
    //--------------------------------------------------
    ofs << "class " << myClassName << " {" << endl
        << "private:" << endl
        << "\t/** Read all rows from query results. */" << endl
        << "\tstatic " << baseClassName << "::Vector parseAll(pqxx::result &);"  << endl
        << endl
        << "\t/** Read one row from query results. */" << endl
        << "\tstatic " << baseClassName << "::Pointer parseOne(pqxx::row &);"  << endl
        << endl
        << "\t/** Insert one row. */" << endl
        << "\tstatic void doInsert(pqxx::connection &, " << baseClassName << " &);"  << endl
        << endl
        << "\t/** Update one row. */" << endl
        << "\tstatic void doUpdate(pqxx::connection &, " << baseClassName << " &);"  << endl
        << endl
        << "public:" << endl
        << "\tstatic " << baseClassName << "::Vector readAll(pqxx::connection &, std::string whereClause = \"\");"  << endl
        << "\tstatic void update(pqxx::connection &, " << baseClassName << " &);"  << endl
        << "\tstatic void deleteWithId(pqxx::connection &, int);"  << endl
        << endl
           ;

    //--------------------------------------------------
    // We also want readers based on having other
    // tables pointed to us.
    //--------------------------------------------------
    generateH_FromForeignKeys(table, ofs, myClassName);
    generateH_FromMapFiles(table, ofs, myClassName);

    //--------------------------------------------------
    // Write a constexpr that holds the list of columns
    // we'll query in any queries we write. We use a
    // second one for inserts that skips the PK.
    //--------------------------------------------------
    string delim {""};

    ofs << "\tstatic constexpr char const * QUERY_LIST { " << '"';
    for (const Column::Pointer &column: table.getColumns()) {
        ofs << delim << column->getDbName();
        delim = ", ";
    }
    ofs << '"' << " };" << endl;

    delim = "";
    ofs << "\tstatic constexpr char const * INSERT_LIST { " << '"';
    for (const Column::Pointer &column: table.getColumns()) {
        if (!column->getIsPrimaryKey()) {
            ofs << delim << column->getDbName();
            delim = ", ";
        }
    }
    ofs << '"' << " };" << endl;

    delim = "";
    ofs << "\tstatic constexpr char const * QUALIFIED_QUERY_LIST { " << '"';
    for (const Column::Pointer &column: table.getColumns()) {
        ofs << delim << table.getDbName() << "." << column->getDbName();
        delim = ", ";
    }
    ofs << '"' << " };" << endl;

    //--------------------------------------------------
    // And close it out.
    //--------------------------------------------------
    ofs << "};" << endl;
}

/**
 * We want "readAll" style methods for any foreign keys we hold. For instance, assume:
 *
 * 		Foo.id
 * 		Bar.fooId -> Foo.id
 *
 * We want a readAll_ForFoo(connection, foo.id) to return all Bars that belong to Foo.
 */
void
CodeGenerator_DB::generateH_FromForeignKeys(Table &table, std::ostream &ofs, const std::string &) {
    string baseClassName = table.getName();

    for (const Column::Pointer &column: table.getColumns()) {
        Column::Pointer fk = column->getReferences();
        if (fk != nullptr) {
            std::shared_ptr<Table> refTable = fk->getOurTable().lock();
            ofs << "\tstatic " << baseClassName << "::Vector readAll_For" << refTable->getName() << "(pqxx::connection &, int);"  << endl;
        }
    }
}

/**
 * Map tables provide many-to-many relationships. Imagine:
 *
 * Foo.id
 * Bar.id
 * Foo_Bar.fooId and Foo_Bar.barId
 *
 * If we have a Foo, we might want all Bars, which is a join against the Foo_Bar table.
 *
 * Map Tables probably have exactly three columns -- the two listed above plus a primary
 * key. What I'll need to do is see if another table looks like a map table mapped to
 * this table. If so, then I'll need the reference for the OTHER table (what we're
 * mapping from) and create a readAll method from that table's PK.
 */
void
CodeGenerator_DB::generateH_FromMapFiles(Table &table, std::ostream &ofs, const std::string &myClassName) {
    string baseClassName = table.getName();

    for (const Table::Pointer &thisTable: model.getTables()) {
        if (thisTable->getName() != table.getName() && thisTable->looksLikeMapTableFor(table)) {
            const Column::Pointer otherRef = thisTable->otherMapTableReference(table);
            if (otherRef == nullptr) {
                continue;
            }

            // We're just writing the signature:
            //    vec readAll_FromMap_<MapTable>(pqxx::connection &, int otherId)
            ofs << "\tstatic " << baseClassName << "::Vector readAll_FromMap_"
                << thisTable->getName() << "(pqxx::connection &, int "
                << otherRef->getName() << ");"  << endl;
        }
    }
}

/**
 * Generate the .cpp file.
 */
void
CodeGenerator_DB::generateCPP(Table &table) {
    string baseClassName = table.getName();
    string myClassName = string{"DB_"} + baseClassName + "_Base";
    string cName = cppStubDirName + "/" + myClassName + ".cpp";
    std::ofstream ofs{cName};
    const Column::Pointer pk = table.findPrimaryKey();

    ofs << "#include <iostream>" << endl
        << endl
        << "#include \"" << myClassName << ".h\"" << endl
        << endl
        << "using std::string;" << endl
        << endl
       ;

    //--------------------------------------------------
    // Generate the reader methods.
    //--------------------------------------------------
    generateCPP_ReadAll(table, ofs, myClassName);
    generateCPP_ParseAll(table, ofs, myClassName);
    generateCPP_ParseOne(table, ofs, myClassName);

    generateCPP_FromForeignKeys(table, ofs, myClassName);
    generateCPP_FromMapFiles(table, ofs, myClassName);

    //--------------------------------------------------
    // Write the add-or-update method.
    //--------------------------------------------------
    string getterName { string{""} +  firstUpper(pk->getName()) };

    ofs << "void " << myClassName << "::update(pqxx::connection &conn, " << baseClassName << " &obj) {"  << endl
        << "\tif (obj.get" << getterName << "() == 0) {" << endl
        << "\t\tdoInsert(conn, obj);" << endl
        << "\t}" << endl
        << "\telse {" << endl
        << "\t\tdoUpdate(conn, obj);" << endl
        << "\t}" << endl
        << "}" << endl
        << endl
           ;

    //--------------------------------------------------
    // Write the inserter and updater.
    //--------------------------------------------------
    generateCPP_DoInsert(table, ofs, myClassName);
    generateCPP_DoUpdate(table, ofs, myClassName);

    generateCPP_DeleteWithId(table, ofs, myClassName);
}

/**
 * This generates the readAll method, which does the query and then gets the
 * other methods to parse it.
 */
void CodeGenerator_DB::generateCPP_ReadAll(Table &table, std::ostream &ofs, const string &myClassName) {
    string baseClassName = table.getName();
    const Column::Pointer pk = table.findPrimaryKey();
    string pkGetter { string{"get"} + firstUpper(pk->getName()) + "()"};

    ofs << baseClassName << "::Vector " << myClassName << "::readAll(pqxx::connection &conn, std::string whereClause) {" << endl
        << "\tpqxx::work work(conn);" << endl
        << "\tpqxx::result results = work.exec( string{\"SELECT \"} + QUERY_LIST + \" FROM " << table.getDbName()
            << '"' << " + (whereClause.length() > 0 ? ( string{\" WHERE \"} + whereClause ): \"\""
            << "));" << endl
        << "\twork.commit();" << endl
        << "\t" << baseClassName << "::Vector vec = parseAll(results);" << endl
        << "\treturn vec;" << endl
        << "}" << endl
        << endl
       ;
}

/**
 * This parses the results of a query.
 */
void CodeGenerator_DB::generateCPP_ParseAll(Table &table, std::ostream &ofs, const string &myClassName) {
    string baseClassName = table.getName();
    const Column::Pointer pk = table.findPrimaryKey();
    string pkGetter { string{"get"} + firstUpper(pk->getName()) + "()"};

    ofs << baseClassName << "::Vector " << myClassName << "::parseAll(pqxx::result &results) {" << endl
        << "\t" << baseClassName << "::Vector vec;" << endl
        << "\tfor (pqxx::row row: results) {" << endl
        << "\t\tvec.push_back(parseOne(row));" << endl
        << "\t}" << endl
        << "\treturn vec;" << endl
        << "}" << endl
        << endl
       ;
}

/**
 * This parses the results of one row.
 */
void CodeGenerator_DB::generateCPP_ParseOne(Table &table, std::ostream &ofs, const string &myClassName) {
    string baseClassName = table.getName();
    const Column::Pointer pk = table.findPrimaryKey();
    string pkGetter { string{"get"} + firstUpper(pk->getName()) + "()"};

    ofs << baseClassName << "::Pointer " << myClassName << "::parseOne(pqxx::row &row) {" << endl
        << "\t" << baseClassName << "::Pointer ptr = std::make_shared<" << baseClassName << ">();" << endl
           ;

    int index = 0;
    for (const Column::Pointer &column: table.getColumns()) {
        string cType = cTypeFor(column->getDataType());

        if (column->isString() || column->isDate() || column->isTimestamp()) {
            ofs << "\tptr->set" << firstUpper(column->getName())
                << "( row[" << index << "].is_null() ? \"\" : row[" << index << "].as<" << cType << ">());" << endl;
               ;
        }
        else {
            ofs << "\tptr->set" << firstUpper(column->getName())
                << "( row[" << index << "].as<" << cType << ">());" << endl;
               ;
        }
        ++index;
    }

    ofs
        << "\treturn ptr;" << endl
        << "}" << endl
        << endl
       ;
}

/**
 * This generates any readers based on having FK relationships.
 */
void
CodeGenerator_DB::generateCPP_FromForeignKeys(Table &table, std::ostream &ofs, const string &myClassName) {
    string baseClassName = table.getName();

    for (const Column::Pointer &column: table.getColumns()) {
        Column::Pointer fk = column->getReferences();
        if (fk != nullptr) {
            std::shared_ptr<Table> refTable = fk->getOurTable().lock();
            ofs << baseClassName << "::Vector " << myClassName << "::readAll_For" << refTable->getName()
                << "(pqxx::connection &conn, int " << column->getName() << ") {"  << endl
                << "\tpqxx::work work(conn);" << endl
                << "\tpqxx::result results = work.exec( string{\"SELECT \"} + QUERY_LIST + \" FROM " << table.getDbName()
                << " WHERE " << column->getDbName() << " = \" + std::to_string(" << column->getName() << "));" << endl
                << "\twork.commit();" << endl
                << "\t" << baseClassName << "::Vector vec = parseAll(results);" << endl
                << "\treturn vec;" << endl
               ;
            ofs << "}" << endl << endl;
        }
    }
}

/**
 * This generates readers based on map tables, which
 * a join, and is complicated to write, even though the SQL
 * really is pretty trivial.
 */
void
CodeGenerator_DB::generateCPP_FromMapFiles(Table &table, std::ostream &ofs, const string &myClassName) {
    for (const Table::Pointer &thisTable: model.getTables()) {
        if (thisTable->getName() != table.getName() && thisTable->looksLikeMapTableFor(table)) {
            const Column::Pointer otherRef = thisTable->otherMapTableReference(table);
            if (otherRef == nullptr) {
                continue;
            }
            generateCPP_ThisMap(table, ofs, *thisTable, myClassName);
        }
    }
}

/**
 * Generates a join.
 *
 *    SELECT foo.* FROM foo, bar WHERE foo.id = bar.foo_id AND bar.gleep_id = 10
 */
void
CodeGenerator_DB::generateCPP_ThisMap(
    Table &table,			// The table we're returning
    std::ostream &ofs,					// Output stream
    Table &mapTable,			// The mapping table
    const std::string &myClassName)
{
    string baseClassName = table.getName();
    Column::Pointer colToUs = mapTable.ourMapTableReference(table);
    Column::Pointer ourKeyColumn = colToUs->getReferences();

    Column::Pointer colToThem = mapTable.otherMapTableReference(table);

    ofs << baseClassName << "::Vector " << myClassName << "::readAll_FromMap_"
        << mapTable.getName() << "(pqxx::connection &conn, int " << colToThem->getName() << ") {"  << endl
        << "\tpqxx::work work(conn);" << endl
        << "\tpqxx::result results = work.exec( string{\"SELECT \"} + QUALIFIED_QUERY_LIST + \" FROM "
        << table.getDbName() << ", " << mapTable.getDbName()

        // This is the join to us
        << " WHERE " << table.getDbName() << "." << ourKeyColumn->getDbName() << " = "
        << mapTable.getDbName() << "." << colToUs->getDbName()

        // And this is the join from the distant table, which is from the method's argument list
        << " AND " << mapTable.getDbName() << "." << colToThem->getDbName() << " = "
        << "\" + std::to_string(" << colToThem->getName() << "));" << endl

        << "\twork.commit();" << endl
        << "\t" << baseClassName << "::Vector vec = parseAll(results);" << endl
        << "\treturn vec;" << endl
       ;
    ofs << "}" << endl << endl;
;
}

/**
 * This writes the doInsert method.
 */
void CodeGenerator_DB::generateCPP_DoInsert(Table &table, std::ostream &ofs, const string &myClassName) {
    string baseClassName = table.getName();
    const Column::Pointer pk = table.findPrimaryKey();
    string pkGetter { string{"get"} + firstUpper(pk->getName()) + "()"};

    ofs << "void " << myClassName << "::doInsert(pqxx::connection &conn, " << baseClassName << " &obj) {"  << endl
        << "\tpqxx::work work {conn};" << endl
        << "\tstring sql { string{\"INSERT INTO " << table.getDbName()
            << " (\"} + INSERT_LIST + \")"
            << " VALUES ("
               ;

    string delim {""};
    int index = 1;
    for (const Column::Pointer &column: table.getColumns()) {
        if (!column->getIsPrimaryKey()) {
            ofs << delim << "$" << index++;
            delim = ", ";
        }
    }

    ofs << ") RETURNING " << pk->getDbName() << "\" };" << endl
        << "\tpqxx::result results = work.exec_params(sql";

    generateCPP_FieldArguments(table, ofs);

    ofs << ");" << endl
        << "\twork.commit();" << endl
        << "\tobj.set" << firstUpper(pk->getName()) << "(results[0][0].as<int>()" << ");" << endl
        << "}" << endl
        << endl
           ;
}

/**
 * This is used to do the latter part of work.exec_params().
 */
void
CodeGenerator_DB::generateCPP_FieldArguments(DataModel::Table &table, std::ostream &ofs) {
    for (const Column::Pointer &column: table.getColumns()) {
        if (!column->getIsPrimaryKey()) {
            string getterStr = string{"obj.get"} + firstUpper(column->getName()) + "()";
            if (column->isString() || column->isDate() || column->isTimestamp()) {
                ofs << ",\n\t\t" << getterStr << ".length() > 0" << " ? "
                    << getterStr << ".c_str() : nullptr"
                    ;
            }
            else {
                ofs << ",\n\t\t" << getterStr;
            }
        }
    }
}

/**
 * This writes the doUpdate() method, which is similar to the doInsert() method,
 * but of course, SQL UPDATE statements aren't identical to INSERT statements.
 */
void CodeGenerator_DB::generateCPP_DoUpdate(Table &table, std::ostream &ofs, const string &myClassName) {
    string baseClassName = table.getName();
    const Column::Pointer pk = table.findPrimaryKey();
    string pkGetter { string{"get"} + firstUpper(pk->getName()) + "()"};

    ofs << "void " << myClassName << "::doUpdate(pqxx::connection &conn, " << baseClassName << " &obj) {"  << endl
        << "\tpqxx::work work {conn};" << endl
        << "\tstring sql { \"UPDATE " << table.getDbName() << " SET "
           ;

    string delim {""};
    int index = 2;
    for (const Column::Pointer &column: table.getColumns()) {
        if (!column->getIsPrimaryKey()) {
            ofs << delim << column->getDbName() << " = $" << index++;
            delim = ", ";
        }
    }

    ofs << " WHERE " << pk->getDbName() << " = $1\" };" << endl
        << "\tpqxx::result results = work.exec_params(sql, obj." << pkGetter
           ;

    generateCPP_FieldArguments(table, ofs);

    ofs << ");" << endl
        << "\twork.commit();" << endl
        << "}" << endl
        << endl
           ;
}

/**
 * This writes deleteWithId().
 */
void CodeGenerator_DB::generateCPP_DeleteWithId(Table &table, std::ostream &ofs, const string &myClassName) {
    string baseClassName = table.getName();
    const Column::Pointer pk = table.findPrimaryKey();
    string pkGetter { string{"get"} + firstUpper(pk->getName()) + "()"};

    ofs << "void " << myClassName << "::deleteWithId(pqxx::connection &conn, int id) {"  << endl
        << "\tpqxx::work work {conn};" << endl
        << "\twork.exec_params(\"DELETE FROM " << table.getDbName() << " WHERE " << pk->getDbName() << " = $1\", id);" << endl
        << "\twork.commit();" << endl
        << "}" << endl
           ;
}

//======================================================================
// Classes for generating the concrete classes, if necessary.
//======================================================================

/**
 * Generate the concrete base class.h if it doesn't exist.
 */
void CodeGenerator_DB::generateConcreteH(Table &table)
{
    string myClassName = string{"DB_"} + table.getName();
    string baseClassName = myClassName + "_Base";
    string hName = outputFileName + "/" + myClassName + ".h";

    if (!std::filesystem::exists(hName)) {
        std::ofstream ofs{hName};

        ofs << "#pragma once" << endl
            << endl
            << "#include <iostream>" << endl
            << "#include <string>" << endl
            << "#include <" << baseClassName << ".h>" << endl
            << endl
            << "class " << myClassName << ": public " << baseClassName << " {" << endl
            << "public:" << endl
            << "\tvirtual ~" << myClassName << "();" << endl
            << "};" << endl
               ;
    }
}

/**
 * Generate the concrete base class.cpp if it doesn't exist.
 */
void CodeGenerator_DB::generateConcreteCPP(Table &table)
{
    string myClassName = string{"DB_"} + table.getName();
    string cppName = outputFileName + "/" + myClassName + ".cpp";

    if (!std::filesystem::exists(cppName)) {
        std::ofstream ofs{cppName};
        ofs << "#include <" << myClassName << ".h>" << endl
            << endl
            << myClassName << "::~" << myClassName << "() {" << endl
            << "}" << endl
               ;
    }
}
