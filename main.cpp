#include <iostream>
#include <vector>
#include <map>
#include <iomanip>
#include "ReservationStation.h"
#include "ROB.h"
#include "Instructions.h"
#include "RegisterStatus.h"
#include <fstream>

using namespace std;

// NUMBER OF RESERVATION STATIONS
const int Num_Load_RS = 2;
const int Num_Store_RS = 1;
const int Num_BEQ_RS = 2;
const int Num_Call_Ret_RS = 1;
const int Num_Add_Sub_RS = 4;
const int Num_Nand_RS = 2;
const int Num_Mul_RS = 1;

// EX Latency
const int LOAD_STORE_LATENCY_1 = 2;
const int LOAD_STORE_LATENCY_2 = 4;
const int BEQ_LATENCY = 1;
const int CALL_RET_LATENCY = 1;
const int ADD_SUB_LATENCY = 2;
const int NAND_LATENCY = 1;
const int MUL_LATENCY = 12;

// Memory (simulation only - for tracking)
map<int, int> memory;

// Registers
int registers[8] = {0}; // R0-R7, R0 always 0

// Program Counter
int PC = 0;
int startingAddress = 0;

// Statistics
int totalCycles = 0;
int instructionsCommitted = 0;
int branchCount = 0;
int branchMispredictions = 0;

// Function prototypes
void loadProgram(vector<Instructions> &program);
void loadMemoryData();
int getLatency(int opcode);

// Find free Reservation Station
int findFreeRS(vector<ReservationStation> &RS)
{
    for (int i = 0; i < RS.size(); i++)
    {
        if (!RS[i].busy)
            return i;
    }
    return -1;
}

void printResults(vector<Instructions> &program);

int main()
{
    vector<ReservationStation> LoadRS(Num_Load_RS);
    vector<ReservationStation> StoreRS(Num_Store_RS);
    vector<ReservationStation> BEQRS(Num_BEQ_RS);
    vector<ReservationStation> CallRetRS(Num_Call_Ret_RS);
    vector<ReservationStation> AddSubRS(Num_Add_Sub_RS);
    vector<ReservationStation> NandRS(Num_Nand_RS);
    vector<ReservationStation> MulRS(Num_Mul_RS);

    int RegFile[8] = {0, 1, 2, 3, 4, 5, 6, 7}; // R0 must be 0

    ROB ROB_Table;
    RegisterStatus regStatus;
    vector<Instructions> program;

    cout << "=== Tomasulo Algorithm Simulator ===" << endl
         << endl;

    // Load program and memory data
    loadProgram(program);
    loadMemoryData();

    int instrIndex = 0; // Tracks which instruction to issue next (index in program vector)
    int cycle = 1;
    bool done = false;

    // Main simulation loop
    while (!done)
    {
        cout << "\n--- Cycle " << cycle << " ---" << endl;

        // ============ ISSUE STAGE ============
        if (instrIndex < program.size())
        {
            Instructions &inst = program[instrIndex];
            int opcode = inst.opcode;

            // 1. Check ROB availability
            if (!ROB_Table.isFull())
            {
                // Pick correct RS list depending on opcode
                vector<ReservationStation> *RS_list = nullptr;

                if (opcode == 1)
                    RS_list = &LoadRS;
                else if (opcode == 2)
                    RS_list = &StoreRS;
                else if (opcode == 3)
                    RS_list = &BEQRS;
                else if (opcode == 4 || opcode == 5)
                    RS_list = &AddSubRS;
                else if (opcode == 6)
                    RS_list = &NandRS;
                else if (opcode == 7)
                    RS_list = &MulRS;
                else if (opcode == 8 || opcode == 9)
                    RS_list = &CallRetRS;

                int rsIndex = findFreeRS(*RS_list);

                if (rsIndex != -1)
                { // RS AVAILABLE
                    // Allocate ROB entry
                    string type = (opcode == 2) ? "STORE" : "REG";
                    ROB_Table.addEntry(inst.rd);
                    int robIndex = (ROB_Table.tail - 1 + 8) % 8;

                    inst.robIndex = robIndex;
                    inst.issued = true;
                    inst.issueCycle = cycle;

                    ReservationStation &rs = (*RS_list)[rsIndex];
                    rs.busy = true;
                    rs.op = opcode;
                    rs.Dest = robIndex;
                    rs.lat = getLatency(opcode);

                    // READ OPERANDS
                    // Initialize
                    rs.Vj = 0;
                    rs.Vk = 0;
                    rs.Qj = 0;
                    rs.Qk = 0;

                    // ---- Source 1: rs1 ----
                    if (opcode != 8) // CALL doesn't use rs1
                    {
                        int tag1 = regStatus.getROB(inst.rs1);
                        if (tag1 != 0)
                        {
                            // Operand waiting in ROB
                            rs.Qj = tag1;

                            // if ROB already has value (completed)
                            const ROBEntry *entries = ROB_Table.getEntries();
                            if (entries[tag1].ready)
                            {
                                rs.Vj = entries[tag1].value;
                                rs.Qj = 0;
                            }
                        }
                        else
                        {
                            // Operand ready in register file
                            rs.Vj = RegFile[inst.rs1];
                            rs.Qj = 0;
                        }
                    }

                    // ---- Source 2: rs2 ---- (for R-type, BEQ, STORE)
                    if (opcode == 2) // STORE needs rs2 (value to store)
                    {
                        int tag2 = regStatus.getROB(inst.rs2);
                        if (tag2 != 0)
                        {
                            rs.Qk = tag2;

                            const ROBEntry *entries = ROB_Table.getEntries();
                            if (entries[tag2].ready)
                            {
                                rs.Vk = entries[tag2].value;
                                rs.Qk = 0;
                            }
                        }
                        else
                        {
                            rs.Vk = RegFile[inst.rs2];
                            rs.Qk = 0;
                        }
                    }
                    else if (opcode == 3 || (opcode >= 4 && opcode <= 7)) // BEQ, arithmetic
                    {
                        int tag2 = regStatus.getROB(inst.rs2);
                        if (tag2 != 0)
                        {
                            rs.Qj = tag2;

                            const ROBEntry *entries = ROB_Table.getEntries();
                            if (entries[tag2].ready)
                            {
                                rs.Vk = entries[tag2].value;
                                rs.Qk = 0;
                            }
                        }
                        else
                        {
                            rs.Vk = RegFile[inst.rs2];
                            rs.Qk = 0;
                        }
                    }

                    // Special cases (LOAD/STORE immediate addr)
                    if (opcode == 1 || opcode == 2)
                    {
                        rs.A = inst.immediate;
                    }

                    // Set register status for destinations (register renaming)
                    if (opcode != 2 && opcode != 3 && opcode != 9) // Not STORE, BEQ, RET
                    {
                        regStatus.setROB(inst.rd, robIndex);
                    }

                    cout << "  [ISSUE] Instr " << instrIndex
                         << " (addr=" << inst.address << ", op=" << opcode
                         << ") -> RS[" << rsIndex << "], ROB[" << robIndex << "]" << endl;

                    instrIndex++; // Move to next instruction
                }
                else
                {
                    cout << "  [ISSUE] No free RS for opcode " << opcode << endl;
                }
            }
            else
            {
                cout << "  [ISSUE] ROB is full" << endl;
            }
        }

        // EXECUTE STAGE

        // WRITE STAGE

        // COMMIT STAGE

        totalCycles = cycle - 1;

        // Print results
        printResults(program);

        cycle++;

        // Check if done: all instructions committed
        if (instructionsCommitted >= program.size())
        {
            done = true;
        }

        // Safety check to prevent infinite loops
        if (cycle > 10000)
        {
            cout << "Simulation exceeded 10000 cycles. Terminating." << endl;
            break;
        }
    }

    return 0;
}

void printResults(vector<Instructions> &program)
{
    cout << "\n\n=== SIMULATION RESULTS ===" << endl;
    cout << "\nInstruction Timing Table:" << endl;
    cout << setw(5) << "Instr" << setw(8) << "Issue" << setw(10) << "ExecStart"
         << setw(10) << "ExecEnd" << setw(8) << "Write" << setw(8) << "Commit" << endl;
    cout << string(57, '-') << endl;

    for (size_t i = 0; i < program.size(); i++)
    {
        cout << setw(5) << i
             << setw(8) << program[i].issueCycle
             << setw(10) << program[i].execStartCycle
             << setw(10) << program[i].execEndCycle
             << setw(8) << program[i].writeCycle
             << setw(8) << program[i].commitCycle
             << endl;
    }

    cout << "\n=== Performance Metrics ===" << endl;
    cout << "Total Execution Time: " << totalCycles << " cycles" << endl;
    cout << "Instructions Committed: " << instructionsCommitted << endl;
    cout << "IPC: " << fixed << setprecision(3)
         << (double)instructionsCommitted / totalCycles << endl;

    if (branchCount > 0)
    {
        cout << "Branch Misprediction Rate: " << fixed << setprecision(2)
             << (100.0 * branchMispredictions / branchCount) << "%" << endl;
    }
}

void loadProgram(vector<Instructions> &program)
{
    cout << "Enter program filename: ";
    string filename;
    cin >> filename;

    ifstream infile(filename);
    if (!infile.is_open())
    {
        cout << "Error opening file!" << endl;
        exit(1);
    }

    program.clear();

    // Read starting address
    infile >> startingAddress;
    PC = startingAddress;

    cout << "Starting address: " << startingAddress << endl;

    int op, a, b, c;
    int currentAddr = startingAddress;

    while (infile >> op >> a >> b >> c)
    {
        Instructions instr;

        if (op == 1)
        {                                         // LOAD rd, offset(rs1)
            instr = Instructions(op, b, 0, a, c); // rs1=b, rd=a, immediate=c
        }
        else if (op == 2)
        {                                         // STORE rs2, offset(rs1)
            instr = Instructions(op, b, a, 0, c); // rs1=b, rs2=a, immediate=c
        }
        else if (op == 3)
        {                                         // BEQ rs1, rs2, offset
            instr = Instructions(op, a, b, 0, c); // rs1=a, rs2=b, immediate=c
        }
        else if (op == 8)
        {                                         // CALL label
            instr = Instructions(op, 0, 0, 1, a); // rd=1 (R1 stores return), immediate=a
        }
        else if (op == 9)
        {                                         // RET
            instr = Instructions(op, 1, 0, 0, 0); // rs1=1 (return address in R1)
        }
        else
        {                                         // ADD, SUB, NAND, MUL (rd, rs1, rs2)
            instr = Instructions(op, b, c, a, 0); // rs1=b, rs2=c, rd=a
        }

        instr.address = currentAddr;
        currentAddr++;

        program.push_back(instr);
    }

    infile.close();
    cout << "Loaded " << program.size() << " instructions." << endl;
}

void loadMemoryData()
{
    cout << "Enter memory filename: ";
    string fname;
    cin >> fname;

    ifstream memfile(fname);
    if (!memfile.is_open())
    {
        cout << "Error loading memory file!" << endl;
        exit(1);
    }

    memory.clear();
    int address, value;

    while (memfile >> address >> value)
    {
        memory[address] = value;
    }

    memfile.close();
    cout << "Memory data loaded successfully." << endl;
}

int getLatency(int opcode)
{
    int LOAD_LATENCY = LOAD_STORE_LATENCY_1 + LOAD_STORE_LATENCY_2;
    int STORE_LATENCY = LOAD_LATENCY;
    switch (opcode)
    {
    case 1:
        return LOAD_LATENCY;
    case 2:
        return STORE_LATENCY;
    case 3:
        return BEQ_LATENCY;
    case 4:
    case 5:
        return ADD_SUB_LATENCY;
    case 6:
        return NAND_LATENCY;
    case 7:
        return MUL_LATENCY;
    case 8:
    case 9:
        return CALL_RET_LATENCY;
    default:
        return 1;
    }
}