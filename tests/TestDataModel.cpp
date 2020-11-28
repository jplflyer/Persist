#include "TestDataModel.h"

CPPUNIT_TEST_SUITE_REGISTRATION(TestDataModel);

using namespace showlib;
using std::string;
using std::cout;
using std::endl;

using Table = DataModel::Table;
using Column = DataModel::Column;

string TEST_NAME = "TestDataModel";

void
TestDataModel::testBasic() {
    DataModel dmCreate;
    DataModel dmRead;

    Table::Pointer memberTable = dmCreate.createTable("members");
    Table::Pointer privTable = dmCreate.createTable("privs");

    Column::Pointer memberId = memberTable->createColumn("id", DataModel::Column::DataType::SmallSerial);
    memberId->setIsPrimaryKey(true)
            .setNullable(false)
            ;

    JSON json = dmCreate.getJSON();
    dmRead.fromJSON(json);

    DataModel::Table::Pointer  table = dmRead.findTable("members");

    CPPUNIT_ASSERT(dmCreate.deepEquals(dmRead));
    CPPUNIT_ASSERT(table != nullptr);

}
