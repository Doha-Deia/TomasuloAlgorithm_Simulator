#include "ReservationStation.h"

// Default constructor
ReservationStation::ReservationStation()
{
    busy = false;
    op = 0;
    lat = 0;
    result = 0;
    resultReady = false;
    Qj = 0;
    Qk = 0;
    Vj = 0;
    Vk = 0;
    A = 0;
    Dest = 0;

    // execute-stage fields
    instrIndex = -1;
    remainingLat = 0;
    executing = false;
    addrCalcDone = false;
    done = false;
}

// Initialize RS fields when issuing an instruction
void ReservationStation::setValues(int OP, int Vj_val, int Vk_val,
                                   int Qj_tag, int Qk_tag,
                                   int address, int destROB, int instrIdx)
{
    busy = true;
    op = OP;
    Vj = Vj_val;
    Vk = Vk_val;
    Qj = Qj_tag;
    Qk = Qk_tag;
    A = address;
    Dest = destROB;
    result = 0;
    resultReady = false;
    lat = 0; // will be set by caller using getLatency(op)

    // execute-stage init
    instrIndex = instrIdx;
    remainingLat = 0;
    executing = false;
    addrCalcDone = false;
    done = false;
}

// Reset RS for next instruction in writing/commit stage
void ReservationStation::clear()
{
    busy = false;
    op = 0;
    lat = 0;
    result = 0;
    resultReady = false;
    Qj = 0;
    Qk = 0;
    Vj = 0;
    Vk = 0;
    A = 0;
    Dest = 0;

    instrIndex = -1;
    remainingLat = 0;
    executing = false;
    addrCalcDone = false;
    done = false;
}
