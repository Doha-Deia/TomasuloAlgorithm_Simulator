#include "RegisterStatus.h"

// Constructor: initialize registers
RegisterStatus::RegisterStatus()
{
    for (int i = 0; i < REG_COUNT; ++i)
    {
        table[i].regNumber = i;
        table[i].robNumber = 0;
    }
}

// Set ROB number
void RegisterStatus::setROB(int regNumber, int robNumber)
{
    if (regNumber >= 0 && regNumber < REG_COUNT)
    {
        table[regNumber].robNumber = robNumber;
    }
}

// Get ROB number
int RegisterStatus::getROB(int regNumber)
{
    if (regNumber >= 0 && regNumber < REG_COUNT)
    {
        return table[regNumber].robNumber;
    }
    return -1;
}

// Check if any register is busy
bool RegisterStatus::isBusy()
{
    for (int i = 0; i < REG_COUNT; ++i)
    {
        if (table[i].robNumber != 0)
            return true;
    }
    return false;
}

// Clear a specific register
void RegisterStatus::clearRegister(int regNumber)
{
    if (regNumber >= 0 && regNumber < REG_COUNT)
    {
        table[regNumber].robNumber = 0;
    }
}

// Clear all registers
void RegisterStatus::clearAll()
{
    for (int i = 0; i < REG_COUNT; ++i)
    {
        table[i].robNumber = 0;
    }
}
