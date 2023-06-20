#pragma once

#include <string>
#include <vector>
#include <memory>

#include <showlib/JSONSerializable.h>

class DataModel: public ShowLib::JSONSerializable
{
public:
    typedef std::shared_ptr<DataModel> Pointer;
    typedef ShowLib::JSONSerializableVector<DataModel> Vector;

    class Table;

    /**
     * One column in a table.
     */
    class Column: public ShowLib::JSONSerializable
    {
    public:
        typedef std::shared_ptr<Column> Pointer;
        typedef ShowLib::JSONSerializableVector<Column> Vector;

        /**
         * This is the list of datatypes a column can have and comes from the PostgreSQL 13.1
         * data types list. Other databases may not support all of these. I've skipped the silly
         * ones like box. There's a bunch I don't use, but I'm putting them in anyway.
         */
        enum class DataType {
            // Numeric values
            BigInt,			// signed 8-byte int
            BigSerial,		// autoincrementing 8-byte int
            Bit,			// fix-length bit string specified as bit(n)
            VarBit,			// Variable length bit string with a max length
            SmallInt,		// 2-byte int
            Serial,			// 4-byte serial
            Boolean,		// true/false
            Double,			// 8-byte float
            Integer,		// 4-byte int
            Real,			// 4-byte float
            Numeric,		// Decimal value with specific precision.

            // Character data
            ByteArray,		// Binary data -- bytea
            Character,		// Fix-length character string
            VarChar,		// Variable-length character string
            Text,			// Variable-length character string

            // Date/time
            Interval,		// Time Span
            Date,			// year, month, day
            Time,			// Time of day without timezone
            TimeTZ,			// Time of day with timezone
            Timestamp,		// Date & time without TZ
            TimestampTZ		// Date & time with tZ
        };

        typedef std::pair<std::string, DataModel::Column::DataType> DataTypePair;

        // Types I'm skipping for now:
        //
        //		box
        //		cidr
        //		inet
        //		json + jsonb
        //		line
        //		lseg
        //		macaddr + macaddr8
        //		money
        //		path
        //		pg_lsn
        //		pg_snapshot
        //		point
        //		polygon
        //		tsquery
        //		tsvector
        //		txid_snapshot
        //		uuid
        //		xml
        //
        // Feel free to move these into the list above, but I don't use them and
        //		am not taking the time to ensure I know how.

        Column() = default;
        Column(std::shared_ptr<Table> t);
        Column(std::shared_ptr<Table> t, DataType dt);

        virtual ~Column();

        bool deepEquals(const Column &orig) const;

        void fromJSON(const JSON &) override;
        JSON toJSON() const override;

        std::weak_ptr<Table> getOurTable() const { return ourTable; }

        const std::string getName() const { return name; }
        const std::string getRefPtrName() const { return refPtrName; }
        const std::string getDbName() const { return dbName; }
        const std::string getReferenceStr() const { return referenceStr; }
        DataType getDataType() const { return dataType; }
        int getLength() const { return dataLength; }
        int getPrecisionP() const { return precisionP; }
        int getPrecisionS() const { return precisionS; }
        bool getNullable() const { return nullable; }
        bool getIsPrimaryKey() const { return isPrimaryKey; }
        bool getWantIndex() const { return wantIndex; }
        bool getWantFinder() const { return wantFinder; }
        bool getSerialize() const { return serialize; }

        bool isString() const;
        bool isDate() const;
        bool isTimestamp() const;
        bool isForeignKey() const { return references != nullptr; }

        Pointer getReferences() const { return references; }

        Column & setName(const std::string &value) { name = value; return *this; }
        Column & setRefPtrName(const std::string &value) { refPtrName = value; return *this; }
        Column & setDbName(const std::string &value) { dbName = value; return *this; }

        Column & setReferenceStr(const std::string &value) {
            referenceStr = value;
            references = nullptr;
            return *this;
        }

        Column & setDataType(DataType dt) { dataType = dt; return *this; }
        Column & setLength(int length) { dataLength = length; return *this; }
        Column & setPrecision(int p, int s) { precisionP = p; precisionS = s; return *this; }
        Column & setNullable(bool value) { nullable = value; return *this; }
        Column & setIsPrimaryKey(bool value) { isPrimaryKey = value; return *this; }
        Column & setWantIndex(bool value) { wantIndex = value; return *this; }
        Column & setWantFinder(bool value) { wantFinder = value; return *this; }
        Column & setSerialize(bool value) { serialize = value; return *this; }

        Column & setReferences(Pointer value) { references = value; return *this; }

        std::string fullName(bool useDbName = false) const;
        std::string precisionStr() const;
        std::string flagsStr() const;

    private:
        /** What table contains us? */
        std::weak_ptr<Table> ourTable;

        /** This is the name within C++ */
        std::string	name;

        /**
         * If this is a foreign key, then this is the name of the field holding a pointer.
         * If empty, we end up using the remote table name, which works any time that
         * we don't have two references to the same table.
         */
        std::string	refPtrName;

        /** This is the column name */
        std::string	dbName;

        /** This is the string from the JSON before we look it up. */
        std::string referenceStr;

        /** Datatype. */
        DataType dataType = DataType::VarChar;

        /** For fields that require a length */
        int dataLength = 0;
        int precisionP = 0;
        int precisionS = 0;

        //----------------------------------------------------------------------
        // Attributes
        //----------------------------------------------------------------------
        /** Are null values allowed? */
        bool nullable = true;

        /** Is this the primary key? */
        bool isPrimaryKey = false;

        /** Generate an index? */
        bool wantIndex = false;

        /** Generate a finder method on the vector? */
        bool wantFinder = false;

        /**
         * Should this column be serialized in toJSON() / fromJSON()?
         * This field is convenient for things like password fields.
         */
        bool serialize = true;

        /** If this is a foreign key. */
        Pointer		references = nullptr;
    };

    /**
     * One table in the database.
     */
    class Table: public std::enable_shared_from_this<Table>, public ShowLib::JSONSerializable
    {
    public:
        typedef std::shared_ptr<Table> Pointer;
        typedef ShowLib::JSONSerializableVector<Table> Vector;

        virtual ~Table();

        bool deepEquals(const Table &orig) const;

        void fromJSON(const JSON &) override;
        JSON toJSON() const override;

        const std::string getName() const { return name; }
        const std::string getDbName() const { return dbName; }

        Table & setName(const std::string &value) { name = value; return *this; }
        Table & setDbName(const std::string &value) { dbName = value; return *this; }

        Column::Pointer createColumn(const std::string &colName, Column::DataType dt);
        const Column::Pointer findColumn(const std::string &colName) const;
        const Column::Pointer findPrimaryKey() const;

        const Column::Vector & getColumns() const { return columns; }
        void sortColumns();

        bool looksLikeMapTableFor(const Table &) const;
        const Column::Vector getAllReferencesToTable(const Table &) const;
        const Column::Pointer otherMapTableReference(const Table &) const;
        const Column::Pointer ourMapTableReference(const Table &) const;

    private:
        Column::Vector	columns;

        /** This is the name within C++ */
        std::string	name;

        /** This is the column name */
        std::string	dbName;

        /** The GUI doesn't know about this yet. */
        bool isMap = false;
    };

    //======================================================================
    // Methods
    //======================================================================
    bool deepEquals(const DataModel &orig) const;

    void fromJSON(const JSON &) override;
    JSON toJSON() const override;

    const std::string & getName() const { return name; }
    void setName(const std::string &value) { name = value; markDirty(); }

    Table::Pointer createTable(const std::string &tableName);
    const Table::Pointer findTable(const std::string &tableName) const;
    void sortTables();
    void sortAllColumns();
    void clear() { tables.clear(); isDirty = false; }

    const Table::Vector & getTables() const { return tables; }
    void pushTable(DataModel::Table::Pointer);

    bool fixReferences();

    bool getIsDirty() const { return isDirty; }
    void markDirty() { isDirty = true; }
    void markClean() { isDirty = false; }

private:
    std::string name;
    Table::Vector tables;
    bool isDirty = false;
};

std::string toString(DataModel::Column::DataType dt);
DataModel::Column::DataType toDataType(const std::string &);
bool dataTypeHasLength(DataModel::Column::DataType dt);
bool dataTypeHasPrecision(DataModel::Column::DataType dt);
bool dataTypeIsSerial(DataModel::Column::DataType dt);
std::string cTypeFor(DataModel::Column::DataType dt);

std::vector<DataModel::Column::DataTypePair> & allDataTypes();


