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
CodeGenerator_DB::CodeGenerator_DB()
    : CodeGenerator("CodeGenerator_DB")
{
}

/**
 * Generate the code. We create one DB_Foo class for each table in the model.
 */
void
CodeGenerator_DB::generate(DataModel &model) {
    if (outputFileName.length() == 0) {
        cerr << "CodeGenerator_DB::generate() with no output directory specified." << endl;
        exit(2);
    }

    for (const Table::Pointer & table: model.getTables()) {
        generateH(*table);
        generateCPP(*table);
    }
}

/**
 * Generate the .h file.
 */
void
CodeGenerator_DB::generateH(Table &table) {
    string baseClassName = table.getName();
    string myClassName = string{"DB_"} + baseClassName;
    string hName = outputFileName + "/" + myClassName + ".h";
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
    // Opening.
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
        << endl
           ;

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

    //--------------------------------------------------
    // And close it out.
    //--------------------------------------------------
    ofs << "};" << endl;
}

/**
 * Generate the .cpp file.
 */
void
CodeGenerator_DB::generateCPP(Table &table) {
    string baseClassName = table.getName();
    string myClassName = string{"DB_"} + baseClassName;
    string cName = outputFileName + "/" + myClassName + ".cpp";
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
    generateCPP_ReadAll(table, ofs);
    generateCPP_ParseAll(table, ofs);
    generateCPP_ParseOne(table, ofs);

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
    generateCPP_DoInsert(table, ofs);
    generateCPP_DoUpdate(table, ofs);
}

/**
 * This generates the readAll method, which does the query and then gets the
 * other methods to parse it.
 */
void CodeGenerator_DB::generateCPP_ReadAll(DataModel::Table &table, std::ostream &ofs) {
    string baseClassName = table.getName();
    string myClassName = string{"DB_"} + baseClassName;
    const Column::Pointer pk = table.findPrimaryKey();
    string pkGetter { string{"get"} + firstUpper(pk->getName()) + "()"};

    ofs << baseClassName << "::Vector " << myClassName << "::readAll(pqxx::connection &conn, std::string whereClause) {" << endl
        << "\tpqxx::work work(conn);" << endl
        << "\tpqxx::result results = work.exec( string{\"SELECT \"} + QUERY_LIST + \" FROM " << table.getDbName()
            << '"' << " + (whereClause.length() > 0 ? ( string{\"WHERE \"} + whereClause ): \"\""
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
void CodeGenerator_DB::generateCPP_ParseAll(DataModel::Table &table, std::ostream &ofs) {
    string baseClassName = table.getName();
    string myClassName = string{"DB_"} + baseClassName;
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
void CodeGenerator_DB::generateCPP_ParseOne(DataModel::Table &table, std::ostream &ofs) {
    string baseClassName = table.getName();
    string myClassName = string{"DB_"} + baseClassName;
    const Column::Pointer pk = table.findPrimaryKey();
    string pkGetter { string{"get"} + firstUpper(pk->getName()) + "()"};

    ofs << baseClassName << "::Pointer " << myClassName << "::parseOne(pqxx::row &row) {" << endl
        << "\t" << baseClassName << "::Pointer ptr = std::make_shared<" << baseClassName << ">();" << endl
           ;

    int index = 0;
    for (const Column::Pointer &column: table.getColumns()) {
        string cType = cTypeFor(column->getDataType());

        ofs << "\tptr->set" << firstUpper(column->getName())
            << "( row[" << index++ << "].as<" << cType << ">());" << endl;
           ;
    }

    ofs
        << "\treturn ptr;" << endl
        << "}" << endl
        << endl
       ;
}

/**
 * This writes the doInsert method.
 */
void CodeGenerator_DB::generateCPP_DoInsert(Table &table, std::ostream &ofs) {
    string baseClassName = table.getName();
    string myClassName = string{"DB_"} + baseClassName;
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

    for (const Column::Pointer &column: table.getColumns()) {
        if (!column->getIsPrimaryKey()) {
            ofs << ", obj.get" << firstUpper(column->getName()) << "()";
        }
    }

    ofs << ");" << endl
        << "\twork.commit();" << endl
        << "\tobj.set" << firstUpper(pk->getName()) << "(results[0][0].as<int>()" << ");" << endl
        << "}" << endl
        << endl
           ;
}

/**
 * This writes the doUpdate() method, which is similar to the doInsert() method,
 * but of course, SQL UPDATE statements aren't identical to INSERT statements.
 */
void CodeGenerator_DB::generateCPP_DoUpdate(Table &table, std::ostream &ofs) {
    string baseClassName = table.getName();
    string myClassName = string{"DB_"} + baseClassName;
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

    for (const Column::Pointer &column: table.getColumns()) {
        if (!column->getIsPrimaryKey()) {
            ofs << ", obj.get" << firstUpper(column->getName()) << "()";
        }
    }

    ofs << ");" << endl
        << "\twork.commit();" << endl
        << "}" << endl
        << endl
           ;
}
