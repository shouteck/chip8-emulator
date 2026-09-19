#pragma once
#include <cstdint>
#include <string>

class Chip8 {
public:
    Chip8();

    bool loadRom(const std::string& path);
    void cycle();
    void tickTimers();
    uint8_t peek(uint16_t addr) const { return memory[addr]; }

    uint32_t display[64 * 32] = {};
    uint8_t keypad[16] = {};
    void keyDown(uint8_t k);
    void keyUp(uint8_t k);

private:
    uint8_t memory[4096] = {};
    uint8_t V[16] = {};
    uint16_t I = 0;
    uint16_t pc = 0x200;
    uint16_t stack[16] = {};
    uint8_t sp = 0;
    uint8_t delayTimer = 0;
    uint8_t soundTimer = 0;
    bool waitingForKey = false;
    uint8_t waitingReg = 0;
};
