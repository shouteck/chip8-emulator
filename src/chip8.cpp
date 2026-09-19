#include "chip8.h"
#include <fstream>
#include <cstdio>
#include <cstdlib>
#include <ctime>

namespace {
constexpr uint16_t FONTSET_START = 0x050;
constexpr uint16_t ROM_START = 0x200;

constexpr uint8_t FONTSET[80] = {
    0xF0, 0x90, 0x90, 0x90, 0xF0, // 0
    0x20, 0x60, 0x20, 0x20, 0x70, // 1
    0xF0, 0x10, 0xF0, 0x80, 0xF0, // 2
    0xF0, 0x10, 0xF0, 0x10, 0xF0, // 3
    0x90, 0x90, 0xF0, 0x10, 0x10, // 4
    0xF0, 0x80, 0xF0, 0x10, 0xF0, // 5
    0xF0, 0x80, 0xF0, 0x90, 0xF0, // 6
    0xF0, 0x10, 0x20, 0x40, 0x40, // 7
    0xF0, 0x90, 0xF0, 0x90, 0xF0, // 8
    0xF0, 0x90, 0xF0, 0x10, 0xF0, // 9
    0xF0, 0x90, 0xF0, 0x90, 0x90, // A
    0xE0, 0x90, 0xE0, 0x90, 0xE0, // B
    0xF0, 0x80, 0x80, 0x80, 0xF0, // C
    0xE0, 0x90, 0x90, 0x90, 0xE0, // D
    0xF0, 0x80, 0xF0, 0x80, 0xF0, // E
    0xF0, 0x80, 0xF0, 0x80, 0x80  // F
};
}

Chip8::Chip8() {
    std::srand((unsigned)std::time(nullptr));
    for (int i = 0; i < 80; ++i) {
        memory[FONTSET_START + i] = FONTSET[i];
    }
}

bool Chip8::loadRom(const std::string& path) {
    // Day 1: read the file at `path` into memory starting at ROM_START.
    std::ifstream file(path, std::ios::binary);
    if (!file) {
        return false;
    }
    file.seekg(0, std::ios::end);
    std::streamsize size = file.tellg();
    file.seekg(0, std::ios::beg);
    // usually, to calculate if size fits into some window, we think of the window indexed in 1 such as 1,2,...,4096
    // lets imagine if ROM_START indexed in 1, it would be size > 4096 - ROM_START + 1
    // but it is indexed in 0, so its size > 4096 - (ROM_START + 1) + 1 = 4096 - ROM_START
    if (size <= 0 || size > 4096 - ROM_START) {
        return false;
    }
    for (int i = 0; i < size; ++i) {
        memory[ROM_START + i] = file.get();
    }
    return true;
}

void Chip8::cycle() {
    // Days 1-4: fetch opcode at pc, decode, execute, advance pc.
    if (waitingForKey) {
        return;
    }
    // opcode is a 16 bit value, this is by convention as its an instruction
    // since we are using a 8 bit memory, we need to combine two 8 bit values to get the 16 bit opcode
    // you do it in 2 steps, first you shift the first 8 bit value to the left by 8 bits 
    // so you have the remaining 8 bits to the right that is empty
    // then using bitwise or you then retrieve the next 8 bit value and combine
    uint16_t opcode = (memory[pc] << 8) | memory[pc + 1];
    pc += 2;
    uint8_t x = (opcode >> 8) & 0x000F; // keep 2nd hex digit
    uint8_t y = (opcode >> 4) & 0x000F; // keep 3rd hex digit
    uint8_t n = opcode & 0x000F; // keep last digit
    uint8_t nn = opcode & 0x00FF; // keep last 2 digits
    uint16_t nnn = opcode & 0x0FFF; // keep last 3 digits
    // std::printf("pc=%03X op=%04X\n", pc - 2, opcode);
    switch (opcode >> 12) {
        case 0x0: 
            if (opcode == 0x00E0) {
                for (int i = 0; i < 64 * 32; ++i) {
                    display[i] = 0;
                }
            }
            if (opcode == 0x00EE) {
                // stack pointer always points to the next empty slot
                sp--;
                pc = stack[sp];
            }
            break;  // 00E0 clear / 00EE return (Day 2-3)
        case 0x1: 
            pc = nnn;
            break;  // 1NNN jump
        case 0x2: 
            stack[sp] = pc;
            sp++;
            pc = nnn;
            break;  // 2NNN call
        case 0x3: 
            if (V[x] == nn) {
                pc += 2;
            }
            break;  // skips
        case 0x4: 
            if (V[x] != nn) {
                pc += 2;
            }
            break;
        case 0x5: 
            if (V[x] == V[y]) {
                pc += 2;
            }
            break;
        case 0x6: 
            V[x] = nn;
            break;  // 6XNN set register
        case 0x7: 
            V[x] += nn;
            break;  // 7XNN add
        case 0x8: 
            switch (n) {
                case 0x0: V[x] = V[y]; break;
                case 0x1: V[x] |= V[y]; break;
                case 0x2: V[x] &= V[y]; break;
                case 0x3: V[x] ^= V[y]; break;
                case 0x4: {
                    uint16_t sum = V[x] + V[y];
                    V[x] = sum & 0xFF;
                    V[0xF] = (sum > 0xFF) ? 1 : 0;
                    break;
                }  
                case 0x5: {
                    uint8_t diff = V[x] - V[y];
                    uint8_t noBorrow = V[x] >= V[y];
                    V[x] = diff;
                    V[0xF] = noBorrow;
                    break;
                }
                case 0x6: {
                    uint8_t lsb = V[x] & 0x1;
                    V[x] >>= 1;
                    if (lsb) {
                        V[0xF] = 1;
                    }
                    else {
                        V[0xF] = 0;
                    }                    
                    break;
                }
                case 0x7: {
                    uint8_t diff = V[y] - V[x];
                    uint8_t noBorrow = V[y] >= V[x];
                    V[x] = diff;
                    V[0xF] = noBorrow;            
                    break;
                }
                case 0xE: {
                    uint8_t msb = V[x] & 0x80;
                    V[x] <<= 1;
                    if (msb) {
                        V[0xF] = 1;
                    }
                    else {
                        V[0xF] = 0;
                    }                    
                }
                break;
            }
            break;  // 8XY? two-register ops (nested switch on n later)
        case 0x9: 
            if (V[x] != V[y]) {
                pc += 2;
            }
            break;
        case 0xA: 
            I = nnn;
            break;  // ANNN set I
        case 0xB: {
            pc = nnn + V[0];
            break;
        }
        case 0xC: {
            // rand() can be big but nn is only 8 bits, so we can use bitwise AND to limit the range of the random number to 0-255
            V[x] = std::rand() & nn;
            break;
        }  // CXNN random
        case 0xD: {
            uint8_t xPos = V[x] % 64;
            uint8_t yPos = V[y] % 32;
            V[0xF] = 0; // collision flag
            for (int row = 0; row < n; ++row) {
                uint8_t spriteByte = memory[I + row];
                for (int bit = 0; bit < 8; ++bit) {
                    if (spriteByte & (0x80 >> bit)) {
                        int px = (xPos + bit) % 64;
                        int py = (yPos + row) % 32;
                        // the display is a 1D array, so we need to convert the 2D coordinates (px, py) into a 1D index
                        // the formula for this is: index = y * width + x, where width is 64 in this case
                        int idx = py * 64 + px;
                        if (display[idx]) V[0xF] = 1;
                        display[idx] ^= 1;
                    }
                }
            }
            break;  // DXYN draw
        }
        case 0xE: {
            switch (nn) {
                case 0x9E:
                    if (keypad[V[x]]) {
                        pc += 2;
                    }
                    break;
                case 0xA1:
                    if (!keypad[V[x]]) {
                        pc += 2;
                    }
                    break;
            }
            break;  // key skips
        }
        case 0xF: {
            switch (nn) {
                case 0x0A: {
                    waitingForKey = true;
                    waitingReg = x;
                    break;
                }
                case 0x07:
                    V[x] = delayTimer;
                    break;
                case 0x15:
                    delayTimer = V[x];
                    break;
                case 0x18:
                    soundTimer = V[x];
                    break;
                case 0x1E:
                    I += V[x];
                    break;
                case 0x29:
                    I = FONTSET_START + (V[x] * 5);
                    break;
                case 0x33:
                    memory[I] = V[x] / 100;
                    memory[I + 1] = (V[x] / 10) % 10;
                    memory[I + 2] = V[x] % 10;
                    break;
                case 0x55:
                    for (int i = 0; i <= x; ++i) {
                        memory[I + i]  = V[i];
                    }
                    break;
                case 0x65:
                    for (int i = 0; i <= x; ++i) {
                        V[i]  = memory[I + i];
                    }
                    break;                
            }
            break;
        }  // FX?? timers/memory/font (nested switch on nn)
        default:
            std::printf("Unknown opcode: %04X\n", opcode);
            break;
    }
}

void Chip8::tickTimers() {
    // Day 4: decrement delayTimer and soundTimer toward zero.
    if (delayTimer > 0) delayTimer--;
    if (soundTimer > 0) soundTimer--;
}

void Chip8::keyDown(uint8_t k) {
    keypad[k] = 1;
    if (waitingForKey) {
        V[waitingReg] = k;
        waitingForKey = false;
    }
}

void Chip8::keyUp(uint8_t k) {
    keypad[k] = 0;
}
