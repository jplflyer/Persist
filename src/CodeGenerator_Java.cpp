#include <iostream>
#include <fstream>
#include <filesystem>

#include <showlib/CommonUsing.h>
#include <showlib/StringUtils.h>

#include "CodeGenerator_Java.h"

using Table = DataModel::Table;
using Column = DataModel::Column;


/**
 * Constructor.
 */
CodeGenerator_Java::CodeGenerator_Java(DataModel &m, DataModel::Generator::Pointer genInfo)
    : CodeGenerator("CodeGenerator_Java", m, genInfo)
{
}

/**
 * Generate our Java source code. For now, this is just Pojos, but I'll figure out more as I go.
 */
void CodeGenerator_Java::generate() {
    slashedClassPath = generatorInfo->getOutputClassPath();
    std::replace(slashedClassPath.begin(), slashedClassPath.end(), '.', '/');

    const std::unordered_map<std::string, std::string> & options = generatorInfo->getOptions();
    for (auto const& [key, value] : options) {
        if (key == "userTable") {
            userTableName = value;
        }
    }

    for (const Table::Pointer & table: model.getTables()) {
        generatePOJO(table);
        generateRepository(table);
    }
}

/**
 * Generate the POJO. Pojos go in the basePath + "/dbmodel/" + tableName.java
 */
void CodeGenerator_Java::generatePOJO(DataModel::Table::Pointer table) {
    bool isMemberTable = table->getName() == userTableName;
    string basePath = generatorInfo->getOutputBasePath() + "/" + slashedClassPath + "/dbmodel/";
    string path = basePath + table->getName() + ".java";

    std::filesystem::create_directories(basePath);

    std::ofstream ofs{path};

    ofs << "package " << generatorInfo->getOutputClassPath() << ".dbmodel;\n"
        << "\n"
        << "import jakarta.persistence.*;\n"
        << "import lombok.AllArgsConstructor;\n"
        << "import lombok.Builder;\n"
        << "import lombok.Data;\n"
        << "import lombok.NoArgsConstructor;\n"
        ;
    for (const Column::Pointer & column: table->getColumns()) {
        if (javaType(column->getDataType()) == "LocalDateTime") {
            ofs << "import java.time.LocalDateTime;\n";
            break;
        }
    }

    string implementsPortion = "";
    if (isMemberTable) {
        ofs << "import org.springframework.security.core.userdetails.UserDetails;\n"
            << "import org.springframework.security.core.GrantedAuthority;\n"
            << "import org.springframework.security.core.authority.SimpleGrantedAuthority;\n"
            << "import java.util.Collection;\n"
            << "import java.util.List;\n"
            ;

        implementsPortion = " implements UserDetails";
    }

   ofs  << "\n"
        << "@Entity\n"
        << "@Data\n"
        << "@NoArgsConstructor\n"
        << "@AllArgsConstructor\n"
        << "@Builder\n"
        << "public class " << table->getName() << implementsPortion << " {\n"
        ;

    for (const Column::Pointer & column: table->getColumns()) {
        if (column->getIsPrimaryKey()) {
            string seqName = table->getDbName() + "_" + column->getDbName() + "_seq";
            ofs << "    @Id\n"
                << "    @GeneratedValue(strategy=GenerationType.AUTO, generator=\"" << seqName << "\")\n"
                << "    @SequenceGenerator(name=\"" << seqName << "\", sequenceName=\"" << seqName << "\", allocationSize = 1)\n"
                ;
        }
        ofs << "    " << javaType(column->getDataType()) << " " << column->getName() << ";\n";
    }

    if (isMemberTable) {
        ofs << "\n"
            << "@Override\n"
            << "    public Collection<? extends GrantedAuthority> getAuthorities() {\n"
            << "        return List.of(new SimpleGrantedAuthority( isAdmin ? \"ADMIN\" : \"MEMBER\"));\n"
            << "     }\n"
            << "\n"

            << "    @Override\n"
            << "    public boolean isAccountNonExpired() {\n"
            << "        return true;\n"
            << "    }\n"
            << "\n"

            << "    @Override\n"
            << "    public boolean isAccountNonLocked() {\n"
            << "        return true;\n"
            << "    }\n"
            << "\n"

            << "    @Override\n"
            << "    public boolean isCredentialsNonExpired() {\n"
            << "        return true;\n"
            << "    }\n"
            << "\n"

            << "    @Override\n"
            << "    public boolean isEnabled() {\n"
            << "        return true;\n"
            << "    }\n"
            << "    \n"
            ;

    }

    ofs << "}\n"
        ;
}

/**
 * We assume Spring Data. Generate the corresponding repository.
 */
void CodeGenerator_Java::generateRepository(Table::Pointer table) {
    string basePath = generatorInfo->getOutputBasePath() + "/" + slashedClassPath  + "/repository/";
    string path = basePath + table->getName() + "Repository.java";
    bool wantOptional = false;

    for (const Column::Pointer & column: table->getColumns()) {
        if (column->getWantFinder()) {
            wantOptional = true;
            break;
        }
    }

    std::filesystem::create_directories(basePath);

    std::ofstream ofs{path};

    ofs << "package " << generatorInfo->getOutputClassPath() << ".repository;\n"
        << "\n"
        << "import org.springframework.data.jpa.repository.JpaRepository;\n"
        << "import " << generatorInfo->getOutputClassPath() << ".dbmodel." << table->getName() << ";\n"
        ;

    if (wantOptional) {
        ofs << "import java.util.Optional;\n";
    }

    ofs << "\n"
        << "public interface " << table->getName() << "Repository extends JpaRepository<" << table->getName() << ", Integer> {\n"
        ;
    for (const Column::Pointer & column: table->getColumns()) {
        if (column->getWantFinder()) {
            ofs << "    Optional<" << table->getName() << "> findBy" << ShowLib::firstUpper(column->getName()) << "(String username);\n";
        }
    }
    ofs << "}\n"
        ;
}

//======================================================================
// Helpers.
//======================================================================

/**
 * From the datatype, what Java type matches it. This list isn't perfect.
 */
std::string CodeGenerator_Java::javaType(DataModel::Column::DataType dt) {
    switch (dt) {
        // Numeric types.
        case Column::DataType::BigInt:			return "Long";
        case Column::DataType::BigSerial:		return "Long";
        case Column::DataType::SmallInt:		return "Short";
        case Column::DataType::Serial:			return "Integer";
        case Column::DataType::Boolean:			return "Boolean";
        case Column::DataType::Double:			return "Double";
        case Column::DataType::Integer:			return "Integer";
        case Column::DataType::Real:			return "Float";
        case Column::DataType::Numeric:			return "Double";

        // Character data
        case Column::DataType::ByteArray:		return "byte[]";
        case Column::DataType::Character:		return "String";
        case Column::DataType::VarChar:			return "String";
        case Column::DataType::Text:			return "String";

        // Some of these aren't right.
        case Column::DataType::Interval:		return "Timestamp";
        case Column::DataType::Date:			return "LocalDateTime";
        case Column::DataType::Time:			return "LocalTime";
        case Column::DataType::TimeTZ:			return "LocalTime";
        case Column::DataType::Timestamp:		return "Timestamp";
        case Column::DataType::TimestampTZ:		return "Timestamp";

        // Bit is fine but VarBit is probably not.
        case Column::DataType::Bit:				return "Boolean";
        case Column::DataType::VarBit:			return "Integer";
    }
}
