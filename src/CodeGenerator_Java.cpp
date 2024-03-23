
#include <fstream>
#include <filesystem>

#include <showlib/CommonUsing.h>
#include <showlib/StringUtils.h>

#include "CodeGenerator_Java.h"

using Table = DataModel::Table;
using Column = DataModel::Column;

using ShowLib::StringVector;


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
        else if (key == "withSpringTags") {
            withSpringTags = value == "true";
        }
        else if (key == "extends") {
            extendsList.tokenize(value, ',');
            for (const StringVector::Pointer &strP: extendsList) {
                ShowLib::trimInPlace(*strP);
            }
        }
        else if (key == "implements") {
            implementsList.tokenize(value, ',');
            for (const StringVector::Pointer &strP: implementsList) {
                ShowLib::trimInPlace(*strP);
            }
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
    Column::Vector foreignRefs = model.findReferencesTo(*table);
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
        << "import lombok.experimental.Accessors;\n"
        << "import com.fasterxml.jackson.annotation.JsonIgnore;\n";
        ;

    for (const Column::Pointer & column: table->getColumns()) {
        if (javaType(column->getDataType()) == "LocalDateTime") {
            ofs << "import java.time.LocalDateTime;\n";
            break;
        }
    }

    if (isMemberTable) {
        ofs << "import org.springframework.security.core.userdetails.UserDetails;\n"
            << "import org.springframework.security.core.GrantedAuthority;\n"
            << "import org.springframework.security.core.authority.SimpleGrantedAuthority;\n"
            << "import java.util.Collection;\n"
            << "import java.util.List;\n"
            ;
    }
    if (!foreignRefs.empty()) {
        ofs << "import java.util.Set;\n";
    }
    for (const Column::Pointer & column: table->getColumns()) {
        if (column->isTimestamp()) {
            ofs << "import java.sql.Timestamp;\n";
            break;
        }
    }

    ofs  << "\n"
         << "@Entity\n"
         << "@Data\n"
         << "@Accessors(chain = true)\n"
         << "@NoArgsConstructor\n"
         << "@AllArgsConstructor\n"
         << "@Builder\n"
         << "public class " << table->getName()
         ;

    if ( !extendsList.empty()) {
        string delim = " ";
        ofs << " extends";
        for (const StringVector::Pointer &strP: extendsList) {
            if (ShowLib::endsWith(*strP, "<?>")) {
                ofs << delim << strP->substr(0, strP->size() - 2) << "<" << table->getName() << ">";
            }
            else {
                ofs << delim << *strP;
            }
            delim = ", ";
        }
    }

    if ( implementsList.empty()) {
        if (isMemberTable) {
            ofs << " implements UserDetails";
        }
    }
    else {
        string delim = " ";
        ofs << " implements";
        for (const StringVector::Pointer &strP: implementsList) {
            if (ShowLib::endsWith(*strP, "<?>")) {
                ofs << delim << strP->substr(0, strP->size() - 3) << "<" << table->getName() << ">";
            }
            else {
                ofs << delim << *strP;
            }
            delim = ", ";
        }
        if (isMemberTable) {
            ofs << delim << "UserDetails";
        }
    }

    ofs << "\n{\n";

    for (const Column::Pointer & column: table->getColumns()) {
        if (column->getIsPrimaryKey()) {
            string seqName = table->getDbName() + "_" + column->getDbName() + "_seq";
            ofs << "    @Id\n"
                << "    @GeneratedValue(strategy=GenerationType.AUTO, generator=\"" << seqName << "\")\n"
                << "    @SequenceGenerator(name=\"" << seqName << "\", sequenceName=\"" << seqName << "\", allocationSize = 1)\n"
                ;
        }

        if (column->isForeignKey()) {
            generateForeignKey(ofs, column);
        }

        if (!column->getSerialize()) {
            ofs << "    @JsonIgnore\n";
        }
        ofs << "    " << javaType(column->getDataType()) << " " << column->getName() << ";\n"
            << "\n"
            ;
    }

    if (isMemberTable) {
        ofs << "\n"
            << "    @Override\n"
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

    // We also want reverse references. Remote tables that reference our primary key,
    // a OneToMany relationship.
    for (const Column::Pointer &col: foreignRefs) {
        Table::Pointer refTable = col->getOurTable().lock();
        string refName = col->getReversePtrName().empty()
             ? ShowLib::firstLower(refTable->getName()) + "s"
             : col->getReversePtrName();

        ofs << "\n"
            << "	@JsonIgnore\n"
            << "    @OneToMany(mappedBy =\"" << col->getName() << "\")\n"
            << "    private Set<" << refTable->getName() << "> " << refName << ";\n"
            ;
    }

    ofs << "}\n"
        ;
}

/**
 * Spring Data likes foreign key references to look like this:
 *
 *  @ManyToOne(fetch = FetchType.LAZY)
 *  @JoinColumn(name = "tutorial_id")
 *  @JsonIgnore
 *  private Tutorial tutorial;
 *
 * Plus we want to expose the key, anyway.
 *
 *  @Column(insertable = false, updatable = false)
 *  Integer tutorialId;
 */
void CodeGenerator_Java::generateForeignKey(std::ofstream & ofs, DataModel::Column::Pointer column) {
    Column::Pointer remoteColumn = column->getReferences();
    Table::Pointer remoteTable = remoteColumn->getOurTable().lock();
    string name = column->getRefPtrName();
    if (name.empty()) {
        name = ShowLib::firstLower(remoteTable->getName());
    }

    ofs << "    @ManyToOne(fetch = FetchType.LAZY)\n"
        << "    @JoinColumn(name = \"" << column->getDbName() << "\")\n"
        << "    @JsonIgnore\n"
        << "    private " << remoteTable->getName() << " " << name << ";\n"
        << "\n"
        << "    @Column(name = \"" << column->getDbName() << "\", insertable = false, updatable = false)\n"
        ;

    // The FK field (memberId, whatever), itself will be generated by the caller.
}


/**
 * We assume Spring Data. Generate the corresponding repository.
 */
void CodeGenerator_Java::generateRepository(Table::Pointer table) {
    string basePath = generatorInfo->getOutputBasePath() + "/" + slashedClassPath  + "/repository/";
    string path = basePath + table->getName() + "Repository.java";

    std::filesystem::create_directories(basePath);

    if (std::filesystem::exists(path)) {
        return;
    }

    std::ofstream ofs{path};

    ofs << "package " << generatorInfo->getOutputClassPath() << ".repository;\n"
        << "\n"
        << "import java.util.List;\n"
        << "import java.util.Optional;\n"
        << "import org.springframework.data.jpa.repository.JpaRepository;\n"
        << "import " << generatorInfo->getOutputClassPath() << ".dbmodel." << table->getName() << ";\n"
        ;


    ofs << "\n"
        << "public interface " << table->getName() << "Repository extends JpaRepository<" << table->getName() << ", Integer> {\n"
        ;

    for (const Column::Pointer & column: table->getColumns()) {
        //
        // This is going to look like one of these:
        //
        //		Optional<Foo> getByBlah(Integer blah);
        //		List<Foo> getByBlah(Integer blah);
        //
        if (column->getWantFinder()) {
            ofs << "    public " << (column->isForeignKey() ? "List<" : "Optional<")
                << table->getName()
                << "> findBy" << ShowLib::firstUpper(column->getName())
                << "(" << javaType(column->getDataType()) << " " << column->getName() << ");\n";
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

        // This shouldn't happen.
        case Column::DataType::Unknown:			return "Integer";
    }
}
