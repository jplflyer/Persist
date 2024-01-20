#pragma once

#include "CodeGenerator_SQL.h"

/**
 * We support flyway for database migrations. This is tricky.
 */
class CodeGenerator_Flyway : public CodeGenerator_SQL
{
public:
    using Database = DataModel::Database;
    using Table = DataModel::Table;
    using Column = DataModel::Column;

    CodeGenerator_Flyway(DataModel &, DataModel::Generator::Pointer genInfo);
    void generate() override;

private:
    void generate_ConfigFiles();
    bool generate_Migrations();

    bool generate_TableMigrations(std::ofstream &ofs, const Table::Pointer &);
    bool generate_TableNameChanges(std::ofstream &ofs, const Table::Pointer &);
    bool generate_ColumnChanges(std::ofstream &ofs, const Table::Pointer &);

    void setGeneratedNames(Table &);
    void saveModel();

    std::string migrationFileName(const std::string &comment);

    int sequence = 0;
    int genVersion;
};

