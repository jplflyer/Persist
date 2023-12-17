#include "CodeGenerator.h"

CodeGenerator::CodeGenerator(const std::string &_name, DataModel &m, DataModel::Generator::Pointer genInfo)
    : model(m), generatorInfo(genInfo), name(_name)
{
}
