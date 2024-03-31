#pragma once

#include "interpreter.h"

#include <raylib.h>
#include <memory>

class Interface {
public:
    Interface(std::shared_ptr<registers> regs);

    void initialize();
    bool update();
    void cleanup();

public:
    std::shared_ptr<registers> regs;
    RenderTexture2D screen_texture;
};
