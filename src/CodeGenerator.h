#pragma once

#include <string>
#include "DataModel.h"

/**
 * This is the base class for all code generators.
 */
class CodeGenerator
{
public:
    CodeGenerator(const std::string & _name, DataModel &_model, DataModel::Generator::Pointer genInfo);

    virtual void generate() = 0;

    std::string name;

protected:
    DataModel &model;
    DataModel::Generator::Pointer generatorInfo;

};

