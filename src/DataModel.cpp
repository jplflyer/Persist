#include <mutex>

#include <showlib/StringUtils.h>

#include "DataModel.h"

using namespace ShowLib;

using std::cout;
using std::cerr;
using std::endl;
using std::string;
using DataType = DataModel::Column::DataType;

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
    tables.fromJSON(jsonArray(json, "tables"));
}

/**
 * Write to JSON.
 */
JSON
DataModel::toJSON(JSON &json) const {
    JSON tablesJSON{JSON::array()};
    json["tables"] = tables.toJSON(tablesJSON);
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

//======================================================================
// Columns.
//======================================================================

class DataTypeInfo {
public:
    string name;
    DataType dataType;
    bool hasLength = false;
    bool hasPrecision = false;
    bool isSerial = false;
    string lowerName;
};

// These two maps are reverses of each other for rapid lookup.
static std::map<std::string, DataTypeInfo> stringToDataTypeMap;
static std::map<DataModel::Column::DataType, DataTypeInfo> dataTypeToStringMap;
static std::mutex mapMutex;

static void populateMaps() {
    std::unique_lock<std::mutex> lock(mapMutex);
    if (stringToDataTypeMap.size() == 0) {
        std::vector<DataTypeInfo> vec;

        vec.push_back( {"BigInt", DataModel::Column::DataType::BigInt, false, false, false, ""} );
        vec.push_back( {"BigSerial", DataModel::Column::DataType::BigSerial, false, false, true, ""} );
        vec.push_back( {"Bit", DataModel::Column::DataType::Bit, false, false, false, ""} );
        vec.push_back( {"VarBit", DataModel::Column::DataType::VarBit, false, false, false, ""} );
        vec.push_back( {"SmallInt", DataModel::Column::DataType::SmallInt, false, false, false, ""} );
        vec.push_back( {"Serial", DataModel::Column::DataType::Serial, false, false, true, ""} );
        vec.push_back( {"Boolean", DataModel::Column::DataType::Boolean, false, false, false, ""} );
        vec.push_back( {"Double", DataModel::Column::DataType::Double, false, false, false, ""} );
        vec.push_back( {"Integer", DataModel::Column::DataType::Integer, false, false, false, ""} );
        vec.push_back( {"Real", DataModel::Column::DataType::Real, false, false, false, ""} );
        vec.push_back( {"Numeric", DataModel::Column::DataType::Numeric, false, true, false, ""} );
        vec.push_back( {"ByteArray", DataModel::Column::DataType::ByteArray, true, false, false, ""} );
        vec.push_back( {"Character", DataModel::Column::DataType::Character, true, false, false, ""} );
        vec.push_back( {"VarChar", DataModel::Column::DataType::VarChar, true, false, false, ""} );
        vec.push_back( {"Text", DataModel::Column::DataType::Text, false, false, false, ""} );
        vec.push_back( {"Interval", DataModel::Column::DataType::Interval, false, false, false, ""} );
        vec.push_back( {"Date", DataModel::Column::DataType::Date, false, false, false, ""} );
        vec.push_back( {"Time", DataModel::Column::DataType::Time, false, false, false, ""} );
        vec.push_back( {"TimeTZ", DataModel::Column::DataType::TimeTZ, false, false, false, ""} );
        vec.push_back( {"Timestamp", DataModel::Column::DataType::Timestamp, false, false, false, ""} );
        vec.push_back( {"TimestampTZ", DataModel::Column::DataType::TimestampTZ, false, false, false, ""} );

        for (DataTypeInfo &info: vec) {
            info.lowerName = toLower(info.name);
            stringToDataTypeMap[info.lowerName] = info;
            dataTypeToStringMap[info.dataType] = info;
        }
    }
}

/**
 * Convert this dt to a string.
 */
std::string toString(DataModel::Column::DataType dt) {
    populateMaps();
    return dataTypeToStringMap[dt].name;
}

/**
 * Return a dt from this string.
 */
DataModel::Column::DataType toDataType(const std::string &str) {
    populateMaps();
    string searchFor = toLower(str);
    return stringToDataTypeMap[searchFor].dataType;
}

bool dataTypeHasLength(DataModel::Column::DataType dt) {
    populateMaps();
    return dataTypeToStringMap[dt].hasLength;
}

bool dataTypeHasPrecision(DataModel::Column::DataType dt) {
    populateMaps();
    return dataTypeToStringMap[dt].hasPrecision;
}

/**
 * Is this data type one of the Serial types?
 */
bool dataTypeIsSerial(DataModel::Column::DataType dt) {
    populateMaps();
    return dataTypeToStringMap[dt].isSerial;
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
 * Perform a deep copy.
 */
bool
DataModel::Column::deepEquals(const DataModel::Column &orig) const {
    return (name == orig.name
        && dbName == orig.dbName
        && dataType == orig.dataType
        && dataLength == orig.dataLength
        && precisionP == orig.precisionP
        && precisionS == orig.precisionS
        && nullable == orig.nullable
        && isPrimaryKey == orig.isPrimaryKey
        && wantIndex == orig.wantIndex
        );
}

/**
 * Read from JSON.
 */
void
DataModel::Column::fromJSON(const JSON &json)  {
    name = stringValue(json, "name");
    dbName = camelToLower(stringValue(json, "dbName"));
    dataType = toDataType(stringValue(json, "dataType"));
    dataLength = intValue(json, "length");
    precisionP = intValue(json, "precisionP");
    precisionS = intValue(json, "precisionS");

    referenceStr = stringValue(json, "references");

    nullable = boolValue(json, "nullable");
    isPrimaryKey = boolValue(json, "isPrimaryKey");
    wantIndex = boolValue(json, "wantIndex");
}

/**
 * Write to JSON.
 */
JSON
DataModel::Column::toJSON(JSON &json) const {
    json["name"] = name;
    json["dbName"] = dbName;
    json["dataType"] = ::toString(dataType);
    setLongValue(json, "length", dataLength);
    setLongValue(json, "precisionP", precisionP);
    setLongValue(json, "precisionS", precisionS);

    setStringValue(json, "references", referenceStr);

    json["nullable"] = nullable;
    json["isPrimaryKey"] = isPrimaryKey;
    if (wantIndex) {
        json["wantIndex"] = wantIndex;
    }

    return json;
}

/**
 * Return ourself as tablename (columnname).
 */
std::string
DataModel::Column::fullName(bool useDbName) const {
    Table::Pointer table = ourTable.lock();

    return (useDbName ? table->getDbName() : table->getName()) + " (" + (useDbName ? name : dbName) + ")";
}

//======================================================================
// Tables.
//======================================================================

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

    // Do this manually so we can pass in the shared pointer to ourself.
    JSON columnsJSON = jsonArray(json, "columns");
    for (auto iter = columnsJSON.begin(); iter != columnsJSON.end(); ++iter) {
        const JSON obj = *iter;

        Column::Pointer thisDiff = std::make_shared<Column>(shared_from_this());
        thisDiff->fromJSON(obj);
        columns.push_back(thisDiff);
    }
}

/**
 * Write to JSON.
 */
JSON
DataModel::Table::toJSON(JSON &json) const {
    JSON colsJSON{ JSON::array() };

    json["name"] = name;
    json["dbName"] = dbName;
    json["columns"] = columns.toJSON(colsJSON);
    return json;
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
