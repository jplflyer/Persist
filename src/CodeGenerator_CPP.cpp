#include <iostream>
#include <fstream>
#include <filesystem>

#include <showlib/StringUtils.h>

#include "CodeGenerator_CPP.h"

using namespace ShowLib;

using std::cout;
using std::cerr;
using std::endl;
using std::string;

using Table = DataModel::Table;
using Column = DataModel::Column;
using DataType = DataModel::Column::DataType;

CodeGenerator_CPP::CodeGenerator_CPP()
    : CodeGenerator("CodeGenerator_CPP")
{
}

/**
 * Generate all .cpp and .h files.
 */
void
CodeGenerator_CPP::generate(DataModel &model) {
    if (cppStubDirName.length() == 0) {
        cerr << "CodeGenerator_CPP::generate() with no output directory specified." << endl;
        exit(2);
    }

    for (const Table::Pointer & table: model.getTables()) {
        generateH(*table);
        generateCPP(*table);

        generateConcreteH(*table);
        generateConcreteCPP(*table);
    }
}

/**
 * Generate the .h file.
 */
void
CodeGenerator_CPP::generateH(Table &table) {
    string name = table.getName();
    string myClassName = name + "_Base";
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
            ;

    if (wantJSON) {
        ofs << "#include <showlib/JSONSerializable.h>" << endl;
        ofs << endl;
    }

    //--------------------------------------------------
    // Foreign reference includes.
    //
    // It is unlikely we'll double-reference a table.
    // We might chain-reference, but the pragma once
    // will protect us.
    //--------------------------------------------------
    bool didRefs = false;
    for (const Column::Pointer &column: table.getColumns()) {
        const Column::Pointer ref = column->getReferences();
        if (ref != nullptr) {
            Table::Pointer refTable = ref->getOurTable().lock();
            ofs << "#include \"" << refTable->getName() << ".h\"" << endl;
            didRefs = true;
        }
    }
    if (didRefs) {
        ofs << endl;
    }

    //--------------------------------------------------
    // Opening.
    //--------------------------------------------------
    ofs << "class " << name << ";" << endl
        << endl
        << "class " << myClassName
          ;

    if (wantJSON) {
        ofs << ": public ShowLib::JSONSerializable";
    }

    //--------------------------------------------------
    // Some typedefs.
    //--------------------------------------------------
    ofs << " {" << endl
        << "public:" << endl
        << "    typedef std::shared_ptr<" << name << "> Pointer;" << endl
        << "    typedef std::weak_ptr<" << name << "> WPointer;" << endl
        << "    typedef std::vector<Pointer> Vector;" << endl
        << endl
        << "	virtual ~" << myClassName << "();" << endl
           ;

    //--------------------------------------------------
    // Getters and setters.
    //--------------------------------------------------

    for (const Column::Pointer &column: table.getColumns()) {
        string upperName = firstUpper(column->getName());
        string cType = cTypeFor(column->getDataType());
        bool isStr = isString(cType);
        string constness { isStr ? "const " : "" };
        string refness { isStr ? " &" : "" };
        string ns { isStr ? "std::" : "" };

        ofs << "    " << constness << ns << cType << refness << " get" << upperName
            << "() const { return " << column->getName() << "; }" << endl;
           ;

        ofs << "    " << myClassName << " & set" << upperName
            << " (" << constness << ns << cType << refness << " value)"
            << " { " << column->getName() << " = value; return *this; }"
            << endl;
           ;
    }

    //--------------------------------------------------
    // JSON methods.
    //--------------------------------------------------
    if (wantJSON) {
        ofs << endl;
        ofs << "    void fromJSON(const JSON &) override;" << endl;
        ofs << "    JSON toJSON(JSON &) const override;" << endl;
    }

    //--------------------------------------------------
    // Fields.
    //--------------------------------------------------
    ofs << endl;
    ofs << "private:" << endl;

    for (const Column::Pointer &column: table.getColumns()) {
        string cType = cTypeFor(column->getDataType());
        bool isStr = isString(cType);
        string ns { isStr ? "std::" : "" };

        ofs << "    " << ns << cType << " " << column->getName();
        if (isInt(cType)) {
            ofs << " = 0";
        }
        if (isDouble(cType)) {
            ofs << " = 0.0";
        }
        if (isBool(cType)) {
            ofs << " = false";
        }
        ofs << ";" << endl;
    }

    //--------------------------------------------------
    // And close it out.
    //--------------------------------------------------
    ofs << "};" << endl;
}

/**
 * Generate the .cpp file.
 */
void
CodeGenerator_CPP::generateCPP(Table &table) {
    string name = table.getName();
    string myClassName = name + "_Base";
    string cppName = cppStubDirName + "/" + myClassName + ".cpp";
    std::ofstream ofs{cppName};

    ofs << "#include <iostream>" << endl
        << endl
        << "#include \"" << myClassName << ".h\"" << endl
        << endl
        << "using std::string;" << endl
        << endl
        << myClassName << "::~" << myClassName << "() {" << endl
        << "}" << endl
        << endl
       ;

    //======================================================================
    // Generate fromJSON.
    //======================================================================
    ofs << "/**" << endl
        << " * Read from JSON." << endl
        << " */" << endl
        << "void " << myClassName << "::fromJSON(const JSON &json) {" << endl
           ;

    for (const Column::Pointer &column: table.getColumns()) {
        ofs << "    " << column->getName() << " = " << cTypeFor(column->getDataType()) << "Value("
            << "json, \"" << column->getName() << "\");"
            << endl ;
    }

    ofs << "}" << endl << endl;

    //======================================================================
    // Generate toJSON.
    //======================================================================
    ofs << "/**" << endl
        << " * Write to JSON." << endl
        << " */" << endl
        << "JSON " << myClassName << "::toJSON(JSON &json) const {" << endl
           ;

    for (const Column::Pointer &column: table.getColumns()) {
        ofs << "    json[\"" << column->getName() << "\"] = " << column->getName() << ";" << endl;
    }

    ofs << "    return json; " << endl
        << "}" << endl << endl;
}

/**
 * Generate the concrete base class.h if it doesn't exist.
 */
void CodeGenerator_CPP::generateConcreteH(DataModel::Table &table)
{
    string name = table.getName();
    string baseClassName = name + "_Base";
    string hName = outputFileName + "/" + name + ".h";

    if (!std::filesystem::exists(hName)) {
        std::ofstream ofs{hName};

        ofs << "#pragma once" << endl
            << endl
            << "#include <iostream>" << endl
            << "#include <string>" << endl
            << "#include <" << baseClassName << ".h>" << endl
            << endl
            << "class " << name << ": public " << baseClassName << " {" << endl
            << "public:" << endl
            << "\t~" << name << "();" << endl
            << "};" << endl
               ;
    }

}

/**
 * Generate the concrete base class.cpp if it doesn't exist.
 */
void CodeGenerator_CPP::generateConcreteCPP(DataModel::Table &table)
{
    string name = table.getName();
    string cppName = outputFileName + "/" + name + ".cpp";

    if (!std::filesystem::exists(cppName)) {
        std::ofstream ofs{cppName};
        ofs << "#include <" << name << ".h>" << endl
            << endl
            << name << "::~" << name << "() {" << endl
            << "}" << endl
               ;
    }
}

//----------------------------------------------------------------------
// Some methods for helping with data types.
//----------------------------------------------------------------------

bool CodeGenerator_CPP::isInt(const std::string &cType) {
    return cType == "long" || cType == "short" || cType == "int";
}

bool CodeGenerator_CPP::isDouble(const std::string &cType) {
    return cType == "double";
}

bool CodeGenerator_CPP::isBool(const std::string &cType) {
    return cType == "bool";
}
bool CodeGenerator_CPP::isString(const std::string &cType) {
    return cType == "string";
}
