#include <iostream>
#include <vector>
#include "ReservationStation.h"
#include "ROB.h"
#include "Instructions.h"
#include "RegisterStatus.h"

using namespace std;

// Forward declarations of the functions
void loadProgram(vector<Instructions>& program);
int* loadMemoryData();
void issueStage(vector<Instructions>& program, int& pc, ROB& rob, 
                RegisterStatus& regStatus,
                vector<ReservationStation>& LoadRS,
                vector<ReservationStation>& StoreRS,
                vector<ReservationStation>& BEQRS,
                vector<ReservationStation>& CallRetRS,
                vector<ReservationStation>& AddSubRS,
                vector<ReservationStation>& NandRS,
                vector<ReservationStation>& MulRS,
                int cycle);
void executeStage(vector<ReservationStation>& LoadRS,
                  vector<ReservationStation>& StoreRS,
                  vector<ReservationStation>& BEQRS,
                  vector<ReservationStation>& CallRetRS,
                  vector<ReservationStation>& AddSubRS,
                  vector<ReservationStation>& NandRS,
                  vector<ReservationStation>& MulRS,
                  int cycle);
void writeStage(vector<ReservationStation>& LoadRS,
                vector<ReservationStation>& StoreRS,
                vector<ReservationStation>& BEQRS,
                vector<ReservationStation>& CallRetRS,
                vector<ReservationStation>& AddSubRS,
                vector<ReservationStation>& NandRS,
                vector<ReservationStation>& MulRS,
                ROB& rob, RegisterStatus& regStatus,
                vector<Instructions>& program, int cycle);
void commitStage(ROB& rob, RegisterStatus& regStatus,
                 vector<Instructions>& program, int& committed, int cycle);
void printResults(vector<Instructions>& program);

// Configuration constants
const int Num_Load_RS = 2;
const int Num_Store_RS = 1;
const int Num_BEQ_RS = 2;
const int Num_Call_Ret_RS = 1;
const int Num_Add_Sub_RS = 4;
const int Num_Nand_RS = 2;
const int Num_Mul_RS = 1;

const int LOAD_LATENCY = 6;
const int STORE_LATENCY = 6;
const int BEQ_LATENCY = 1;
const int CALL_RET_LATENCY = 1;
const int ADD_SUB_LATENCY = 2;
const int NAND_LATENCY = 1;
const int MUL_LATENCY = 12;

int main()
{
    cout << "================================\n";
    cout << "   TOMASULO ALGORITHM SIMULATOR\n";
    cout << "================================\n\n";
    
    // Initialize components
    vector<Instructions> program;
    ROB ROB_Table;
    RegisterStatus regStatus;
    
    // Create reservation stations
    vector<ReservationStation> LoadRS(Num_Load_RS);
    vector<ReservationStation> StoreRS(Num_Store_RS);
    vector<ReservationStation> BEQRS(Num_BEQ_RS);
    vector<ReservationStation> CallRetRS(Num_Call_Ret_RS);
    vector<ReservationStation> AddSubRS(Num_Add_Sub_RS);
    vector<ReservationStation> NandRS(Num_Nand_RS);
    vector<ReservationStation> MulRS(Num_Mul_RS);
    
    // Load program and memory
    loadProgram(program);
    int* memory = loadMemoryData();
    
    // Simulation variables
    int pc = 0;
    int cycle = 1;
    int committed = 0;
    const int MAX_CYCLES = 10;
    
    // Main simulation loop
    while (cycle <= MAX_CYCLES && committed < program.size()) {
        cout << "\n--- CYCLE " << cycle << " ---\n";
        
        commitStage(ROB_Table, regStatus, program, committed, cycle);
        writeStage(LoadRS, StoreRS, BEQRS, CallRetRS, AddSubRS, NandRS, MulRS,
                  ROB_Table, regStatus, program, cycle);
        executeStage(LoadRS, StoreRS, BEQRS, CallRetRS, AddSubRS, NandRS, MulRS, cycle);
        issueStage(program, pc, ROB_Table, regStatus,
                  LoadRS, StoreRS, BEQRS, CallRetRS, AddSubRS, NandRS, MulRS, cycle);
        
        cycle++;
    }
    
    // Print results
    printResults(program);
    
    cout << "\n================================\n";
    cout << "   SIMULATION COMPLETE\n";
    cout << "================================\n";
    
    return 0;
}