#pragma once

#include "interpreter.h"

class Interface {
public:
    Interface(Interpreter* interpreter);

    void initialize();
    bool update();
    void cleanup();

public:
    Interpreter* interpreter;
};
