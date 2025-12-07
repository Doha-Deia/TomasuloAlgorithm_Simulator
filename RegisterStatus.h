#ifndef REGISTERSTATUS_H
#define REGISTERSTATUS_H

// Simple struct to represent a register entry
struct RegEntry
{
    int regNumber; // 0 to 7
    int robNumber; // 0 if not busy
};

class RegisterStatus
{
public:
    static const int REG_COUNT = 8;
    RegEntry table[REG_COUNT];

    RegisterStatus();

    // Set the ROB number for a given register number
    void setROB(int regNumber, int robNumber);

    // Get the ROB number for a given register number
    int getROB(int regNumber);

    // Check if any register is busy
    bool isBusy();

    // Clear a specific register
    void clearRegister(int regNumber);

    // Clear all registers
    void clearAll();
};

#endif // REGISTERSTATUS_H
