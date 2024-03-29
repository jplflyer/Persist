#include <algorithm>
#include <mutex>

#include <showlib/CommonUsing.h>
#include <showlib/StringUtils.h>

#include "DataModel.h"

using namespace ShowLib;

using Table = DataModel::Table;
using Column = DataModel::Column;
using DataType = DataModel::Column::DataType;

const char * DataModel::Generator::NAME_SQL = "SQL";
const char * DataModel::Generator::NAME_CPP = "C++";
const char * DataModel::Generator::NAME_CPP_DBACCESS = "C++ DBAccess";
const char * DataModel::Generator::NAME_JAVA = "Java";
const char * DataModel::Generator::NAME_FLYWAY = "Flyway";

const char * DataModel::Database::DRIVER_POSTGRESQL = "PostgreSql";

/**
 * Do a deep compare against this other DataModel.
 */
bool
DataModel::deepEquals(const DataModel &orig) const {
    if (tables.size() != orig.tables.size()) {
        return false;
    }

    for (const Table::Pointer & table: tables) {
        const Table::Pointer otherTable = orig.findTable(table->getName());
        if (otherTable == nullptr) {
            return false;
        }
        if (!table->deepEquals(*otherTable)) {
            return false;
        }
    }

    return true;
}

/**
 * Read from JSON.
 */
void
DataModel::fromJSON(const JSON &json)  {
    name = stringValue(json, "name");
    tables.fromJSON(jsonArray(json, "tables"));
    generators.fromJSON(jsonArray(json, "generators"));
    databases.fromJSON(jsonArray(json, "databases"));
    generatedVersion = intValue(json, "generatedVersion");
}

/**
 * Write to JSON.
 */
JSON DataModel::toJSON() const {
    JSON json = JSON::object();

    json["name"] = name;
    json["tables"] = tables.toJSON();
    json["generators"] = generators.toJSON();
    json["databases"] = databases.toJSON();

    if (generatedVersion > 0) {
        json["generatedVersion"] = generatedVersion;
    }

    return json;
}

/**
 * Create a new table with this name.
 */
DataModel::Table::Pointer
DataModel::createTable(const std::string &tableName) {
    Table::Pointer table = std::make_shared<Table>();
    tables.push_back(table);
    table->setName(tableName)
          .setDbName(camelToLower(tableName)) ;
    return table;
}

/**
 * Find this table.
 */
const DataModel::Table::Pointer
DataModel::findTable(const std::string &tableName) const {
    return tables.findIf( [=](const Table::Pointer &ptr){ return ptr->getName() == tableName; } );
}

void DataModel::pushTable(DataModel::Table::Pointer table) {
    tables.push_back(table);
}

/**
 * This goes through all tables and finds any with foreign key relationships.
 * It updates their pointers.
 *
 * Returns true if any references were resolved.
 */
bool
DataModel::fixReferences() {
    bool errors = false;

    for (const Table::Pointer & table: tables) {
        for (const Column::Pointer & column: table->getColumns()) {
            if (column->getReferenceStr().length() > 0 && column->getReferences() == nullptr) {
                std::vector<string> parts = split(column->getReferenceStr(), ".");
                Table::Pointer foundTable = findTable(parts.at(0));
                Column::Pointer foundColumn = nullptr;

                if (foundTable == nullptr) {
                    cerr << "Unable to find referenced table for " << column->getReferenceStr() << endl;
                    errors = true;
                    continue;
                }

                foundColumn = (parts.size() > 1) ? foundTable->findColumn(parts.at(1))
                        : foundTable->findPrimaryKey();
                if (foundColumn == nullptr) {
                    cerr << "Can't find referenced column for " << column->getReferenceStr() << endl;
                    errors = true;
                    continue;
                }

                column->setReferences(foundColumn);
            }
        }
    }

    return !errors;
}

/**
 * Sort the tables based on name.
 */
void
DataModel::sortTables() {
    std::sort(tables.begin(), tables.end(),
        [](const Table::Pointer &first, const Table::Pointer &second)
        {
            return first->getName() < second->getName();
        } );
}

/**
 * This runs through all tables and does a sort on all the columns.
 */
void
DataModel::sortAllColumns() {
    for (Table::Pointer &table: tables) {
        table->sortColumns();
    }
}

void DataModel::pushGenerator(DataModel::Generator::Pointer gen) {
    generators.push_back(gen);
}

void DataModel::pushDatabase(DataModel::Database::Pointer db) {
    databases.push_back(db);
}

/**
 * Find all foreign key relationships to this table.
 */
Column::Vector DataModel::findReferencesTo(const Table &table) {
    Column::Vector vec;

    for (const Table::Pointer & thisTable: tables) {
        Column::Vector refs = thisTable->getAllReferencesToTable(table);
        for (const Column::Pointer &col: refs) {
            vec.push_back(col);
        }
    }

    return vec;
}

//======================================================================
// Columns.
//======================================================================

class DataTypeInfo {
public:
    DataTypeInfo() = default;
    DataTypeInfo(string _name, DataType _dt, string _cType): name(_name), dataType(_dt), cType(_cType) {}

    string name;
    DataType dataType;
    bool hasLength = false;
    bool hasPrecision = false;
    bool isSerial = false;
    bool isString = false;
    bool isDate = false;
    bool isTimestamp = false;
    string lowerName;
    string cType;
};

// These two maps are reverses of each other for rapid lookup.
static std::map<std::string, DataTypeInfo> * stringToDataTypeMap = nullptr;
static std::map<DataModel::Column::DataType, DataTypeInfo> * dataTypesMap = nullptr;
static std::vector<std::pair<std::string, DataType>> * dataTypeVector = nullptr;

static DataTypeInfo serialType(string name, DataType dt, string cType) {
    DataTypeInfo retVal{name, dt, cType};
    retVal.isSerial = true;
    return retVal;
}

static DataTypeInfo precisionType(string name, DataType dt, string cType) {
    DataTypeInfo retVal{name, dt, cType};
    retVal.hasPrecision = true;
    return retVal;
}

static DataTypeInfo stringType(string name, DataType dt, bool hasLength) {
    DataTypeInfo retVal{name, dt, "string"};
    retVal.hasLength = hasLength;
    retVal.isString = true;
    return retVal;
}

static DataTypeInfo dateType(string name, DataType dt) {
    DataTypeInfo retVal{name, dt, "string"};
    retVal.isDate = true;
    return retVal;
}

static DataTypeInfo timeType(string name, DataType dt) {
    DataTypeInfo retVal{name, dt, "string"};
    retVal.isTimestamp = true;
    return retVal;
}

static void populateMaps() {
    static std::mutex mapMutex;
    std::unique_lock<std::mutex> lock(mapMutex);
    if (stringToDataTypeMap == nullptr) {
        stringToDataTypeMap = new std::map<std::string, DataTypeInfo>();
        dataTypesMap = new std::map<DataModel::Column::DataType, DataTypeInfo>();
        dataTypeVector = new std::vector<std::pair<std::string, DataType>>();

        std::vector<DataTypeInfo> vec;

        // TODO: Some of these are wrong.
        vec.push_back( {"BigInt", DataType::BigInt, "long"} );
        vec.push_back( serialType("BigSerial", DataType::BigSerial, "long") );
        vec.push_back( {"Bit", DataType::Bit, "short"} );
        vec.push_back( {"VarBit", DataType::VarBit, "short"} );
        vec.push_back( {"SmallInt", DataType::SmallInt, "short"} );
        vec.push_back( serialType("Serial", DataType::Serial, "int") );
        vec.push_back( {"Boolean", DataType::Boolean, "bool"} );
        vec.push_back( {"Double", DataType::Double, "double"} );
        vec.push_back( {"Integer", DataType::Integer, "int"} );
        vec.push_back( {"Real", DataType::Real, "double"} );
        vec.push_back( precisionType("Numeric", DataType::Numeric, "double") );

        vec.push_back( stringType("ByteArray", DataType::ByteArray, true) );
        vec.push_back( stringType("Character", DataType::Character, true) );
        vec.push_back( stringType("VarChar", DataType::VarChar, true) );
        vec.push_back( stringType("Text", DataType::Text, false) );

        vec.push_back( {"Interval", DataType::Interval, "string"} );

        vec.push_back( dateType("Date", DataType::Date) );
        vec.push_back( timeType("Time", DataType::Time) );
        vec.push_back( timeType("TimeTZ", DataType::TimeTZ) );
        vec.push_back( timeType("Timestamp", DataType::Timestamp) );
        vec.push_back( timeType("TimestampTZ", DataType::TimestampTZ) );

        for (DataTypeInfo &info: vec) {
            info.lowerName = toLower(info.name);
            (*stringToDataTypeMap)[info.lowerName] = info;
            (*dataTypesMap)[info.dataType] = info;

            dataTypeVector->push_back( {info.name, info.dataType} );
        }
    }
}

/**
 * Convert this dt to a string.
 */
std::string toString(DataModel::Column::DataType dt) {
    populateMaps();
    return dataTypesMap->at(dt).name;
}

/**
 * Return a dt from this string.
 */
DataModel::Column::DataType toDataType(const std::string &str) {
    populateMaps();
    string searchFor = toLower(str);
    return stringToDataTypeMap->at(searchFor).dataType;
}

bool dataTypeHasLength(DataModel::Column::DataType dt) {
    populateMaps();
    return dataTypesMap->at(dt).hasLength;
}

bool dataTypeHasPrecision(DataModel::Column::DataType dt) {
    populateMaps();
    return dataTypesMap->at(dt).hasPrecision;
}

/**
 * Is this data type one of the Serial types?
 */
bool dataTypeIsSerial(DataModel::Column::DataType dt) {
    populateMaps();
    return dataTypesMap->at(dt).isSerial;
}

/**
 * What underlying C++ datatype do we use?
 */
string cTypeFor(DataModel::Column::DataType dt) {
    populateMaps();
    return dataTypesMap->at(dt).cType;
}

std::vector<std::pair<std::string, DataModel::Column::DataType>> & allDataTypes() {
    populateMaps();
    return *dataTypeVector;
}



/**
 * Constructor.
 */
DataModel::Column::Column(Table::Pointer t)
    : ourTable(t)
{
}

/**
 * Constructor.
 */
DataModel::Column::Column(Table::Pointer t, DataType dt)
    : ourTable(t), dataType(dt)
{
}

/**
 * Destructor.
 */
DataModel::Column::~Column() {
}

/**
 * Is these objects identical?
 */
bool
DataModel::Column::deepEquals(const DataModel::Column &orig) const {
    return (name == orig.name
        && refPtrName == orig.refPtrName
        && reversePtrName == orig.reversePtrName
        && dbName == orig.dbName
        && dataType == orig.dataType
        && dataLength == orig.dataLength
        && precisionP == orig.precisionP
        && precisionS == orig.precisionS
        && nullable == orig.nullable
        && isPrimaryKey == orig.isPrimaryKey
        && wantIndex == orig.wantIndex
        && wantFinder == orig.wantFinder
        );
}

/**
 * Read from JSON.
 */
void
DataModel::Column::fromJSON(const JSON &json) {
    name = stringValue(json, "name");
    refPtrName = stringValue(json, "refPtrName");
    reversePtrName = stringValue(json, "reversePtrName");
    dbName = camelToLower(stringValue(json, "dbName"));
    dataType = toDataType(stringValue(json, "dataType"));
    dataLength = intValue(json, "length");
    precisionP = intValue(json, "precisionP");
    precisionS = intValue(json, "precisionS");

    referenceStr = stringValue(json, "references");

    nullable = boolValue(json, "nullable");
    isPrimaryKey = boolValue(json, "isPrimaryKey");
    wantIndex = boolValue(json, "wantIndex");
    wantFinder = boolValue(json, "wantFinder");
    serialize = boolValue(json, "serialize", true);
}

/**
 * Write to JSON.
 */
JSON  DataModel::Column::toJSON() const {
    JSON json = JSON::object();
    json["name"] = name;
    json["dbName"] = dbName;
    json["dataType"] = ::toString(dataType);

    setStringValue(json, "refPtrName", refPtrName);
    setStringValue(json, "reversePtrName", reversePtrName);
    setLongValue(json, "length", dataLength);
    setLongValue(json, "precisionP", precisionP);
    setLongValue(json, "precisionS", precisionS);

    if (references != nullptr) {
        Table::Pointer refTable = references->getOurTable().lock();
        setStringValue(json, "references", refTable->getName() + "." + references->getName());
    }
    else {
        setStringValue(json, "references", referenceStr);
    }

    json["nullable"] = nullable;
    json["isPrimaryKey"] = isPrimaryKey;
    json["wantIndex"] = wantIndex;
    json["wantFinder"] = wantFinder;
    json["serialize"] = serialize;

    return json;
}

/**
 * Return ourself as tablename (columnname).
 */
std::string
DataModel::Column::fullName(bool useDbName) const {
    Table::Pointer table = ourTable.lock();

    return (useDbName ? table->getDbName() : table->getName()) + " (" + (useDbName ? dbName : name) + ")";
}

/**
 * Return a string representing the precision / length
 */
std::string
DataModel::Column::precisionStr() const {
    string retVal;

    if (dataTypeHasPrecision(dataType)) {
        retVal = std::to_string(precisionP) + "." + std::to_string(precisionS);
    }
    else if (dataTypeHasLength(dataType)) {
        if (dataLength > 0) {
            retVal = std::to_string(dataLength);
        }
    }

    return retVal;
}

std::string
DataModel::Column::flagsStr() const {
    string retVal;
    string delim;

    if (!nullable) {
        retVal = retVal + delim + "NOT NULL";
        delim = " ";
    }
    if (isPrimaryKey) {
        retVal = retVal + delim + "PRIMARY KEY";
        delim = " ";
    }
    if (wantIndex) {
        retVal = retVal + delim + "INDEX";
        delim = " ";
    }
    if (serialize) {
        retVal += delim + "SERIALIZE";
        delim = " ";
    }

    return retVal;
}

/**
 * Has the data type changed from what was previously generated.
 */
bool DataModel::Column::hasDataTypeChanged() const {
    return dataType != dataTypeGenerated
        || dataLength != dataLengthGenerated
        || precisionP != precisionPGenerated
        || precisionS != precisionSGenerated
        ;
}

/**
 * These fields used in the Flyway generator (and maybe others in the future) to detect
 * changes in column name and/or datatype. Once we've generated a migration, we want
 * to set them.
 */
DataModel::Column & DataModel::Column::setGeneratedValues() {
    dbNameGenerated = dbName;
    dataTypeGenerated = dataType;
    dataLengthGenerated = dataLength;
    precisionPGenerated = precisionP;
    precisionSGenerated = precisionS;

    return *this;
}

/**
 * Vector of Tables -- populate from JSON. This is special because
 * Column wants a Table::Pointer
 */
void DataModel::Column_Vector::populate(Table::Pointer table, const JSON & json) {
    for (auto iter = json.begin(); iter != json.end(); ++iter) {
        const JSON obj = *iter;

        Column::Pointer thisDiff = std::make_shared<Column>(table);
        thisDiff->fromJSON(obj);
        push_back(thisDiff);
    }
}


//======================================================================
// Tables.
//======================================================================

/**
 * Destructor.
 */
DataModel::Table::~Table() {
}

/**
 * Perform a deep comparison.
 */
bool
DataModel::Table::deepEquals(const DataModel::Table &orig) const {
    if ( name != orig.name || dbName != orig.dbName || columns.size() != orig.columns.size()) {
        return false;
    }

    for (const Column::Pointer &col: columns) {
        const Column::Pointer otherCol = orig.findColumn(col->getName());
        if (otherCol == nullptr || !col->deepEquals(*otherCol) ) {
            return false;
        }
    }

    return true;
}

/**
 * Read from JSON.
 */
void
DataModel::Table::fromJSON(const JSON &json)  {
    name = stringValue(json, "name");
    dbName = camelToLower(stringValue(json, "dbName"));
    dbNameGenerated = stringValue(json, "dbNameGenerated");
    version = intValue(json, "version");

    columns.populate(shared_from_this(), jsonArray(json, "columns"));
    deletedColumns.populate(shared_from_this(), jsonArray(json, "deletedColumns"));
}

/**
 * Write to JSON.
 */
JSON  DataModel::Table::toJSON() const {
    JSON json = JSON::object();

    json["name"] = name;
    json["dbName"] = dbName;
    json["dbNameGenerated"] = dbNameGenerated;
    json["columns"] = columns.toJSON();
    json["deletedColumns"] = deletedColumns.toJSON();
    if (version > 0) {
        json["version"] = version;
    }

    return json;
}

bool DataModel::Column::isString() const {
    return dataTypesMap->at(dataType).isString;
}

bool DataModel::Column::isDate() const {
    return dataTypesMap->at(dataType).isDate;
}

bool DataModel::Column::isTimestamp() const {
    return dataTypesMap->at(dataType).isTimestamp;
}

/**
 * Create a new column with this name.
 */
DataModel::Column::Pointer
DataModel::Table::createColumn(const std::string &colName, DataModel::Column::DataType dt) {
    Column::Pointer col = std::make_shared<Column>(shared_from_this(), dt);
    columns.push_back(col);
    col->setName(colName)
        .setDbName(camelToLower(colName))
            ;
    return col;
}

/**
 * Delete a column.
 */
void DataModel::Table::deleteColumn(Column::Pointer col) {
    columns.removeAll( [=](Column::Pointer c) { return c == col; } );
    deletedColumns.push_back(col);
}

/**
 * Find this column.
 */
const DataModel::Column::Pointer
DataModel::Table::findColumn(const std::string &colName) const {
    return columns.findIf( [=](const Column::Pointer &ptr){ return ptr->getName() == colName; } );
}

/**
 * Find our primary key.
 */
const DataModel::Column::Pointer
DataModel::Table::findPrimaryKey() const {
    return columns.findIf( [=](const Column::Pointer &ptr){ return ptr->getIsPrimaryKey(); } );
}

/**
 * Sort the columns based on IsPrimaryKey then by name.
 */
void
DataModel::Table::sortColumns() {
    std::sort(columns.begin(), columns.end(),
        [](const DataModel::Column::Pointer &first, const DataModel::Column::Pointer &second)
        {
            return first->getIsPrimaryKey() ||
                ( !second->getIsPrimaryKey() && first->getName() < second->getName() );
        } );
}

/**
 * We've done a Flyway migration and can forget any deleted columns.
 */
void DataModel::Table::clearDeletedColumns() {
    deletedColumns.clear();
}

/**
 * Are we a map table for this other table?
 */
bool
DataModel::Table::looksLikeMapTableFor(const Table &other) const {
    // We're only a map table if:
    //		isMap is true
    //		or our name ends in _Map
    //		or we have exactly 3 columns, our PK and 2 FKs.
    bool canBeMap = isMap || endsWith(name, "_Map");

    if (!canBeMap) {
        if (columns.size() == 3) {
            canBeMap = true;
            for (const Column::Pointer &column: columns) {
                if (!column->getIsPrimaryKey()) {
                    Column::Pointer ref = column->getReferences();
                    if (ref == nullptr) {
                        canBeMap = false;
                        break;
                    }
                }
            }
        }
    }

    if (!canBeMap) {
        return false;
    }

    // Okay, we're probably a map file, so now see if we have a reference to other.
    Column::Pointer otherPK = other.findPrimaryKey();
    for (const Column::Pointer &column: columns) {
        Column::Pointer ref = column->getReferences();
        if (ref == otherPK) {
            return true;
        }
    }

    return false;
}

/**
 * We're a map table. One half of our map is other. We want
 * the our column that represents the link to the opposite table.
 */
const DataModel::Column::Pointer
DataModel::Table::otherMapTableReference(const Table &other) const {
    Column::Pointer otherPK = other.findPrimaryKey();
    for (const Column::Pointer &column: columns) {
        Column::Pointer ref = column->getReferences();
        if (ref != nullptr && ref != otherPK) {
            // this must be it.
            return column;
        }
    }
    return nullptr;
}

/**
 * We may have a foreign key to another table. If so, return it.
 */
const DataModel::Column::Pointer
DataModel::Table::ourMapTableReference(const Table &other) const {
    Column::Pointer otherPK = other.findPrimaryKey();
    for (const Column::Pointer &column: columns) {
        Column::Pointer ref = column->getReferences();
        if (ref != nullptr && ref == otherPK) {
            return column;
        }
    }
    return nullptr;
}

/**
 * Find any columns that reference this other table.
 */
const DataModel::Column::Vector DataModel::Table::getAllReferencesToTable(const Table &other) const {
    Column::Vector vec;

    for (const Column::Pointer &col: columns) {
        Column::Pointer ref = col->getReferences();
        if (ref != nullptr) {
            Table::Pointer refTable = ref->getOurTable().lock();
            if (refTable->getName() == other.getName()) {
                vec.push_back(col);
            }
        }
    }

    return vec;
}



//======================================================================
// Generators definitions.
//======================================================================

/**
 * Destructor.
 */
DataModel::Generator::~Generator() {
}

/**
 * Populate from JSON.
 */
void DataModel::Generator::fromJSON(const JSON &json) {
    name = stringValue(json, "name");
    description = stringValue(json, "description");
    outputBasePath = stringValue(json, "outputBasePath");
    outputClassPath = stringValue(json, "outputClassPath");
    JSON optionsJSON = jsonValue(json, "options");
    if (!optionsJSON.empty()) {
        options = optionsJSON.get<std::unordered_map<std::string, string>>();
    }
}

/**
 * Write to JSON.
 */
JSON DataModel::Generator::toJSON() const {
    JSON json = JSON::object();

    json["name"] = name;
    json["description"] = description;
    json["outputBasePath"] = outputBasePath;
    setStringValue(json, "outputClassPath", outputClassPath);
    if (!options.empty()) {
        json["options"] = options;
    }
    return json;
}

//======================================================================
// Database definitions.
//======================================================================

/**
 * Destructor.
 */
DataModel::Database::~Database() {
}

/**
 * Populate from JSON.
 */
void DataModel::Database::fromJSON(const JSON &json) {
    envName = stringValue(json, "envName");
    driver = stringValue(json, "driver");
    host = stringValue(json, "host");
    port = intValue(json, "port");
    dbName = stringValue(json, "dbName");
    username = stringValue(json, "username");
    password = stringValue(json, "password");
}

/**
 * Write to JSON.
 */
JSON DataModel::Database::toJSON() const {
    JSON json = JSON::object();

    json["envName"] = envName;
    json["driver"] = driver;
    json["host"] = host;
    json["port"] = port;
    json["dbName"] = dbName;
    json["username"] = username;
    json["password"] = password;

    return json;
}



