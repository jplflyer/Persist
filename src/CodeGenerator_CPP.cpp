#include <iostream>
#include <fstream>
#include <filesystem>

#include <showlib/CommonUsing.h>
#include <showlib/StringUtils.h>

#include "CodeGenerator_CPP.h"

using namespace ShowLib;

using std::ostream;

using Table = DataModel::Table;
using Column = DataModel::Column;
using DataType = DataModel::Column::DataType;

CodeGenerator_CPP::CodeGenerator_CPP(DataModel &m)
    : CodeGenerator("CodeGenerator_CPP", m)
{
}

/**
 * Generate all .cpp and .h files.
 */
void
CodeGenerator_CPP::generate() {
    if (cppStubDirName.length() == 0) {
        cerr << "CodeGenerator_CPP::generate() with no output directory specified." << endl;
        exit(2);
    }

    generateIncludes();

    for (const Table::Pointer & table: model.getTables()) {
        generateH(*table);
        generateCPP(*table);

        generateConcreteH(*table);
        generateConcreteCPP(*table);
    }

    generateUtilities();
}

/**
 * This creates two big Includes, one for the objects, one for the DB helpers.
 */
void
CodeGenerator_CPP::generateIncludes() {
    string hName = outputFileName + "/" + model.getName() + ".h";
    string dbhName = outputFileName + "/DB_" + model.getName() + ".h";

    std::ofstream hOutput{hName};
    std::ofstream dbOutput{dbhName};

    for (const Table::Pointer &table: model.getTables()) {
        hOutput << "#include <" << cppIncludePath << table->getName() << ".h>" << endl;
        dbOutput << "#include <" << cppIncludePath << "DB_" << table->getName() << ".h>" << endl;
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

    generateH_ForwardReferences(ofs, table);

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
        << "    typedef ShowLib::JSONSerializableVector<" << name << "> Vector;" << endl
        << endl;

    //--------------------------------------------------
    // Constructors and destructor
    //--------------------------------------------------
    ofs << "    " << myClassName << "();" << endl
        << "    " << myClassName << "(const JSON &json);" << endl
        << "	virtual ~" << myClassName << "();" << endl
           ;

    //--------------------------------------------------
    // Getters and setters.
    //--------------------------------------------------

    ofs << endl << "\t// Getters and setters." << endl;
    for (const Column::Pointer &column: table.getColumns()) {
        string upperName = firstUpper(column->getName());
        string cType = cTypeFor(column->getDataType());
        bool isStr = isString(cType);
        string constness { isStr ? "const " : "" };
        string refness { isStr ? " &" : "" };
        string ns { isStr ? "std::" : "" };

        ofs << "    " << constness << ns << cType << refness << " get" << upperName
            << "() const { return " << column->getName() << "; }" << endl;

        ofs << "    " << myClassName << " & set" << upperName
            << " (" << constness << ns << cType << refness << " value)"
            << " { " << column->getName() << " = value; return *this; }"
            << endl;
    }
    generateH_FK_Access(ofs, table);

    //--------------------------------------------------
    // JSON methods.
    //--------------------------------------------------
    if (wantJSON) {
        ofs << endl;
        ofs << "    void fromJSON(const JSON &) override;" << endl;
        ofs << "    JSON toJSON() const override;" << endl;
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
    generateH_FK_Storage(ofs, table);

    //--------------------------------------------------
    // And close it out.
    //--------------------------------------------------
    ofs << "};" << endl << endl;

    //--------------------------------------------------
    // Finders.
    //--------------------------------------------------
    for (const Column::Pointer &col: table.getColumns()) {
        if (!col->getWantFinder()) {
            continue;
        }

        string colUpper = firstUpper(col->getName());
        string dataType = cTypeFor(col->getDataType());
        if (dataType == "string") {
            dataType = "std::string";
        }

        ofs << myClassName << "::Pointer find_By" << colUpper
            << "(" << myClassName << "::Vector & vec, const " << dataType << " & value);" << endl;
    }
}


void
CodeGenerator_CPP::generateH_ForwardReferences(std::ostream &ofs, DataModel::Table &table) {
    bool didRefs = false;

    // Do any FKs we contain.
    for (const Column::Pointer &column: table.getColumns()) {
        const Column::Pointer ref = column->getReferences();
        if (ref != nullptr) {
            Table::Pointer refTable = ref->getOurTable().lock();
            ofs << "class " << refTable->getName() << ";" << endl;
            didRefs = true;
        }
    }

    // Do any FKs to us. We'll assume we don't point back and forth.
    for (const Table::Pointer & otherTable: model.getTables()) {
        Column::Pointer ref = otherTable->ourMapTableReference(table);
        if (ref != nullptr) {
            ofs << "class " << otherTable->getName() << ";" << endl;
            didRefs = true;
        }
    }
}

/**
 * This generates the code for foreign key relationships.
 * This happens in two parts: things we have FKs for, and
 * tables that have FKs to us.
 */
void CodeGenerator_CPP::generateH_FK_Access(ostream &ofs, DataModel::Table &table) {

    ofs << endl << "\t// Foreign relationships." << endl;
    for (const Column::Pointer &col: table.getColumns()) {
        Column::Pointer ref = col->getReferences();
        if (ref != nullptr) {
            Table::Pointer refTable = ref->getOurTable().lock();
            string refPtrName = col->getRefPtrName();

            if (refPtrName.empty()) {
                refPtrName = firstLower(refTable->getName());
            }

            ofs << endl
                << "\tstd::shared_ptr<" << refTable->getName() << "> get" << refPtrName << "() const"
                << " { return " << refPtrName << "; }" << endl
                << "\tvoid set" << firstUpper(refPtrName) << "(std::shared_ptr<" << refTable->getName() << "> ptr)"
                << " { " << refPtrName << " = ptr; }" << endl
                ;
        }
    }

    //----------------------------------------------------------------------
    // This is for things that have FK relationships to us. We'll store
    // a vector and put getFoos and addFoo method.
    //----------------------------------------------------------------------
    for (const Table::Pointer & otherTable: model.getTables()) {
        Column::Pointer ref = otherTable->ourMapTableReference(table);
        if (ref != nullptr) {
            Table::Pointer refTable = ref->getOurTable().lock();
            ofs << endl
                << "	const ShowLib::JSONSerializableVector<" << refTable->getName() << "> & "
                << "get" << refTable->getName() << "s() const { return "
                << firstLower(refTable->getName()) << "Vector; }" << endl
                << "	void add" << refTable->getName() << "(const std::shared_ptr<" << refTable->getName() << ">);" << endl
                << "	void remove" << refTable->getName() << "(const std::shared_ptr<" << refTable->getName() << ">);" << endl
                << "	void removeAll" << refTable->getName() << "();" << endl
                   ;
        }
    }
}

/**
 * This generates the code for foreign key relationships. This is tricky as we can have
 * more than one ID into the same table. For instance, I have some role-based permissions,
 * and I have one table with "viewerRoleId" and "posterRoleId". If you only have a single
 * link to the Role table, you could call the column "Role::Pointer role". But clearly that
 * doesn't work if you have more than one.
 */
void CodeGenerator_CPP::generateH_FK_Storage(ostream &ofs, DataModel::Table &table) {
    for (const Column::Pointer &col: table.getColumns()) {
        Column::Pointer ref = col->getReferences();
        if (ref != nullptr) {
            Table::Pointer refTable = ref->getOurTable().lock();
            string refPtrName = col->getRefPtrName();

            if (refPtrName.empty()) {
                refPtrName = firstLower(refTable->getName());
            }

            ofs << "\tstd::shared_ptr<" << refTable->getName() << "> " << refPtrName << " = nullptr;" << endl;
        }
    }

    for (const Table::Pointer & otherTable: model.getTables()) {
        Column::Pointer ref = otherTable->ourMapTableReference(table);
        if (ref != nullptr) {
            Table::Pointer refTable = ref->getOurTable().lock();
            ofs << "	ShowLib::JSONSerializableVector<" << refTable->getName() << "> " << firstLower(refTable->getName())
                << "Vector;" << endl;
        }
    }
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
        << "#include <showlib/VectorUtilities.h>" << endl
        << endl

        << "#include <" << cppIncludePath << "base/" << myClassName << ".h>" << endl
        << "#include <" << cppIncludePath << name << ".h>" << endl
           ;

    generateC_CommonIncludes(ofs, table);

    ofs << "using std::string;" << endl
        << endl

        << "/**" << endl
        << " * Constructor." << endl
        << " */" << endl
        << myClassName << "::" << myClassName << "() {" << endl
        << "}" << endl
        << endl

        << "/**" << endl
        << " * Constructor from JSON." << endl
        << " */" << endl
        << myClassName << "::" << myClassName << "(const JSON &json) {" << endl
        << "	fromJSON(json);" << endl
        << "}" << endl
        << endl

        << "/**" << endl
        << " * Destructor." << endl
        << " */" << endl
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
        if (column->getSerialize()) {
            ofs << "    " << column->getName() << " = " << cTypeFor(column->getDataType()) << "Value("
                << "json, \"" << column->getName() << "\");"
                << endl ;
        }
    }

    ofs << "}" << endl << endl;

    //======================================================================
    // Generate toJSON.
    //======================================================================
    ofs << "/**\n"
        << " * Write to JSON.\n"
        << " */\n"
        << "JSON " << myClassName << "::toJSON() const {\n"
        << "    JSON json = JSON::object();\n"
           ;

    for (const Column::Pointer &column: table.getColumns()) {
        if (column->getSerialize()) {
            ofs << "    json[\"" << column->getName() << "\"] = " << column->getName() << ";\n";
        }
    }

    ofs << "    return json;\n"
        << "}\n" << endl;

    //======================================================================
    // Any finders.
    //======================================================================
    for (const Column::Pointer &col: table.getColumns()) {
        if (!col->getWantFinder()) {
            continue;
        }

        string colUpper = firstUpper(col->getName());
        ofs << myClassName << "::Pointer find_By" << colUpper
            << "(" << myClassName << "::Vector & vec, const " << cTypeFor(col->getDataType())
                << " & value) {" << endl
            << "    return vec.findIf([=](const " << name
                << "::Pointer &ptr){ return ptr->get" << colUpper << "() == value; });" << endl
            << "}" << endl;
    }

    //======================================================================
    // Here is where we generate added and remover methods against
    // things with foreign key relationships to us.
    //======================================================================
    for (const Table::Pointer & otherTable: model.getTables()) {
        Column::Pointer ref = otherTable->ourMapTableReference(table);
        if (ref != nullptr) {
            generateC_FK_Add(ofs, table, *otherTable, *ref);
            generateC_FK_Remove(ofs, table, *otherTable, *ref);
            generateC_FK_RemoveAll(ofs, table, *otherTable, *ref);
        }
    }
}

/**
 * Generate any includes suggested by having foreing key relationships.
 *
 * It is unlikely we'll double-reference a table.
 * We might chain-reference, but the pragma once
 * will protect us.
 */
void
CodeGenerator_CPP::generateC_CommonIncludes(std::ostream &ofs, DataModel::Table & table) {
    bool didRefs = false;

    // Do any FKs we contain.
    for (const Column::Pointer &column: table.getColumns()) {
        const Column::Pointer ref = column->getReferences();
        if (ref != nullptr) {
            Table::Pointer refTable = ref->getOurTable().lock();
            ofs << "#include <" << cppIncludePath << refTable->getName() << ".h>" << endl;
            didRefs = true;
        }
    }

    // Do any FKs to us. We'll assume we don't point back and forth.
    for (const Table::Pointer & otherTable: model.getTables()) {
        Column::Pointer ref = otherTable->ourMapTableReference(table);
        if (ref != nullptr) {
            ofs << "#include <" << cppIncludePath << otherTable->getName() << ".h>" << endl;
            didRefs = true;
        }
    }

    // An extra blank line.
    if (didRefs) {
        ofs << endl;
    }
}

/**
 * void addFoo(Foo::Pointer obj) {
 *      int thisId = ptr->getId();
 *      ShowLib::addIfNot(vec, obj, [=](const Foo::Pointer ptr){ return ptr->getId() == thisId; });
 * }
 *
 * @param ofs The output stream (a C++ file)
 * @param table The parent table (we're probably referenced by our primary key)
 * @param refTable The table with a foreign key into us
 * @param refColumn The column in the other table that links to us
 */
void
CodeGenerator_CPP::generateC_FK_Add(
    std::ostream &ofs,
    DataModel::Table & table,
    DataModel::Table & refTable,
    DataModel::Column & refColumn)
{
    string myCol = ShowLib::firstUpper(refColumn.getReferences()->getName());
    string theirCol = ShowLib::firstUpper(refColumn.getName());
    string refName = refTable.getName();
    string vecName = ShowLib::firstLower(refTable.getName()) + "Vector";

    ofs << endl
        << "void " << table.getName() << "_Base::"
        << "add" << refTable.getName() << "(const std::shared_ptr<" << refTable.getName() << "> obj) {" << endl
        << "    int thisId = obj->get" << myCol << "();" << endl
        << "    ShowLib::addIfNot(" << vecName << ", obj, [=](" << refName << "::Pointer ptr){ return ptr->get" << theirCol << "() == thisId; });" << endl
        << "}" << endl
           ;
}

/**
 * Generate removeFoo(Foo::Pointer foo) {...}
 *
 * @param ofs The output stream (a C++ file)
 * @param table The parent table (we're probably referenced by our primary key)
 * @param refTable The table with a foreign key into us
 * @param refColumn The column in the other table that links to us
 */
void
CodeGenerator_CPP::generateC_FK_Remove(
    std::ostream &ofs,
    DataModel::Table & table,
    DataModel::Table & refTable,
    DataModel::Column & refColumn)
{
    string myCol = ShowLib::firstUpper(refColumn.getReferences()->getName());
    string theirCol = ShowLib::firstUpper(refColumn.getName());
    string refName = refTable.getName();
    string vecName = ShowLib::firstLower(refTable.getName()) + "Vector";

    ofs << endl
        << "void " << table.getName() << "_Base::"
        << "remove" << refTable.getName() << "(const std::shared_ptr<" << refTable.getName() << "> obj) {" << endl
        << "    int thisId = obj->get" << myCol << "();" << endl
        << "    ShowLib::eraseIf(" << vecName << ", [=](" << refName << "::Pointer ptr){ return ptr->get" << theirCol << "() == thisId; });" << endl
        << "}" << endl
           ;
}

/**
 * Generate removeAllFoo(Foo::Pointer foo) {...}
 */
void
CodeGenerator_CPP::generateC_FK_RemoveAll(
     std::ostream &ofs,
     DataModel::Table & table,
     DataModel::Table & refTable,
     DataModel::Column &)
{
    string refName = refTable.getName();
    string vecName = ShowLib::firstLower(refTable.getName()) + "Vector";

    ofs << endl
        << "void " << table.getName() << "_Base::"
        << "removeAll" << refTable.getName() << "() {" << endl
        << "    " << vecName << ".clear();" << endl
        << "}" << endl
    ;
}

//======================================================================
// The concrete (subclass) include file.
//======================================================================

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
            << "#include <" << cppIncludePath << "base/" << baseClassName << ".h>" << endl
            << endl
            << "class " << name << ": public " << baseClassName << " {" << endl
            << "public:" << endl
            << "\t" << name << "() = default;" << endl
            << "\t" << name << "(const JSON &json) : " << baseClassName << "(json) {};" << endl
            << "\tvirtual ~" << name << "();" << endl
            << endl;



        //--------------------------------------------------
        // Close it out
        //--------------------------------------------------
        ofs << "};" << endl;
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
        ofs << "#include <" << cppIncludePath << name << ".h>" << endl
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

//----------------------------------------------------------------------
// This is for generating the Utilities objects, defined in base/Utilities.h
//----------------------------------------------------------------------

void
CodeGenerator_CPP::generateUtilities() {
    string hName = cppStubDirName + "/Utilities.h";
    string cppName = cppStubDirName + "/Utilities.cpp";

    std::ofstream hOutput{hName};
    std::ofstream cppOutput{cppName};

    //----------------------------------------------------------------------
    // The includes in both the .h and .cpp
    //----------------------------------------------------------------------
    hOutput << "#include <vector>" << endl
            << "#include <memory>" << endl
            << endl;

    for (const Table::Pointer &table: model.getTables()) {
        hOutput << "#include <" << cppIncludePath << table->getName() << ".h>" << endl;
    }

    cppOutput << "#include <iostream>" << endl
              << "#include \"Utilities.h\"" << endl
              << endl;

    //----------------------------------------------------------------------
    // Generate a findById() call.
    //----------------------------------------------------------------------

    hOutput
        << endl
        << "/**" << endl
        << " * All our tables have an id field. Search a vector on it."<< endl
        << " */"<< endl
        << "template<typename T>"<< endl
        << "std::shared_ptr<T> findById(const ShowLib::JSONSerializableVector<T> & vec, int id) {"<< endl
        << "	return vec.findIf( [=](auto ptr) { return ptr->getId() == id; } );"<< endl
        << "}"<< endl
        << endl;

    //----------------------------------------------------------------------
    // Now for each file, we find any FK references and create the helper.
    //----------------------------------------------------------------------
    for (const Table::Pointer &table: model.getTables()) {
        for (const Column::Pointer &column: table->getColumns()) {
            const Column::Pointer ref = column->getReferences();
            if (ref != nullptr) {
                Table::Pointer refTable = ref->getOurTable().lock();
                string fromGetter = string{"get"} + ShowLib::firstUpper(column->getName());
                string toGetter = string{"get"} + ShowLib::firstUpper(ref->getName());
                string fromSetter = string{"set"} + refTable->getName();
                string toAdder = string{"add"} + table->getName();

                // We do this bidirection so you don't need to remember which way.
                hOutput
                        << "void resolveReferences("
                        << table->getName() << "::Vector &, "
                        << refTable->getName() << "::Vector &);"
                        << endl

                        << "void resolveReferences("
                        << refTable->getName() << "::Vector &vecA, "
                        << table->getName() << "::Vector &vecB);" << endl

                        << "void resolveReferences("
                        << refTable->getName() << "::Pointer &, "
                        << table->getName() << "::Vector &);" << endl
                        << endl ;

                // And we do a version that takes the single object (with the PK)
                // and a vector of possible children.

                cppOutput << "void resolveReferences("
                        << table->getName() << "::Vector &vecA, "
                        << refTable->getName() << "::Vector &vecB) {" << endl
                        << "	for (const " << table->getName() << "::Pointer &outer: vecA) {" << endl
                        << "		for (const " << refTable->getName() << "::Pointer &inner: vecB) {" << endl
                        << "			if (outer->" << fromGetter << "() == inner->" << toGetter << "()) {" << endl
                        << "				outer->" << fromSetter << "(inner);" << endl
                        << "				inner->" << toAdder << "(outer);" << endl
                        << "			}" << endl
                        << "		}" << endl
                        << "	}" << endl
                        << "}" << endl << endl
                           ;

                cppOutput << "void resolveReferences("
                          << refTable->getName() << "::Vector &vecA, " << table->getName() << "::Vector &vecB) {" << endl
                          << "	resolveReferences(vecB, vecA); " << endl
                          << "}" << endl << endl;

                cppOutput << "void resolveReferences("
                        << refTable->getName() << "::Pointer &parent, "
                        << table->getName() << "::Vector &vec) {" << endl
                        << "	for (const " << table->getName() << "::Pointer &child: vec) {" << endl
                        << "		if (child->" << fromGetter << "() == parent->" << toGetter << "()) {" << endl
                        << "			child->" << fromSetter << "(parent);" << endl
                        << "			parent->" << toAdder << "(child);" << endl
                        << "		}" << endl
                        << "	}" << endl
                           ;

                cppOutput << "}" << endl << endl;

            }
        }
    }
}
