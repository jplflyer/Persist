#pragma once

#include <string>
#include "DataModel.h"

/**
 * This is the base class for all code generators.
 */
class CodeGenerator
{
public:
    CodeGenerator() = default;

    virtual void generate(DataModel &) = 0;

    std::string name;
    std::string outputFileName;
};

