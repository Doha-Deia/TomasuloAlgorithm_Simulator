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

    // Helper lambda to start/continue execution for a generic RS list
    auto executeRSList = [&](vector<ReservationStation> &RS_list)
    {
        for (auto &rs : RS_list)
        {
            if (!rs.busy)
                continue;

            // Skip if already done (waiting for write)
            if (rs.done)
                continue;

            // Check operands ready
            bool operandsReady = (rs.Qj == 0) && (rs.Qk == 0);

            int op = rs.op;

            // -------- LOAD / STORE two-step execute --------
            if (op == 1 || op == 2)
            {
                cout << "  [EXEC-LOAD/STORE] Instr " << rs.instrIndex
                     << " op=" << op << " addrCalcDone=" << rs.addrCalcDone
                     << " Qj=" << rs.Qj << " Qk=" << rs.Qk << endl;

                // Step 1: compute effective address once,
                // when base operand ready and not yet done (2 cycles)
                if (!rs.addrCalcDone && rs.Qj == 0)
                {
                    // do not start execution in same cycle as broadcast
                    if (!rs.executing)
                    {
                        if (rs.readyThisCycle)
                            continue; // wait until next cycle

                        rs.executing = true;
                        rs.remainingLat = LOAD_STORE_LATENCY_1; // <<<< use phase-1 latency (2)
                        // record execStart only once (phase-1 start)
                        if (program[rs.instrIndex].execStartCycle == 0)
                            program[rs.instrIndex].execStartCycle = cycle;
                    }

                    if (rs.remainingLat > 0)
                        rs.remainingLat--;

                    if (rs.remainingLat == 0)
                    {
                        rs.A = rs.Vj + rs.A; // address computed
                        rs.addrCalcDone = true;

                        // finished phase-1: mark not executing so phase-2 can initialize next cycle
                        rs.executing = false;
                    }

                    // Store finishes execute after addr + data ready
                    if (rs.addrCalcDone && op == 2 && rs.Qk == 0)
                    {
                        rs.done = true;
                        rs.result = rs.Vk; // Store value
                        program[rs.instrIndex].execEndCycle = cycle;
                        // clear ready flag so execution won't start this same cycle
                        rs.readyThisCycle = false;
                    }

                    // don't fall through to other execution cases in the same cycle
                    // (we already handled phase-1 or store completion)
                    continue;
                }

                // Step 2: LOAD memory access (4 cycles) - only for LOAD (op==1)
                if (op == 1 && rs.addrCalcDone && !rs.done)
                {
                    // ensure we start phase-2 in a fresh executing state
                    if (!rs.executing)
                    {
                        // do not restart execStartCycle here — keep the one from phase-1
                        if (rs.readyThisCycle)
                        {
                            // if operands were broadcast this cycle, start next cycle
                            continue;
                        }
                        rs.executing = true;
                        rs.remainingLat = LOAD_STORE_LATENCY_2; // <<<< phase-2 latency (4)
                    }

                    if (rs.remainingLat > 0)
                        rs.remainingLat--;

                    if (rs.remainingLat == 0)
                    {
                        rs.result = memory[rs.A];
                        rs.done = true;
                        program[rs.instrIndex].execEndCycle = cycle;
                        // clear ready flag so we don't immediately restart in same cycle
                        rs.readyThisCycle = false;
                    }
                    // don't fall through to other ops
                    continue;
                }

                // If we reached here and op == 2 (STORE) but addrCalcDone wasn't true above,
                // just continue (no action this cycle).
                if (op == 2)
                    continue;
            }

            // -------- All other ops (1-step execute) --------
            if (!operandsReady)
                continue;

            if (!rs.executing)
            {
                rs.executing = true;
                rs.remainingLat = rs.lat;
                program[rs.instrIndex].execStartCycle = cycle;
            }

            if (rs.remainingLat > 0)
                rs.remainingLat--;

            if (rs.remainingLat == 0)
            {
                // Compute result depending on opcode
                switch (op)
                {
                case 3: // BEQ
                    rs.result = (rs.Vj == rs.Vk);
                    break;
                case 4: // ADD
                    rs.result = rs.Vj + rs.Vk;
                    break;
                case 5: // SUB
                    rs.result = rs.Vj - rs.Vk;
                    break;
                case 6: // NAND
                    rs.result = ~(rs.Vj & rs.Vk);
                    break;
                case 7: // MUL
                    rs.result = (rs.Vj * rs.Vk) & 0xFFFF;
                    break;
                case 8:                    // CALL
                    rs.result = PC + rs.A; // or PC+1+imm depending on your ISA
                    break;
                case 9:                // RET
                    rs.result = rs.Vj; // value from R1
                    break;
                }
                rs.done = true;
                program[rs.instrIndex].execEndCycle = cycle;
                rs.readyThisCycle = false;
            }
        }
    };

    ///////////////////*********not sure from executing case 8, 9 */
    // Main simulation loop
    while (!done)
    {
        cout << "\n--- Cycle " << cycle << " ---" << endl;

        vector<vector<ReservationStation> *> allRS = {
            &LoadRS, &StoreRS, &BEQRS, &CallRetRS,
            &AddSubRS, &NandRS, &MulRS};

        // ============ COMMIT STAGE ============
        // Wait until: Instruction is at the head of the ROB (entry h) and ROB[h].ready == yes
        if (!ROB_Table.isEmpty())
        {
            int h = ROB_Table.head;
            ROBEntry &entry = ROB_Table.entries[h];

            if (entry.ready && entry.busy)
            {
                int instrIdx = entry.instrIndex;
                if (instrIdx >= 0 && instrIdx < program.size())
                {
                    Instructions &inst = program[instrIdx];
                    int opcode = inst.opcode;

                    // d ← ROB[h].Dest; /* register dest, if exists */
                    int d = entry.dest;

                    // if (ROB[h].Instruction==Branch)
                    if (opcode == 3) // BEQ
                    {
                        inst.commitCycle = cycle;
                        branchCount++;

                        // if (branch was mispredicted)
                        // Always-not-taken predictor: misprediction if branch was taken
                        if (entry.value == 1) ///////////////////////////////////////not handeled correctly
                        {
                            branchMispredictions++;
                            // {clear ROB[h], RegisterStat; fetch branch dest;};
                            cout << "  [COMMIT] Instr " << instrIdx
                                 << " BEQ (MISPREDICTED - taken) - should clear pipeline" << endl;
                            // Note: Full speculation handling would clear pipeline here
                        }
                        else
                        {
                            cout << "  [COMMIT] Instr " << instrIdx
                                 << " BEQ (correctly predicted not taken)" << endl;
                        }

                        ROB_Table.remove();
                        instructionsCommitted++;
                    }
                    // else if (ROB[h].Instruction==Store)
                    else if (opcode == 2) // STORE
                    {
                        // The store uses a 4-cycle commit stage. entry.commitCountdown
                        // counts remaining commit cycles. When it reaches 1, perform the write.
                        if (entry.commitCountdown > 1)
                        {
                            // still occupying commit stage; decrement and wait
                            entry.commitCountdown--;
                            cout << "  [COMMIT] Instr " << instrIdx
                                 << " STORE commit in progress, remaining cycles = "
                                 << entry.commitCountdown << endl;
                            // Do NOT remove ROB entry yet
                        }
                        else
                        {
                            // last cycle: perform memory write and finalize commit
                            memory[entry.dest] = entry.value;
                            inst.commitCycle = cycle;

                            cout << "  [COMMIT] Instr " << instrIdx
                                 << " STORE M[" << entry.dest << "]=" << entry.value
                                 << " (4-cycle write completed)" << endl;

                            ROB_Table.remove();
                            instructionsCommitted++;
                        }
                    }
                    // else /* put the result in the register destination */
                    else
                    {
                        // {Regs[d] ← ROB[h].Value;};
                        if (d != 0) // Don't write to R0
                        {
                            RegFile[d] = entry.value;

                            cout << "  [COMMIT] Instr " << instrIdx
                                 << " wrote R" << d << "=" << entry.value << endl;
                        }
                        else
                        {
                            cout << "  [COMMIT] Instr " << instrIdx
                                 << " (writes R0, no effect)" << endl;
                        }

                        // ROB[h].Busy ← no; /* free up ROB entry */
                        ROB_Table.remove();

                        // /* free up dest register if no one else writing it */
                        // if (RegisterStat[d].Reorder==h) {RegisterStat[d].Busy ← no;};
                        if (regStatus.getROB(d) == h)
                        {
                            regStatus.setROB(d, 0);
                        }

                        inst.commitCycle = cycle;
                        instructionsCommitted++;
                    }
                }
            }
        }

        // ============ WRITE STAGE ============
        // According to the image:
        // - Write result for all but store: when execution done at r and CDB available
        // - Store: when execution done at r and RS[r].Qk == 0

        //********************************cdb availability is not handeled yet */
        for (auto RS_list : allRS)
        {
            for (auto &rs : *RS_list)
            {
                if (!rs.busy)
                    continue;

                // Check if execution is done
                if (!rs.done)
                    continue;

                int robIndex = rs.Dest;
                if (robIndex < 0 || !ROB_Table.entries[robIndex].busy)
                    continue;

                int instrIdx = ROB_Table.entries[robIndex].instrIndex;
                if (instrIdx < 0 || instrIdx >= program.size())
                    continue;

                Instructions &inst = program[instrIdx];

                // Skip if already written
                if (inst.writeCycle > 0)
                    continue;

                // STORE: Special handling - wait for Qk == 0
                if (rs.op == 2)
                {
                    if (rs.Qk != 0)
                        continue; // Value not ready yet

                    // Write store value to ROB
                    // ROB[h].Value <- RS[r].Vk
                    ROB_Table.entries[robIndex].value = rs.result;
                    ROB_Table.entries[robIndex].dest = rs.A; // Store the address
                    ROB_Table.entries[robIndex].ready = true;
                    inst.writeCycle = cycle;

                    cout << "  [WRITE] Instr " << instrIdx
                         << " (STORE) ROB[" << robIndex << "] value=" << rs.result
                         << " addr=" << rs.A << endl;

                    rs.busy = false;
                    continue;
                }

                // ALL OTHER INSTRUCTIONS (not store)
                // b <- RS[r].Dest; RS[r].Busy <- no;
                int result = rs.result;

                // Broadcast to all RS entries waiting for this result
                // ∀x(if (RS[x].Qj==b) {RS[x].Vj <- result; RS[x].Qj <- 0});
                // ∀x(if (RS[x].Qk==b) {RS[x].Vk <- result; RS[x].Qk <- 0});
                for (auto RS2_list : allRS)
                {
                    for (auto &rs2 : *RS2_list)
                    {
                        if (!rs2.busy)
                            continue;

                        if (rs2.Qj == robIndex)
                        {
                            rs2.Vj = result;
                            rs2.Qj = 0;
                            rs2.readyThisCycle = true;
                        }
                        if (rs2.Qk == robIndex)
                        {
                            rs2.Vk = result;
                            rs2.Qk = 0;
                            rs2.readyThisCycle = true;
                        }
                    }
                }

                // ROB[b].Value <- result; ROB[b].Ready <- yes;
                ROB_Table.entries[robIndex].value = result;
                ROB_Table.entries[robIndex].ready = true;
                inst.writeCycle = cycle;

                cout << "  [WRITE] Instr " << instrIdx
                     << " (ROB[" << robIndex << "]) wrote result=" << result << endl;

                // Free the reservation station
                rs.busy = false;
            }
        }

        // ============ EXECUTE STAGE ============
        executeRSList(LoadRS);
        executeRSList(StoreRS);
        executeRSList(BEQRS);
        executeRSList(CallRetRS);
        executeRSList(AddSubRS);
        executeRSList(NandRS);
        executeRSList(MulRS);

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

                if (RS_list != nullptr)
                {
                    int rsIndex = findFreeRS(*RS_list);

                    if (rsIndex != -1)
                    { // RS AVAILABLE
                        // Allocate ROB entry with instruction index
                        int robIndex = ROB_Table.addEntry(inst.rd, instrIndex);

                        // Set type for STORE
                        if (opcode == 2)
                            ROB_Table.entries[robIndex].type = "STORE";

                        inst.robIndex = robIndex;
                        inst.issued = true;
                        inst.issueCycle = cycle;

                        ReservationStation &rs = (*RS_list)[rsIndex];
                        rs.busy = true;
                        rs.op = opcode;
                        rs.Dest = robIndex;
                        rs.lat = getLatency(opcode);
                        rs.instrIndex = instrIndex; // Store instruction index in RS

                        // READ OPERANDS
                        // Initialize
                        rs.Vj = 0;
                        rs.Vk = 0;
                        rs.Qj = 0;
                        rs.Qk = 0;
                        rs.A = 0;
                        rs.executing = false;
                        rs.done = false;
                        rs.addrCalcDone = false;

                        // ---- Source 1: rs1 ----
                        if (opcode != 8) // CALL doesn't use rs1
                        {
                            int tag1 = regStatus.getROB(inst.rs1);
                            if (tag1 != 0)
                            {
                                // Operand waiting in ROB
                                rs.Qj = tag1;

                                // if ROB already has value (completed)
                                if (ROB_Table.entries[tag1].ready)
                                {
                                    rs.Vj = ROB_Table.entries[tag1].value;
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

                            ROB_Table.entries[robIndex].commitCountdown = LOAD_STORE_LATENCY_2;
                            int tag2 = regStatus.getROB(inst.rs2);
                            if (tag2 != 0)
                            {
                                rs.Qk = tag2;

                                if (ROB_Table.entries[tag2].ready)
                                {
                                    rs.Vk = ROB_Table.entries[tag2].value;
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
                                rs.Qk = tag2; // FIXED

                                if (ROB_Table.entries[tag2].ready)
                                {
                                    rs.Vk = ROB_Table.entries[tag2].value;
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
            }
            else
            {
                cout << "  [ISSUE] ROB is full" << endl;
            }
        }

        cycle++;

        // Check if done: all instructions committed
        if (instructionsCommitted >= program.size())
        {
            totalCycles = cycle - 1;
            done = true;
            cout << "\n=== All instructions committed ===" << endl;
        }

        // Safety check to prevent infinite loops
        if (cycle > 10000)
        {
            cout << "Simulation exceeded 10000 cycles. Terminating." << endl;
            totalCycles = cycle - 1;
            break;
        }
    }

    // Print final results
    printResults(program);

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

    if (totalCycles > 0)
    {
        cout << "IPC: " << fixed << setprecision(3)
             << (double)instructionsCommitted / totalCycles << endl;
    }

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