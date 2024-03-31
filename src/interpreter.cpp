#include "interpreter.h"

#include <spdlog/spdlog.h>
#include <random>

constexpr double kTimerFrequency = 1 / 60.0;

Interpreter::Interpreter(std::shared_ptr<registers> regs) : regs(regs) {}

void Interpreter::initialize() {
    timer.tick();
}

void Interpreter::update() {
    update_timers();
}

void Interpreter::cleanup() {
}

void Interpreter::step() {
    uint16_t instr = regs->mem[regs->pc];
    regs->pc += 2;

    uint8_t first = instr >> 12;
    uint8_t x = (instr >> 8) & 0xf;
    uint8_t y = (instr >> 4) & 0xf;
    uint8_t n = instr & 0xf;
    uint8_t nn = instr & 0xff;
    uint8_t nnn = instr & 0xfff;

    switch (first) {
    case 0x0:
        if (instr == 0x00e0) {
            // clear screen
        } else if (instr == 0x00ee) {
            // returns from subroutine
            regs->pc = stack_pop();
        } else {
            // call machine routine at NNN
            // unimplemented
        }
        break;
    case 0x1:
        // jumps to address NNN
        regs->pc = nnn;
        break;
    case 0x2:
        // calls subroutine at NNN
        stack_push(regs->pc);
        regs->pc = nnn;
        break;
    case 0x3:
        // skips next instruction if VX == NN
        if (regs->v[x] == nn) {
            regs->pc += 2;
        }
        break;
    case 0x4:
        // skips next instruction if VX != NN
        if (regs->v[x] != nn) {
            regs->pc += 2;
        }
        break;
    case 0x5:
        // skips next instruction if VX == VY
        if (regs->v[x] == regs->v[y]) {
            regs->pc += 2;
        }
        break;
    case 0x6:
        // sets VX to NN
        regs->v[x] = nn;
        break;
    case 0x7:
        // adds NN to VX (carry flag not changed)
        regs->v[x] += nn;
        break;
    case 0x8:
        switch (n) {
        case 0x0:
            // sets VX to VY
            regs->v[x] = regs->v[y];
            break;
        case 0x1:
            // sets VX to VX | VY
            regs->v[x] = regs->v[x] | regs->v[y];
            break;
        case 0x2:
            // sets VX to VX & VY
            regs->v[x] = regs->v[x] & regs->v[y];
            break;
        case 0x3:
            // sets VX to VX xor VY
            regs->v[x] = regs->v[x] xor regs->v[y];
        case 0x4:
            // adds VY to VX, set VF to 1 if overflow else 0
            {
                uint16_t val = regs->v[x] + regs->v[y];
                regs->v[0xf] = val >> 8;
                regs->v[x] = val & 0xff;
            }
            break;
        case 0x5:
            // subtract VY from VX, set VF to 0 if underflow else 1
            regs->v[0xf] = regs->v[x] >= regs->v[y] ? 1 : 0;
            regs->v[x] -= regs->v[y];
            break;
        case 0x6:
            // store LSB of VX in VF, right shift VX by 1
            regs->v[0xf] = regs->v[x] & 0x1;
            regs->v[x] >>= 1;
            break;
        case 0x7:
            // sets VX to VY - VX, set VF to 0 if underflow else 1
            regs->v[0xf] = regs->v[y] >= regs->v[x] ? 1 : 0;
            regs->v[x] = regs->v[y] - regs->v[x];
            break;
        case 0xe:
            // store MSB of VX in VF, left shift VX by 1
            regs->v[0xf] = regs->v[x] & 0x80;
            regs->v[x] <<= 1;
            break;
        }
        break;
    case 0x9:
        // skips next instruction if VX != VY
        if (regs->v[x] != regs->v[y]) {
            regs->pc += 2;
        }
        break;
    case 0xa:
        // sets I to NNN
        regs->i = nnn;
        break;
    case 0xb:
        // jumps to address NNN + V0
        regs->pc = nnn + regs->v[0];
        // TODO: CHIP-48 & SUPER-CHIP - BXNN jump to XNN + VX
        break;
    case 0xc:
        // sets VX to rand() & NN
        regs->v[x] = random_byte() & nn;
        break;
    case 0xd:
        // draws sprite at coord (VX, VY)
        break;
    case 0xe:
        switch (nn) {
        case 0x9e:
            // skips next instruction if key stored in VX is pressed
            break;
        case 0xa1:
            // skips next instruction if key stored in VX is not pressed
            break;
        }
        break;
    case 0xf:
        {
            switch (nn) {
                case 0x07:
                    // sets VX to delay timer
                    regs->v[x] = regs->dt;
                    break;
                case 0x0a:
                    // wait for keypress, store in VX
                    {
                        auto key = get_pressed_key();
                        if (!key.has_value()) {
                            regs->pc -= 2;
                        } else {
                            regs->v[x] = key.value();
                        }
                    }
                    break;
                case 0x15:
                    // set delay timer to VX
                    regs->dt = regs->v[x];
                    break;
                case 0x18:
                    // set sound timer to VX
                    regs->st = regs->v[x];
                    break;
                case 0x1e:
                    // adds VX to I, VF unchanged
                    regs->i += regs->v[x];
                    break;
                case 0x29:
                    // sets I to for sprite character in VX
                    break;
                case 0x33:
                    // stores BCD repr of VX at address I
                    {
                        uint8_t i = regs->i;
                        uint8_t val = regs->v[x];
                        regs->mem[i] = val / 100;
                        regs->mem[i + 1] = (val / 10) % 10;
                        regs->mem[i + 2] = val % 10;
                    }
                    break;
                case 0x55:
                    // stores V0 to VX (inclusive) in memory starting at address I
                    {
                        uint8_t i = regs->i;
                        for (int n = 0; n < x; i += 1) {
                            regs->mem[i + n] = regs->v[n];
                        }
                    }
                    break;
                case 0x65:
                    // fills V0 to VX (inclusive) with values from memory starting at I
                    {
                        uint8_t i = regs->i;
                        for (int n = 0; n < x; i += 1) {
                            regs->v[n] = regs->mem[i + n];
                        }
                    }
                    break;
            }
        }
        break;
    }
}

void Interpreter::update_timers() {
    last_tick += timer.tick();
    while (last_tick > kTimerFrequency) {
        if (regs->dt > 0) {
            regs->dt -= 1;
        }
        if (regs->st > 0) {
            regs->st -= 1;
        }

        last_tick -= kTimerFrequency;
    }
}

void Interpreter::stack_push(uint16_t val) {
    regs->stack[regs->sp++] = val;
}

uint16_t Interpreter::stack_pop() {
    return regs->stack[--regs->sp];
}

uint8_t Interpreter::random_byte() {
    static std::random_device rd;
    static std::mt19937 gen(rd());
    static std::uniform_int_distribution<> dist(0, 255);
    return dist(gen);
}

tl::optional<uint8_t> Interpreter::get_pressed_key() {
    return tl::nullopt;
}
