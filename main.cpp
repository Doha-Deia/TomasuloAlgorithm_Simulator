#include <iostream>
using namespace std;
#include <vector>
#include "ReservationStation.h"
#include "ROB.h"

// NUMBER OF RESERVATION STATIONS
const int Num_Load_RS = 2;
const int Num_Store_RS = 1;
const int Num_BEQ_RS = 2;
const int Num_Call_Ret_RS = 1;
const int Num_Add_Sub_RS = 4;
const int Num_Nand_RS = 2;
const int Num_Mul_RS = 1;

//EX Latency
const int LOAD_LATENCY = 6; //2+4
const int STORE_LATENCY = 6;  //2+4
const int BEQ_LATENCY = 1;
const int CALL_RET_LATENCY = 1;
const int ADD_SUB_LATENCY = 2;
const int NAND_LATENCY = 1;
const int MUL_LATENCY = 12;


int main()
{
    vector<ReservationStation> LoadRS(Num_Load_RS);
    vector<ReservationStation> StoreRS(Num_Store_RS);
    vector<ReservationStation> BEQRS(Num_BEQ_RS);
    vector<ReservationStation> CallRetRS(Num_Call_Ret_RS);
    vector<ReservationStation> AddSubRS(Num_Add_Sub_RS);
    vector<ReservationStation> NandRS(Num_Nand_RS);
    vector<ReservationStation> MulRS(Num_Mul_RS);

    ROB ROB_Table;

    // ----------------------------
    // 1. Load program into memory
    // 2. Load data into memory
    // 3. Cycle-by-cycle simulation:
    //      - Issue stage
    //      - Execute stage
    //      - Writeback stage
    //      - Commit stage
    //
    // 4. Print results (IPC, misprediction %, etc.)
    // ----------------------------

    return 0;
}
