#pragma once

#include "CodeGenerator.h"

/**
 * This generates DB access code. You'll need to subclass it for specific
 * SQL libraries.
 */
class CodeGenerator_DB: public CodeGenerator
{
public:
    CodeGenerator_DB();
};

