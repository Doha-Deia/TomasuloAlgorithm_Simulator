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
const int LOAD_LATENCY = 6;  // 2+4
const int STORE_LATENCY = 6; // 2+4
const int BEQ_LATENCY = 1;
const int CALL_RET_LATENCY = 1;
const int ADD_SUB_LATENCY = 2;
const int NAND_LATENCY = 1;
const int MUL_LATENCY = 12;

// Memory and Registers
map<int, int> memory;   // Address -> Value
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
vector<ReservationStation> *getRS(int opcode, vector<ReservationStation> &LoadRS,
                                  vector<ReservationStation> &StoreRS,
                                  vector<ReservationStation> &BEQRS,
                                  vector<ReservationStation> &CallRetRS,
                                  vector<ReservationStation> &AddSubRS,
                                  vector<ReservationStation> &NandRS,
                                  vector<ReservationStation> &MulRS);
void issueStage(vector<Instructions> &program, int &instrIndex, ROB &rob,
                RegisterStatus &regStatus, vector<ReservationStation> &LoadRS,
                vector<ReservationStation> &StoreRS, vector<ReservationStation> &BEQRS,
                vector<ReservationStation> &CallRetRS, vector<ReservationStation> &AddSubRS,
                vector<ReservationStation> &NandRS, vector<ReservationStation> &MulRS,
                int cycle);
void executeStage(vector<ReservationStation> &LoadRS, vector<ReservationStation> &StoreRS,
                  vector<ReservationStation> &BEQRS, vector<ReservationStation> &CallRetRS,
                  vector<ReservationStation> &AddSubRS, vector<ReservationStation> &NandRS,
                  vector<ReservationStation> &MulRS, int cycle);
void writeStage(vector<ReservationStation> &LoadRS, vector<ReservationStation> &StoreRS,
                vector<ReservationStation> &BEQRS, vector<ReservationStation> &CallRetRS,
                vector<ReservationStation> &AddSubRS, vector<ReservationStation> &NandRS,
                vector<ReservationStation> &MulRS, ROB &rob, RegisterStatus &regStatus,
                vector<Instructions> &program, int cycle);
void commitStage(ROB &rob, RegisterStatus &regStatus, vector<Instructions> &program,
                 int &PC, int cycle);
void printResults(vector<Instructions> &program);
int executeInstruction(ReservationStation &rs);

int main()
{
    vector<ReservationStation> LoadRS(Num_Load_RS);
    vector<ReservationStation> StoreRS(Num_Store_RS);
    vector<ReservationStation> BEQRS(Num_BEQ_RS);
    vector<ReservationStation> CallRetRS(Num_Call_Ret_RS);
    vector<ReservationStation> AddSubRS(Num_Add_Sub_RS);
    vector<ReservationStation> NandRS(Num_Nand_RS);
    vector<ReservationStation> MulRS(Num_Mul_RS);

    ROB rob;
    RegisterStatus regStatus;
    vector<Instructions> program;

    cout << "=== Tomasulo Algorithm Simulator ===" << endl
         << endl;

    // Load program and data
    loadProgram(program);
    loadMemoryData();

    int instrIndex = 0; // Next instruction to issue
    int cycle = 1;
    bool done = false;

    // Main simulation loop
    while (!done)
    {
        cout << "\n--- Cycle " << cycle << " ---" << endl;

        // 4 stages (in reverse order to avoid conflicts)
        commitStage(rob, regStatus, program, PC, cycle);
        writeStage(LoadRS, StoreRS, BEQRS, CallRetRS, AddSubRS, NandRS, MulRS,
                   rob, regStatus, program, cycle);
        executeStage(LoadRS, StoreRS, BEQRS, CallRetRS, AddSubRS, NandRS, MulRS, cycle);
        issueStage(program, instrIndex, rob, regStatus, LoadRS, StoreRS, BEQRS,
                   CallRetRS, AddSubRS, NandRS, MulRS, cycle);

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
 
    totalCycles = cycle - 1;

    // Print results
    printResults(program);

    return 0;
}

// void loadProgram(vector<Instructions> &program)
// {
//     cout << "Enter starting address for program: ";
//     cin >> startingAddress;
//     PC = startingAddress;

//     int numInstr;
//     cout << "Enter number of instructions: ";
//     cin >> numInstr;
//     cin.ignore();

//     cout << "\nEnter instructions (format: OPCODE operands):" << endl;
//     cout << "Opcodes: 1=LOAD, 2=STORE, 3=BEQ, 4=ADD, 5=SUB, 6=NAND, 7=MUL, 8=CALL, 9=RET" << endl;

//     for (int i = 0; i < numInstr; i++)
//     {
//         int op;
//         string line;
//         cout << "Instruction " << i << " opcode: ";
//         cin >> op;
//         cin.ignore();
//         cout << "Instruction " << i << " operands: ";
//         getline(cin, line);

//         Instructions instr;
//         instr.parse(line, op);
//         instr.address = startingAddress + i;
//         program.push_back(instr);
//     }
// }

// void loadMemoryData()
// {
//     int numData;
//     cout << "\nEnter number of memory data items: ";
//     cin >> numData;

//     for (int i = 0; i < numData; i++)
//     {
//         int addr, value;
//         cout << "Data " << i << " address: ";
//         cin >> addr;
//         cout << "Data " << i << " value: ";
//         cin >> value;
//         memory[addr] = value;
//     }
// }

void loadProgram(vector<Instructions> &program)
{
    cout << "Enter program filename: ";
    string filename;
    cin >> filename;

    ifstream infile(filename);
    if (!infile.is_open()) {
        cout << "Error opening file!\n";
        exit(1);
    }

    program.clear();
    infile >> startingAddress;
    PC = startingAddress;

    int op, a, b, c;

    while (infile >> op >> a >> b >> c) {
        Instructions instr;
        instr.opcode = op;

        if (op == 1) {               // LOAD rd, offset(rs1)
            instr.rd = a;
            instr.rs1 = b;
            instr.immediate = c;
        }
        else if (op == 2) {          // STORE rs2, offset(rs1)
            instr.rs2 = a;
            instr.rs1 = b;
            instr.immediate = c;
        }
        else if (op == 3) {          // BEQ rs1, rs2, offset
            instr.rs1 = a;
            instr.rs2 = b;
            instr.immediate = c;
        }
        else {                       // ADD, SUB, NAND, MUL
            instr.rd = a;
            instr.rs1 = b;
            instr.rs2 = c;
        }

        instr.address = startingAddress + program.size();
        program.push_back(instr);
    }

    infile.close();
}


void loadMemoryData()
{
    cout << "Enter memory filename: ";
    string fname;
    cin >> fname;

    ifstream memfile(fname);
    if (!memfile.is_open()) {
        cout << "Error loading memory file!\n";
        exit(1);
    }

    memory.clear();
    int address, value;

    while (memfile >> address >> value) {
        memory[address] = value;
    }

    memfile.close();
}



int getLatency(int opcode)
{
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

vector<ReservationStation> *getRS(int opcode, vector<ReservationStation> &LoadRS,
                                  vector<ReservationStation> &StoreRS,
                                  vector<ReservationStation> &BEQRS,
                                  vector<ReservationStation> &CallRetRS,
                                  vector<ReservationStation> &AddSubRS,
                                  vector<ReservationStation> &NandRS,
                                  vector<ReservationStation> &MulRS)
{
    switch (opcode)
    {
    case 1:
        return &LoadRS;
    case 2:
        return &StoreRS;
    case 3:
        return &BEQRS;
    case 4:
    case 5:
        return &AddSubRS;
    case 6:
        return &NandRS;
    case 7:
        return &MulRS;
    case 8:
    case 9:
        return &CallRetRS;
    default:
        return nullptr;
    }
}

void issueStage(vector<Instructions> &program, int &instrIndex, ROB &rob,
                RegisterStatus &regStatus, vector<ReservationStation> &LoadRS,
                vector<ReservationStation> &StoreRS, vector<ReservationStation> &BEQRS,
                vector<ReservationStation> &CallRetRS, vector<ReservationStation> &AddSubRS,
                vector<ReservationStation> &NandRS, vector<ReservationStation> &MulRS,
                int cycle)
{
    if (instrIndex >= program.size())
        return;
    if (rob.isFull())
        return;

    Instructions &instr = program[instrIndex];
    vector<ReservationStation> *rs = getRS(instr.opcode, LoadRS, StoreRS, BEQRS,
                                           CallRetRS, AddSubRS, NandRS, MulRS);

    if (!rs)
        return;

    // Find free RS
    int freeRS = -1;
    for (size_t i = 0; i < rs->size(); i++)
    {
        if (!(*rs)[i].busy)
        {
            freeRS = i;
            break;
        }
    }

    if (freeRS == -1)
        return; // No free RS

    // Allocate ROB entry
    string type = (instr.opcode == 2) ? "STORE" : "REG";
    rob.addEntry(type, instr.rd);
    int robIndex = (rob.tail - 1 + 8) % 8;
    instr.robIndex = robIndex;

    // Get operand values or tags
    int Vj = 0, Vk = 0, Qj = 0, Qk = 0;

    if (instr.opcode != 8 && instr.opcode != 9)
    { // Not CALL/RET
        // Source 1
        int tag1 = regStatus.getTag(instr.rs1);
        if (tag1 == 0)
        {
            Vj = registers[instr.rs1];
        }
        else
        {
            Qj = tag1;
        }

        // Source 2 (if applicable)
        if (instr.opcode == 4 || instr.opcode == 5 || instr.opcode == 6 || instr.opcode == 7)
        {
            int tag2 = regStatus.getTag(instr.rs2);
            if (tag2 == 0)
            {
                Vk = registers[instr.rs2];
            }
            else
            {
                Qk = tag2;
            }
        }
    }

    // Set RS values
    int addr = (instr.opcode == 1 || instr.opcode == 2) ? instr.immediate : 0;
    (*rs)[freeRS].setValues(instr.opcode, Vj, Vk, Qj, Qk, addr, robIndex);
    (*rs)[freeRS].lat = getLatency(instr.opcode);

    // Update register status
    if (instr.opcode != 2 && instr.opcode != 3 && instr.opcode != 9)
    { // Not STORE, BEQ, RET
        regStatus.setTag(instr.rd, robIndex);
    }

    instr.issued = true;
    instr.issueCycle = cycle;
    cout << "Issued instruction " << instrIndex << " to RS[" << freeRS << "]" << endl;
    instrIndex++;
}

void executeStage(vector<ReservationStation> &LoadRS, vector<ReservationStation> &StoreRS,
                  vector<ReservationStation> &BEQRS, vector<ReservationStation> &CallRetRS,
                  vector<ReservationStation> &AddSubRS, vector<ReservationStation> &NandRS,
                  vector<ReservationStation> &MulRS, int cycle)
{
    vector<ReservationStation> *allRS[] = {&LoadRS, &StoreRS, &BEQRS, &CallRetRS,
                                           &AddSubRS, &NandRS, &MulRS};

    for (auto rsSet : allRS)
    {
        for (auto &rs : *rsSet)
        {
            if (rs.busy && !rs.resultReady && rs.Qj == 0 && rs.Qk == 0)
            {
                if (rs.lat > 1)
                {
                    rs.lat--;
                }
                else if (rs.lat == 1)
                {
                    rs.result = executeInstruction(rs);
                    rs.resultReady = true;
                    rs.lat = 0;
                    cout << "Completed execution on RS" << endl;
                }
            }
        }
    }
}

int executeInstruction(ReservationStation &rs)
{
    switch (rs.op)
    {
    case 1: // LOAD
        return memory[rs.Vj + rs.A];
    case 2: // STORE (just compute address)
        return rs.Vj + rs.A;
    case 3: // BEQ
        return (rs.Vj == rs.Vk) ? 1 : 0;
    case 4: // ADD
        return rs.Vj + rs.Vk;
    case 5: // SUB
        return rs.Vj - rs.Vk;
    case 6: // NAND
        return ~(rs.Vj & rs.Vk);
    case 7: // MUL
        return (rs.Vj * rs.Vk) & 0xFFFF;
    case 8: // CALL
        return PC + 1;
    case 9: // RET
        return rs.Vj;
    default:
        return 0;
    }
}

void writeStage(vector<ReservationStation> &LoadRS, vector<ReservationStation> &StoreRS,
                vector<ReservationStation> &BEQRS, vector<ReservationStation> &CallRetRS,
                vector<ReservationStation> &AddSubRS, vector<ReservationStation> &NandRS,
                vector<ReservationStation> &MulRS, ROB &rob, RegisterStatus &regStatus,
                vector<Instructions> &program, int cycle)
{
    vector<ReservationStation> *allRS[] = {&LoadRS, &StoreRS, &BEQRS, &CallRetRS,
                                           &AddSubRS, &NandRS, &MulRS};

    for (auto rsSet : allRS)
    {
        for (auto &rs : *rsSet)
        {
            if (rs.busy && rs.resultReady)
            {
                // Write to ROB
                rob.markReady(rs.Dest, rs.result);

                // Broadcast to other RS (resolve dependencies)
                for (auto rsSet2 : allRS)
                {
                    for (auto &rs2 : *rsSet2)
                    {
                        if (rs2.busy)
                        {
                            if (rs2.Qj == rs.Dest)
                            {
                                rs2.Vj = rs.result;
                                rs2.Qj = 0;
                            }
                            if (rs2.Qk == rs.Dest)
                            {
                                rs2.Vk = rs.result;
                                rs2.Qk = 0;
                            }
                        }
                    }
                }

                // Update instruction
                for (auto &instr : program)
                {
                    if (instr.robIndex == rs.Dest && !instr.written)
                    {
                        instr.written = true;
                        instr.writeCycle = cycle;
                        break;
                    }
                }

                cout << "Written result to ROB[" << rs.Dest << "]" << endl;
                rs.clear();
            }
        }
    }
}

void commitStage(ROB &rob, RegisterStatus &regStatus, vector<Instructions> &program,
                 int &PC, int cycle)
{
    ROBEntry *entry = rob.peek();
    if (!entry || !entry->ready)
        return;

    // Find corresponding instruction
    for (auto &instr : program)
    {
        if (instr.robIndex == rob.head && !instr.committed)
        {
            if (entry->type == "STORE")
            {
                memory[entry->value] = entry->dest; // Store to memory
            }
            else if (entry->dest > 0)
            {
                registers[entry->dest] = entry->value;
                regStatus.clearTag(entry->dest);
            }

            instr.committed = true;
            instr.commitCycle = cycle;
            instructionsCommitted++;

            if (instr.opcode == 3)
            { // BEQ
                branchCount++;
                if (entry->value == 1)
                {                           // Branch taken
                    branchMispredictions++; // Always-not-taken predictor
                }
            }

            cout << "Committed instruction " << instructionsCommitted - 1 << endl;
            rob.remove();
            break;
        }
    }
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
             << setw(8) << program[i].commitCycle << endl;
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