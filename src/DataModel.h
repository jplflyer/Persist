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
        const std::string getReversePtrName() const { return reversePtrName; }
        const std::string getDbName() const { return dbName; }
        const std::string getDbNameGenerated() const { return dbNameGenerated; }
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

        int getVersion() const { return version; }

        Pointer getReferences() const { return references; }

        Column & setName(const std::string &value) { name = value; return *this; }
        Column & setRefPtrName(const std::string &value) { refPtrName = value; return *this; }
        Column & setReversePtrName(const std::string &value) { reversePtrName = value; return *this; }
        Column & setDbName(const std::string &value) { dbName = value; return *this; }
        Column & setDbNameGenerated(const std::string &value) { dbNameGenerated = value; return *this; }
        Column & setVersion(int value) { version = value; return *this; }

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

        /**
         * If this is a foreign key, this is the name used by the other table
         * to point back to us.
         */
        std::string reversePtrName;

        /** This is the column name */
        std::string	dbName;

        /**
         * This gets used in migrations. If this name doesn't match dbName, either we're a new column
         * or the name was changed.
         */
        std::string	dbNameGenerated;

        /** This is the string from the JSON before we look it up. */
        std::string referenceStr;

        /** Datatype. */
        DataType dataType = DataType::VarChar;

        /** For fields that require a length */
        int dataLength = 0;
        int precisionP = 0;
        int precisionS = 0;

        /** Used during migrations. */
        int version = 0;

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
        const std::string getDbNameGenerated() const { return dbNameGenerated; }
        int getVersion() const { return version; }

        Table & setName(const std::string &value) { name = value; return *this; }
        Table & setDbName(const std::string &value) { dbName = value; return *this; }
        Table & setDbNameGenerated(const std::string &value) { dbNameGenerated = value; return *this; }
        Table &  setVersion(const int value) { version = value; return *this; }

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

        /** This is the class name within the generated code */
        std::string	name;

        /** This is the table name */
        std::string	dbName;

        /**
         * This gets used in migrations. If this name doesn't match dbName, either we're a new table
         * Or the name was changed.
         */
        std::string dbNameGenerated;

        /** The GUI doesn't know about this yet. */
        bool isMap = false;

        /** This is used for migrations. It gets set when the table is created. Columns are separate. */
        int version = 0;
    };

    //======================================================================
    // Generators.
    //======================================================================
    class Generator: public ShowLib::JSONSerializable
    {
    public:
        typedef std::shared_ptr<Generator> Pointer;
        typedef ShowLib::JSONSerializableVector<Generator> Vector;

        static const char * NAME_SQL;
        static const char * NAME_CPP;
        static const char * NAME_CPP_DBACCESS;
        static const char * NAME_JAVA;
        static const char * NAME_FLYWAY;

        virtual ~Generator();

        void fromJSON(const JSON &) override;
        JSON toJSON() const override;

        const std::string & getName() const { return name; }
        Generator & setName(const std::string & value) { name = value; return *this; }

        const std::string & getDescription() const { return description; }
        Generator & setDescription(const std::string & value) { description = value; return *this; }

        const std::string & getOutputBasePath() const { return outputBasePath; }
        Generator & setOutputBasePath(const std::string & value) { outputBasePath = value; return *this; }

        const std::string & getOutputClassPath() const { return outputClassPath; }
        Generator & setOutputClassPath(const std::string & value) { outputClassPath = value; return *this; }

        const std::unordered_map<std::string, std::string> & getOptions() const { return options; }

        void setOption(const std::string &key, const std::string value) {
            options[key] = value;
        }

    private:
        /** This is the name of the generator such as SQL, CPP, or Java. */
        std::string name;

        std::string description;

        /**
         * This is the output path. For:
         *
         * SQL: the name of the output file
         * C++: Top of the output directory.
         * Java: Top of the output directory with ClassPath to be added.
         */
        std::string outputBasePath;

        /**
         * For C++, this is the prefix for include files.
         * For Java, this is the class path.
         */
        std::string outputClassPath;

        /**
         * Any special options.
         */
        std::unordered_map<std::string, std::string> options;
    };

    //======================================================================
    // Database Connections. Currently we support a limited set of choices.
    //======================================================================
    class Database: public ShowLib::JSONSerializable {
    public:
        using Pointer = std::shared_ptr<Database>;
        using Vector = ShowLib::JSONSerializableVector<Database>;

        static const char * DRIVER_POSTGRESQL;

        virtual ~Database();

        void fromJSON(const JSON &) override;
        JSON toJSON() const override;

        std::string getEnvName() const { return envName; }
        std::string getDriver() const { return driver; }
        std::string getHost() const { return host; }
        int getPort() const { return port; }
        std::string getDbName() const { return dbName; }
        std::string getUsername() const { return username; }
        std::string getPassword() const { return password; }

        Database & setEnvName(const std::string &value) { envName = value; return *this; }
        Database & setDriver(const std::string &value) { driver = value; return *this; }
        Database & setHost(const std::string &value) { host = value; return *this; }
        Database & setPort(int value) { port = value; return *this; }
        Database & setDbName(const std::string &value) { dbName = value; return *this; }
        Database & setUsername(const std::string &value) { username = value; return *this; }
        Database & setPassword(const std::string &value) { password = value; return *this; }

    private:
        std::string envName = "default";
        std::string driver = "PostgreSql";
        std::string host;
        int port = 0;
        std::string dbName;
        std::string username;
        std::string password;
    };

    //======================================================================
    // Methods
    //======================================================================
    bool deepEquals(const DataModel &orig) const;

    void fromJSON(const JSON &) override;
    JSON toJSON() const override;

    int getGeneratedVersion() const { return generatedVersion; }
    DataModel & setGeneratedVersion(int value) { generatedVersion = value; return *this; }

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
    Column::Vector findReferencesTo(const Table &table);

    bool getIsDirty() const { return isDirty; }
    void markDirty() { isDirty = true; }
    void markClean() { isDirty = false; }

    const Generator::Vector & getGenerators() const { return generators; }
    void pushGenerator(DataModel::Generator::Pointer);

    const Database::Vector & getDatabases() const { return databases; }
    void pushDatabase(DataModel::Database::Pointer);

private:
    std::string name;
    Table::Vector tables;
    Generator::Vector generators;
    Database::Vector databases;
    bool isDirty = false;
    int generatedVersion = 0;
};

std::string toString(DataModel::Column::DataType dt);
DataModel::Column::DataType toDataType(const std::string &);
bool dataTypeHasLength(DataModel::Column::DataType dt);
bool dataTypeHasPrecision(DataModel::Column::DataType dt);
bool dataTypeIsSerial(DataModel::Column::DataType dt);
std::string cTypeFor(DataModel::Column::DataType dt);

std::vector<DataModel::Column::DataTypePair> & allDataTypes();


