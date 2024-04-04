#include "interpreter.h"
#include "random.h"

#include <_types/_uint16_t.h>
#include <_types/_uint8_t.h>
#include <optional>
#include <spdlog/spdlog.h>

constexpr double kTimerFrequency = 1 / 60.0;

const int kStackPtrIndex = 0x0;
const int kStackStartIndex = 0x10;
const int kFontStartIndex = 0x50;
const int kRomStartIndex = 0x200;

Interpreter::Interpreter(std::shared_ptr<registers> regs) : regs(regs) {}

void Interpreter::initialize()
{
  timer.tick();
  reset();

  spdlog::info("Initialized interpreter");
}

void Interpreter::update()
{
  update_timers();
  if (playing)
  {
    step();
  }
}

void Interpreter::cleanup() {
  spdlog::info("Cleaning up interpreter");
}

void Interpreter::load_rom_bytes(const std::vector<uint8_t> &bytes)
{
  std::copy(bytes.begin(), bytes.end(), regs->mem.begin() + kRomStartIndex);
}

void Interpreter::reset()
{
  regs->reset();

  regs->pc = 0x200;
  init_font_sprites();
}

void Interpreter::play() { playing = true; }

void Interpreter::stop() { playing = false; }

bool Interpreter::is_playing() const { return playing; }

void Interpreter::step()
{
  uint8_t instr_hi = regs->mem[regs->pc];
  uint8_t instr_lo = regs->mem[regs->pc + 1];

  regs->pc += 2;

  uint16_t instr = instr_lo | (instr_hi << 8);
  uint8_t first = instr_hi >> 4;
  uint8_t x = instr_hi & 0xf;
  uint8_t y = instr_lo >> 4;
  uint8_t n = instr & 0xf;
  uint8_t nn = instr_lo;
  uint16_t nnn = (instr_lo | (instr_hi << 8)) & 0xfff;

  switch (first)
  {
  case 0x0:
    if (instr == 0x00e0)
    {
      // clear screen
      screen_clear();
    }
    else if (instr == 0x00ee)
    {
      // returns from subroutine
      regs->pc = stack_pop();
    }
    else
    {
      // call machine routine at NNN
      // unimplemented
      spdlog::warn("Unimplemented instruction 1NNN: {:x}", nnn);
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
    if (regs->v[x] == nn)
    {
      regs->pc += 2;
    }
    break;
  case 0x4:
    // skips next instruction if VX != NN
    if (regs->v[x] != nn)
    {
      regs->pc += 2;
    }
    break;
  case 0x5:
    // skips next instruction if VX == VY
    if (regs->v[x] == regs->v[y])
    {
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
    switch (n)
    {
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
      break;
    case 0x4:
      // adds VY to VX, set VF to 1 if overflow else 0
      {
        uint16_t val = regs->v[x] + regs->v[y];
        regs->v[x] = val & 0xff;
        regs->v[0xf] = val >> 8;
      }
      break;
    case 0x5:
      // subtract VY from VX, set VF to 0 if underflow else 1
      {
        uint8_t result = regs->v[x] >= regs->v[y] ? 1 : 0;
        regs->v[x] -= regs->v[y];
        regs->v[0xf] = result;
      }
      break;
    case 0x6:
      // store LSB of VX in VF, right shift VX by 1
      {
        uint8_t lsb = regs->v[x] & 0x1;
        regs->v[x] >>= 1;
        regs->v[0xf] = lsb;
      }
      break;
    case 0x7:
      // sets VX to VY - VX, set VF to 0 if underflow else 1
      regs->v[x] = regs->v[y] - regs->v[x];
      regs->v[0xf] = regs->v[y] >= regs->v[x] ? 1 : 0;
      break;
    case 0xe:
      // store MSB of VX in VF, left shift VX by 1
      {
        uint8_t msb = (regs->v[x] & 0x8) >> 3;
        regs->v[x] <<= 1;
        regs->v[0xf] = msb;
      }
      break;
    }
    break;
  case 0x9:
    // skips next instruction if VX != VY
    if (regs->v[x] != regs->v[y])
    {
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
    screen_draw_sprite(regs->v[x], regs->v[y], n);
    break;
  case 0xe:
    switch (nn)
    {
    case 0x9e:
      // skips next instruction if key stored in VX is pressed
      if (is_key_pressed(regs->v[x]))
      {
        regs->pc += 2;
      }
      break;
    case 0xa1:
      // skips next instruction if key stored in VX is not pressed
      if (!is_key_pressed(regs->v[x]))
      {
        regs->pc += 2;
      }
      break;
    }
    break;
  case 0xf:
  {
    switch (nn)
    {
    case 0x07:
      // sets VX to delay timer
      regs->v[x] = regs->dt;
      break;
    case 0x0a:
      // wait for keypress, store in VX
      {
        auto key = get_pressed_key();
        if (!key.has_value())
        {
          regs->pc -= 2;
        }
        else
        {
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
      regs->i = get_font_sprite_addr(regs->v[x]);
      break;
    case 0x33:
      // stores BCD repr of VX at address I
      {
        int i = regs->i;
        uint8_t val = regs->v[x];
        regs->mem[i] = val / 100;
        regs->mem[i + 1] = (val / 10) % 10;
        regs->mem[i + 2] = val % 10;
      }
      break;
    case 0x55:
      // stores V0 to VX (inclusive) in memory starting at address I
      for (int n = 0, i = regs->i; n <= x; i++, n++)
      {
        regs->mem[i] = regs->v[n];
      }
      break;
    case 0x65:
      // fills V0 to VX (inclusive) with values from memory starting at I
      for (int n = 0, i = regs->i; n <= x; n++, i++)
      {
        regs->v[n] = regs->mem[i];
      }
      break;
    }
  } break;
  }
}

void Interpreter::update_timers()
{
  last_tick += timer.tick();
  while (last_tick > kTimerFrequency)
  {
    if (regs->dt > 0)
    {
      regs->dt -= 1;
    }
    if (regs->st > 0)
    {
      regs->st -= 1;
    }

    last_tick -= kTimerFrequency;
  }
}

void Interpreter::stack_push(uint16_t val) {
  uint8_t *sp = &regs->mem[kStackPtrIndex];
  uint16_t *stack = reinterpret_cast<uint16_t*>(&regs->mem[kStackStartIndex]);
  stack[(*sp)++] = val;
}

uint16_t Interpreter::stack_pop() {
  uint8_t *sp = &regs->mem[kStackPtrIndex];
  uint16_t *stack = reinterpret_cast<uint16_t*>(&regs->mem[kStackStartIndex]);
  return stack[--(*sp)];
}

std::optional<uint8_t> Interpreter::get_pressed_key()
{
  for (int key = 0; key < regs->kbd.size(); key += 1)
  {
    if (regs->kbd[key])
    {
      return key;
    }
  }
  return std::nullopt;
}

bool Interpreter::is_key_pressed(uint8_t key) { return regs->kbd[key]; }

void Interpreter::screen_clear()
{
  for (int i = 0; i < regs->screen.size(); i += 1)
  {
    regs->screen[i] = false;
  }
}

void Interpreter::screen_draw_sprite(int x, int y, int n)
{
  x = x % kScreenWidth;
  y = y % kScreenHeight;

  regs->v[0xf] = 0;

  uint16_t start = regs->i;
  for (int j = 0; j < n; j += 1)
  {
    uint16_t i = start + j;
    uint16_t sx = x + 8;
    uint16_t sy = y + j;
    uint8_t row = regs->mem[i];

    for (int b = 0; b < 8; b += 1)
    {
      if (row & 1)
      {
        screen_flip_pixel_at(sx, sy);
      }

      row >>= 1;
      sx -= 1;
    }
  }
}

void Interpreter::screen_flip_pixel_at(int x, int y)
{
  if (x < 0 || x >= kScreenWidth || y < 0 || y >= kScreenHeight)
  {
    return;
  }

  int idx = (y * kScreenWidth) + x;
  regs->screen[idx] = !regs->screen[idx];
}

void Interpreter::init_font_sprites()
{
  uint16_t idx = kFontStartIndex;

  auto load_sprite = [&](std::array<uint8_t, 5> bytes)
  {
    for (int i = 0; i < 5; i += 1)
    {
      regs->mem[idx++] = bytes[i];
    }
  };

  // 0
  load_sprite({0xf0, 0x90, 0x90, 0x90, 0xf0});

  // 1
  load_sprite({0x20, 0x60, 0x20, 0x20, 0x70});

  // 2
  load_sprite({0xf0, 0x10, 0xf0, 0x80, 0xf0});

  // 3
  load_sprite({0xf0, 0x10, 0xf0, 0x10, 0xf0});

  // 4
  load_sprite({0x90, 0x90, 0xf0, 0x10, 0x10});

  // 5
  load_sprite({0xf0, 0x80, 0xf0, 0x10, 0xf0});

  // 6
  load_sprite({0xf0, 0x80, 0xf0, 0x90, 0xf0});

  // 7
  load_sprite({0xf0, 0x10, 0x20, 0x40, 0x40});

  // 8
  load_sprite({0xf0, 0x90, 0xf0, 0x90, 0xf0});

  // 9
  load_sprite({0xf0, 0x90, 0xf0, 0x10, 0xf0});

  // A
  load_sprite({0xf0, 0x90, 0xf0, 0x90, 0x90});

  // B
  load_sprite({0xe0, 0x90, 0xe0, 0x90, 0xe0});

  // C
  load_sprite({0xf0, 0x80, 0x80, 0x80, 0xf0});

  // D
  load_sprite({0xe0, 0x90, 0x90, 0x90, 0xe0});

  // E
  load_sprite({0xf0, 0x80, 0xf0, 0x80, 0xf0});

  // F
  load_sprite({0xf0, 0x80, 0xf0, 0x80, 0x80});
}

uint16_t Interpreter::get_font_sprite_addr(uint8_t c)
{
  return regs->mem[kFontStartIndex + (c * 5)];
}
