//
// Created by ASUS Zenbook on 12/3/2025.
//

#include "ReservationStation.h"

// Default constructor
ReservationStation::ReservationStation() {
    busy = false;
    op = 0;
    lat = 0;
    result = 0;
    resultReady = false;
    Qj = 0;
    Qk = 0;
    Vj = 0;
    Vk = 0;
    A=0;
    Dest=0;
}

void ReservationStation::setValues(int OP, int Vj_val, int Vk_val, int Qj_tag, int Qk_tag, int address, int destROB) {
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
    lat = 0;
}

// Reset RS for next instruction in wrtting stage
void ReservationStation::clear() {
    busy = false;
    op = 0;
    lat = 0;
    result = 0;
    resultReady = false;
    Qj = 0;
    Qk = 0;
    Vj = 0;
    Vk = 0;
    A=0;
    Dest =0;
}
