#ifndef RESERVATIONSTATION_H
#define RESERVATIONSTATION_H

class ReservationStation
{
public:
    // Constructors
    ReservationStation();

    // RS state
    bool busy;
    int op;  // operation code
    int lat; // total latency (for info)
    int result;
    bool resultReady;
    int A;    // address / displacement / effective address
    int Dest; // ROB entry index

    // Operand tags and values
    int Qj, Qk; // producer tags (0 = value ready)
    int Vj, Vk; // operand values

    // ---- New fields for execute stage ----
    int instrIndex;    // index in program[] of this instruction
    int remainingLat;  // cycles left in current execute phase
    bool executing;    // true once execution has started
    bool addrCalcDone; // for LOAD/STORE step 1 completion
    bool done;         // execute finished, waiting for write

    bool readyThisCycle = false;

    void setValues(int OP, int Vj_val, int Vk_val,
                   int Qj_tag, int Qk_tag,
                   int address, int destROB, int instrIdx);

    void clear(); // reset RS for next instruction
};

#endif // RESERVATIONSTATION_H
