#pragma once

#include <string>
#include "DataModel.h"

/**
 * This is the base class for all code generators.
 */
class CodeGenerator
{
protected:
    DataModel &model;

public:
    CodeGenerator(const std::string & _name, DataModel &_model);

    virtual void generate() = 0;

    std::string name;
    std::string outputFileName;
};

