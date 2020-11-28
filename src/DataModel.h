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
        Column(DataType dt);

        bool deepEquals(const Column &orig) const;

        void fromJSON(const JSON &) override;
        JSON toJSON(JSON &) const override;

        const std::string getName() const { return name; }
        const std::string getDbName() const { return dbName; }
        DataType getDataType() const { return dataType; }
        int getLength() const { return dataLength; }
        int getPercisionP() const { return precisionP; }
        int getPrecisionS() const { return precisionS; }
        bool getNullable() const { return nullable; }
        bool getIsPrimaryKey() const { return isPrimaryKey; }
        bool getWantIndex() const { return wantIndex; }

        Column & setName(const std::string &value) { name = value; return *this; }
        Column & setDbName(const std::string &value) { dbName = value; return *this; }
        Column & setDataType(DataType dt) { dataType = dt; return *this; }
        Column & setLength(int length) { dataLength = length; return *this; }
        Column & setPrecision(int p, int s) { precisionP = p; precisionS = s; return *this; }
        Column & setNullable(bool value) { nullable = value; return *this; }
        Column & setIsPrimaryKey(bool value) { isPrimaryKey = value; return *this; }
        Column & setWantIndex(bool value) { wantIndex = value; return *this; }

    private:
        /** This is the name within C++ */
        std::string	name;

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
        bool nullable = true;
        bool isPrimaryKey = false;
        bool wantIndex = false;

        /** If this is a foreign key. */
        Pointer		references = nullptr;
    };

    /**
     * One table in the database.
     */
    class Table: public ShowLib::JSONSerializable
    {
    public:
        typedef std::shared_ptr<Table> Pointer;
        typedef ShowLib::JSONSerializableVector<Table> Vector;

        bool deepEquals(const Table &orig) const;

        void fromJSON(const JSON &) override;
        JSON toJSON(JSON &) const override;

        const std::string getName() const { return name; }
        const std::string getDbName() const { return dbName; }

        Table & setName(const std::string &value) { name = value; return *this; }
        Table & setDbName(const std::string &value) { dbName = value; return *this; }

        Column::Pointer createColumn(const std::string &colName, Column::DataType dt);
        const Column::Pointer findColumn(const std::string &colName) const;

    private:
        Column::Vector	columns;

        /** This is the name within C++ */
        std::string	name;

        /** This is the column name */
        std::string	dbName;
    };

    //======================================================================
    // Methods
    //======================================================================
    bool deepEquals(const DataModel &orig) const;

    void fromJSON(const JSON &) override;
    JSON toJSON(JSON &) const override;


    Table::Pointer createTable(const std::string &tableName);
    const Table::Pointer findTable(const std::string &tableName) const;

private:
    Table::Vector tables;
};

std::string toString(DataModel::Column::DataType dt);
DataModel::Column::DataType toDataType(const std::string &);
bool dataTypeHasLength(DataModel::Column::DataType dt);
bool dataTypeHasPrecision(DataModel::Column::DataType dt);
