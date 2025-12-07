//
// Created by ASUS Zenbook on 12/3/2025.
//
#ifndef RESERVATIONSTATION_H
#define RESERVATIONSTATION_H


class ReservationStation {
    public:
        // Constructors
        ReservationStation();


        // RS state
        bool busy;
        int op;             // operation code
        int lat;            // execution latency
        int result;
        bool resultReady;
        int A;
        int Dest;  // ROB number

        // Operand tags and values
        int Qj, Qk;         // which RS will produce the operands (0 = ready)
        int Vj, Vk;         // operand values

        void setValues(int OP, int Vj_val, int Vk_val, int Qj_tag, int Qk_tag, int address, int destROB);
        void clear();       // reset RS for next instruction

};



#endif //RESERVATIONSTATION_H
