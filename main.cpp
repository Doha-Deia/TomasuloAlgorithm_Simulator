// tomasulo_sim.cpp
// Tomasulo-style simulator (single-file C++17)
// Input: decoded program file and memory file (optional).
// Compile: g++ -std=c++17 tomasulo_sim.cpp -O2 -o tomasulo_sim
// Run: ./tomasulo_sim program.txt memory.txt

#include <bits/stdc++.h>
using namespace std;

// ---------------- Configuration ----------------
const int NUM_REG = 8;      // R0..R7 (R0 == 0)
const int MEM_SIZE = 64000; // word-addressable
const int ROB_SIZE = 8;     // changeable ROB size
const int ISSUE_WIDTH = 1;  // single-issue
const int DEFAULT_MAX_CYCLES = 1000000;

// Operation latencies (cycles)
struct OpInfo
{
    string name;
    int exec_latency;
    int commit_latency;
};
const unordered_map<int, OpInfo> OPCODES = {
    {1, {"LOAD", 6, 1}},
    {2, {"STORE", 1, 4}},
    {3, {"BEQ", 1, 1}},
    {4, {"ADD", 2, 1}},
    {5, {"SUB", 2, 1}},
    {6, {"NAND", 1, 1}},
    {7, {"MUL", 12, 1}},
    {8, {"CALL", 1, 1}},
    {9, {"RET", 1, 1}} };

// Reservation station counts per op family
struct RSConfig
{
    string name;
    int count;
    int latency;
};
const vector<pair<string, int>> RS_COUNTS = {
    {"LOAD", 2},
    {"STORE", 1},
    {"BR", 2},  // BEQ
    {"ADD", 4}, // ADD/SUB/NAND map here (NAND latency 1 but share or separate? we'll use separate family by op name below)
    {"MUL", 1},
    {"CALL", 1},
    {"RET", 1} };

// mapping opcode to RS family name
string opcodeRSFamily(int opcode)
{
    auto it = OPCODES.find(opcode);
    if (it == OPCODES.end())
        return "OTHER";
    string op = it->second.name;
    if (op == "LOAD")
        return "LOAD";
    if (op == "STORE")
        return "STORE";
    if (op == "BEQ")
        return "BR";
    if (op == "ADD" || op == "SUB")
        return "ADD";
    if (op == "NAND")
        return "NAND";
    if (op == "MUL")
        return "MUL";
    if (op == "CALL" || op == "RET")
        return "CALL";
    return "OTHER";
}

// ---------------- Data structures ----------------
struct Instr
{
    int id;
    int addr; // instruction address (starting address + index)
    int opcode;
    int rd;
    int rs1;
    int rs2_imm; // either rs2 or immediate depending on opcode
    // timing
    int issue = -1;
    int exec_start = -1;
    int exec_end = -1;
    int write = -1;
    int commit = -1;
    int rob_idx = -1;
    string text;
};

struct RS
{
    bool busy = false;
    int opcode = 0;
    int rob_dest = -1;
    int Vj = 0, Vk = 0;
    int Qj = -1, Qk = -1; // ROB tags that produce operands (-1 if ready)
    int A = 0;            // address/immediate
    int exec_remaining = 0;
    bool exec_started = false;
    int write_remaining = 1;
    int instr_id = -1;
    void clear() { *this = RS(); }
};

struct ROBEntry
{
    bool busy = false;
    string type;   // "REG", "STORE", "BR", "CALL", "RET"
    int dest = -1; // destination register (for REG) or memory address (for STORE)
    int value = 0;
    bool ready = false;
    int instr_id = -1;
    int pc_on_issue = -1; // instruction address when issued (useful for branch recovery)
    int br_target = -1;   // for branches: the target address
    int commit_remaining = 0;
    void clear() { *this = ROBEntry(); }
};

// ---------------- Global state ----------------
vector<Instr> program;              // list of instructions
unordered_map<int, int> addr2index; // instruction address -> program index
int startPC = 0;

vector<int> regs(NUM_REG, 0);     // rf
vector<int> reg_tag(NUM_REG, -1); // reg_stat    // map reg -> producing ROB index, -1 if none
vector<int> memory_mem(MEM_SIZE, 0);

vector<pair<string, vector<RS>>> RS_sets; // pair of family name and list of RS entries
vector<string> RS_names_order;            //**** */           // to find RS set by name

vector<ROBEntry> ROB(ROB_SIZE);
int rob_head = 0, rob_tail = 0, rob_count = 0;

int PC = 0; // current fetch address (instruction address)
int cycle_num = 0;

deque<int> fetch_queue; // program indices eligible to be issued in-order (by PC order)

int cdb_used = 0;

int total_instructions = 0;
int branch_count = 0;
int mispredictions = 0;

// ---------------- Helpers ----------------
int wrap16(int x) { return (x & 0xFFFF); } ////******* */

int allocROB()
{
    if (rob_count == ROB_SIZE)
        return -1;
    int idx = rob_tail;
    ROB[idx].busy = true;
    ROB[idx].ready = false;
    ROB[idx].value = 0;
    ROB[idx].instr_id = -1;
    ROB[idx].type = "";
    ROB[idx].dest = -1;
    ROB[idx].pc_on_issue = -1;
    ROB[idx].br_target = -1;
    rob_tail = (rob_tail + 1) % ROB_SIZE;
    ++rob_count;
    return idx;
}

void freeROB(int idx)
{
    ROB[idx].clear();
    // If freeing head/tail won't be updated here - commit handles head
}

// find RS set and an available RS for opcode
bool find_free_rs_for_opcode(int opcode, int& rs_set_idx, int& rs_idx)
{
    string fam = opcodeRSFamily(opcode);
    for (size_t i = 0; i < RS_sets.size(); ++i)
    {
        if (RS_sets[i].first == fam)
        {
            for (size_t j = 0; j < RS_sets[i].second.size(); ++j)
            {
                if (!RS_sets[i].second[j].busy)
                {
                    rs_set_idx = (int)i;
                    rs_idx = (int)j;
                    return true;
                }
            }
        }
    }
    // if not found by family, try exact op name RS set (some ops grouped)
    return false;
}

int find_rs_set_index_by_name(const string& name)
{ ///////////// *******
    for (size_t i = 0; i < RS_sets.size(); ++i)
        if (RS_sets[i].first == name)
            return (int)i;
    return -1;
}

void clear_all_rs_and_rob_younger_than_instr(int instr_pc) {
    // clear RS entries whose instruction has pc > instr_pc
    for (auto& p : RS_sets) {
        for (auto& rs : p.second) {
            if (rs.busy && rs.instr_id != -1) {
                int pid = rs.instr_id;
                if (program[pid].addr > instr_pc) rs.clear();
            }
        }
    }
    // free ROB entries whose instruction pc > instr_pc (conservative: clear all)
    for (int i = 0; i < ROB_SIZE; ++i) {
        if (ROB[i].busy && ROB[i].instr_id != -1) {
            int pid = ROB[i].instr_id;
            if (program[pid].addr > instr_pc) {
                // untag destination register if it pointed to this ROB entry
                if (ROB[i].type == "REG" && ROB[i].dest >= 0 && ROB[i].dest < NUM_REG) {
                    if (reg_tag[ROB[i].dest] == i) reg_tag[ROB[i].dest] = -1;
                }
                ROB[i].clear();
            }
        }
    }
    // rebuild fetch queue starting at the next sequential PC after instr_pc (if predicted not taken) or branch target if taken
    fetch_queue.clear();
    // We'll repopulate fetch queue from program entries with addr >= PC (PC will be set by caller)
    for (size_t i = 0; i < program.size(); ++i) {
        if (program[i].addr >= PC) fetch_queue.push_back((int)i);
    }
}
//
// void flush_after_mispredict(int branch_rob_idx)
// {
//     // 1. Clear RS entries that depend on ROB entries younger than the branch
//     for (auto &set : RS_sets)
//     {
//         for (auto &rs : set.second)
//         {
//             if (rs.busy && rs.rob_dest != -1)
//             {
//                 // if RS belongs to instruction after the branch ==> clear it
//                 int r = rs.rob_dest;
//                 if (r != branch_rob_idx &&
//                     ((branch_rob_idx < rob_tail && r > branch_rob_idx && r < rob_tail) ||
//                      (branch_rob_idx > rob_tail && (r > branch_rob_idx || r < rob_tail))))
//                 {
//                     rs.clear();
//                 }
//             }
//         }
//     }
//
//     // 2. Clear ROB entries after the branch
//     int i = (branch_rob_idx + 1) % ROB_SIZE;
//     while (i != rob_tail)
//     {
//         // untag registers
//         if (ROB[i].busy && ROB[i].type == "REG")
//         {
//             int reg = ROB[i].dest;
//             if (reg_tag[reg] == i)
//                 reg_tag[reg] = -1;
//         }
//         ROB[i].clear();
//         i = (i + 1) % ROB_SIZE;
//     }
//
//     // 3. Reset ROB tail to right after branch
//     rob_tail = (branch_rob_idx + 1) % ROB_SIZE;
//
//     // 4. Recompute ROB count
//     if (rob_tail >= rob_head)
//         rob_count = rob_tail - rob_head;
//     else
//         rob_count = rob_tail + ROB_SIZE - rob_head;
//
//     // 5. Rebuild fetch_queue starting from new PC (already set by caller)
//     fetch_queue.clear();
//     for (int p = 0; p < (int)program.size(); ++p)
//     {
//         if (program[p].addr >= PC)
//             fetch_queue.push_back(p);
//     }
// }

// ---------------- Parsing ----------------
bool load_program_file(const string& fname)
{
    ifstream f(fname);
    if (!f)
    {
        cerr << "Cannot open program file: " << fname << "\n";
        return false;
    }
    string line;
    vector<string> lines;
    while (getline(f, line))
    {
        // strip comments (# or //)
        size_t p = line.find('#');
        if (p != string::npos)
            line = line.substr(0, p);
        p = line.find("//");
        if (p != string::npos)
            line = line.substr(0, p);
        // trim
        auto trim = [](string s) -> string
            {
                size_t a = 0;
                while (a < s.size() && isspace((unsigned char)s[a]))
                    ++a;
                size_t b = s.size();
                while (b > a && isspace((unsigned char)s[b - 1]))
                    --b;
                return s.substr(a, b - a);
            };
        line = trim(line);
        if (line.empty())
            continue;
        lines.push_back(line);
    }
    if (lines.empty())
    {
        cerr << "Empty program file\n";
        return false;
    }
    // first non-empty line is starting address
    startPC = stoi(lines[0]);
    PC = startPC;
    // remaining lines are instructions
    program.clear();
    addr2index.clear();
    int idx = 0;
    for (size_t i = 1; i < lines.size(); ++i)
    {
        istringstream iss(lines[i]);
        int opcode, a, b, c;
        if (!(iss >> opcode >> a >> b >> c))
        {
            cerr << "Bad instruction format on line " << i + 1 << ": " << lines[i] << "\n";
            return false;
        }
        Instr ins;
        ins.id = idx;
        ins.addr = startPC + idx;
        ins.opcode = opcode;
        ins.rd = a;
        ins.rs1 = b;
        ins.rs2_imm = c;
        ins.text = lines[i];
        program.push_back(ins);
        addr2index[ins.addr] = idx;
        ++idx;
    }
    total_instructions = (int)program.size();
    return true;
}

bool load_memory_file(const string& fname)
{
    ifstream f(fname);
    if (!f)
    {
        // not fatal: memory stays zero
        return false;
    }
    string line;
    while (getline(f, line))
    {
        if (line.empty())
            continue;
        istringstream iss(line);
        int addr, val;
        if (!(iss >> addr >> val))
            continue;
        if (addr >= 0 && addr < MEM_SIZE)
            memory_mem[addr] = wrap16(val);
    }
    return true;
}

// ---------------- Initialization ----------------
void init_structures()
{
    // reset registers and tags
    for (int i = 0; i < NUM_REG; ++i)
        regs[i] = 0, reg_tag[i] = -1;
    // R0 is always zero -- reg_tag irrelevant

    // build RS sets: we'll create families used by opcodeRSFamily
    RS_sets.clear();
    // create entries for families with counts
    // We'll use a small mapping to set counts
    unordered_map<string, int> mapCounts;
    mapCounts["LOAD"] = 2;
    mapCounts["STORE"] = 1;
    mapCounts["BR"] = 2;
    mapCounts["ADD"] = 4; // covers ADD/SUB
    mapCounts["NAND"] = 2;
    mapCounts["MUL"] = 1;
    mapCounts["CALL"] = 1; // ret,call

    for (auto& kv : mapCounts)
    {
        vector<RS> list;
        for (int i = 0; i < kv.second; ++i)
            list.push_back(RS());
        RS_sets.push_back({ kv.first, list });
        RS_names_order.push_back(kv.first);
    }
    // clear ROB
    for (int i = 0; i < ROB_SIZE; ++i)
        ROB[i].clear();
    rob_head = rob_tail = rob_count = 0;

    // build initial fetch queue starting from PC
    fetch_queue.clear();
    for (size_t i = 0; i < program.size(); ++i)
    {
        if (program[i].addr >= PC)
            fetch_queue.push_back((int)i);
    }

    cycle_num = 0;
    cdb_used = 0;
    branch_count = 0;
    mispredictions = 0;
}

// ---------------- Pipeline stages ----------------

// Issue stage (single-issue)
void do_issue()
{
    if (fetch_queue.empty())
        return;
    int prog_idx = fetch_queue.front();
    Instr& ins = program[prog_idx];
    if (ins.addr != PC)
        return; // must issue in-order by PC
    // check ROB free slot
    int rob_idx = allocROB();
    if (rob_idx == -1)
        return; // stall due ROB full
    // find RS free
    int rs_set_idx = -1, rs_idx = -1;
    if (!find_free_rs_for_opcode(ins.opcode, rs_set_idx, rs_idx))
    {
        // no RS available -> rollback ROB alloc and stall
        rob_tail = (rob_tail - 1 + ROB_SIZE) % ROB_SIZE;
        --rob_count;
        return;
    }
    RS& rs = RS_sets[rs_set_idx].second[rs_idx];
    rs.busy = true;
    rs.opcode = ins.opcode;
    rs.instr_id = ins.id;
    rs.exec_started = false;
    rs.exec_remaining = OPCODES.at(ins.opcode).exec_latency;
    rs.Qj = rs.Qk = -1;
    rs.Vj = rs.Vk = 0;
    rs.A = 0;
    rs.rob_dest = rob_idx;

    // fill ROB entry metadata
    ROB[rob_idx].busy = true;
    ROB[rob_idx].ready = false;
    ROB[rob_idx].instr_id = ins.id;
    ROB[rob_idx].pc_on_issue = ins.addr;

    // Decode operands based on opcode (updated for your formats)
    auto getRegOrImm = [&](int token, int& val, int& tag)
        {
            tag = -1;
            if (token >= 0 && token < NUM_REG)
            {
                if (token == 0)
                {
                    val = 0;
                    tag = -1;
                }
                else if (reg_tag[token] != -1)
                {
                    tag = reg_tag[token];
                    val = 0;
                }
                else
                {
                    val = regs[token];
                    tag = -1;
                }
            }
            else
            {
                val = token;
                tag = -1;
            } // immediate or out-of-range treated immediate
        };

    string opname = OPCODES.at(ins.opcode).name;
    if (opname == "LOAD")
    {
        // 1 rd rs1 imm  => LOAD rd, imm(rs1)
        ROB[rob_idx].type = "REG";
        ROB[rob_idx].dest = ins.rd;
        int val, tag;
        getRegOrImm(ins.rs1, val, tag);  // rs1 (base)
        if (tag != -1)
            rs.Qj = tag;
        else
            rs.Vj = val;
        rs.A = ins.rs2_imm;  // imm (offset)
    }
    else if (opname == "STORE")
    {
        // 2 rs2 rs1 imm => STORE rs2, imm(rs1)
        ROB[rob_idx].type = "STORE";
        int val, tag;
        getRegOrImm(ins.rs1, val, tag);  // rs1 (base)
        if (tag != -1)
            rs.Qj = tag;
        else
            rs.Vj = val;
        rs.A = ins.rs2_imm;  // imm (offset)
        // rs2 (data to store)
        int val2, tag2;
        getRegOrImm(ins.rd, val2, tag2);  // rs2 is in ins.rd
        if (tag2 != -1)
            rs.Qk = tag2;
        else
            rs.Vk = val2;
    }

    else if (opname == "BEQ")
    {
        // 3 rs1 rs2 imm => BEQ rs1, rs2, imm
        ROB[rob_idx].type = "BR";
        ++branch_count;
        int v1, t1, v2, t2;
        getRegOrImm(ins.rd, v1, t1);   // rs1
        getRegOrImm(ins.rs1, v2, t2);  // rs2
        if (t1 != -1)
            rs.Qj = t1;
        else
            rs.Vj = v1;
        if (t2 != -1)
            rs.Qk = t2;
        else
            rs.Vk = v2;
        // correct PC_at_issue = ins.addr
        int pc_issue = ins.addr;

        // branch target = PC_at_issue + 1 + imm
        ROB[rob_idx].br_target = pc_issue + 1 + ins.rs2_imm;
    }
    else if (opname == "CALL")
    {
        // 8 0 0 imm => CALL imm
        ROB[rob_idx].type = "CALL";
        ROB[rob_idx].br_target = ins.rs2_imm;  // imm (absolute target)
    }
    else if (opname == "RET")
    {
        // 9 0 0 0 => RET
        ROB[rob_idx].type = "RET";
    }
    else
    {
        // ALU ops: ADD/SUB/NAND/MUL  (4 rd rs1 rs2)
        ROB[rob_idx].type = "REG";
        ROB[rob_idx].dest = ins.rd;
        int v1, t1, v2, t2;
        getRegOrImm(ins.rs1, v1, t1);       // rs1
        getRegOrImm(ins.rs2_imm, v2, t2);   // rs2
        if (t1 != -1)
            rs.Qj = t1;
        else
            rs.Vj = v1;
        if (t2 != -1)
            rs.Qk = t2;
        else
            rs.Vk = v2;
    }

    // if ROB writes to register, set reg_tag
    if (ROB[rob_idx].type == "REG" && ROB[rob_idx].dest >= 0 && ROB[rob_idx].dest < NUM_REG)
    {
        if (ROB[rob_idx].dest != 0)
            reg_tag[ROB[rob_idx].dest] = rob_idx;
    }

    // set instruction metadata
    ins.issue = cycle_num;
    ins.rob_idx = rob_idx;

    // advance fetch queue and PC (speculative fetch)
    fetch_queue.pop_front();
    PC = PC + 1; // sequential next instruction address by default
    // repopulate fetch_queue as needed (we already had entries for addresses >= start)
    // (fetch queue is adjusted by branch recovery)
}


// Execute stage: decrement exec_remaining for started RS entries if operands ready
// Execute stage: decrement exec_remaining for started RS entries if operands ready
void do_execute()
{
    // reset cdb flag for this cycle
    cdb_used = 0;
    // For each RS set and each RS, if operands ready and not started, start; if started decrement
    for (auto& set : RS_sets)
    {
        for (auto& rs : set.second)
        {
            if (!rs.busy)
                continue;
            // attempt to resolve operands from ROB if they are tagged
            if (rs.Qj != -1 && ROB[rs.Qj].ready)
            {
                rs.Vj = ROB[rs.Qj].value;
                rs.Qj = -1;
            }
            if (rs.Qk != -1 && ROB[rs.Qk].ready)
            {
                rs.Vk = ROB[rs.Qk].value;
                rs.Qk = -1;
            }
            Instr& ins = program[rs.instr_id];
            if (!rs.exec_started)
            {
                bool ready = (rs.Qj == -1 && rs.Qk == -1);
                if (ready)
                {
                    rs.exec_started = true;
                    if (ins.exec_start == -1)
                        ins.exec_start = cycle_num;
                    // consume one cycle immediately (this cycle)
                    rs.exec_remaining -= 1;
                    if (rs.exec_remaining == 0)
                    {
                        ins.exec_end = cycle_num;
                        // FIX: For STORE, set write_remaining to 0 to allow immediate write-back (2-cycle address execution).
                        // For other ops, keep 1-cycle delay.
                        string opname = OPCODES.at(ins.opcode).name;
                        rs.write_remaining = (opname == "STORE") ? 0 : 1;
                    }
                }
            }
            else
            {
                if (rs.exec_remaining > 0)
                {
                    rs.exec_remaining -= 1;
                    if (rs.exec_remaining == 0)
                    {
                        ins.exec_end = cycle_num;
                        // FIX: Same as above.
                        string opname = OPCODES.at(ins.opcode).name;
                        rs.write_remaining = (opname == "STORE") ? 0 : 1;
                    }
                }
            }
        }
    }

    // Special handling for STORE: only wait for base (Qj) to start execution, then respect latency
    for (auto& set : RS_sets)
    {
        for (auto& rs : set.second)
        {
            if (!rs.busy || rs.exec_started) continue;
            string opname = OPCODES.at(rs.opcode).name;
            if (opname == "STORE")
            {
                bool ready = (rs.Qj == -1);  // Only base needs to be ready to start
                if (ready)
                {
                    rs.exec_started = true;
                    Instr& ins = program[rs.instr_id];
                    if (ins.exec_start == -1)
                        ins.exec_start = cycle_num;
                    // Do NOT set exec_remaining to 0; let it decrement over the full latency (2 cycles)
                    // rs.exec_remaining is already set to 2 in do_issue()
                }
            }
        }
    }
}

// Write-back stage: pick at most one finished RS to write to ROB/CDB
// Write-back stage: write to ROB 1 cycle after execution completes
void do_write()
{
    // reset cdb flag for this cycle
    cdb_used = 0;

    // find the oldest RS ready to write
    int chosen_set = -1, chosen_idx = -1;
    int chosen_pc = INT_MAX;
    for (size_t s = 0; s < RS_sets.size(); ++s)
    {
        for (size_t i = 0; i < RS_sets[s].second.size(); ++i)
        {
            RS& rs = RS_sets[s].second[i];
            if (!rs.busy || !rs.exec_started || rs.exec_remaining > 0)
                continue;
            Instr& ins = program[rs.instr_id];
            if (ins.write != -1)
                continue; // already written
            if (ins.addr < chosen_pc)
            {
                chosen_pc = ins.addr;
                chosen_set = (int)s;
                chosen_idx = (int)i;
            }
        }
    }
    if (chosen_set == -1)
        return;
    if (cdb_used)
        return; // only one write per cycle

    RS& rs = RS_sets[chosen_set].second[chosen_idx];
    Instr& ins = program[rs.instr_id];

    // countdown write latency (1 cycle after exec)
    if (rs.write_remaining > 0)
    {
        rs.write_remaining--;
        return;
    }

    string opname = OPCODES.at(ins.opcode).name;
    ROB[rs.rob_dest].commit_remaining = OPCODES.at(ins.opcode).commit_latency;

    // write to ROB
    if (opname == "LOAD")
    {
        int addr = wrap16(rs.Vj + rs.A);
        int val = (addr >= 0 && addr < MEM_SIZE) ? memory_mem[addr] : 0;
        ROB[rs.rob_dest].value = val;
        ROB[rs.rob_dest].ready = true;

    }
    else if (opname == "STORE")
    {
        if (rs.Qk == -1)  // Wait for data (rs2) to be ready
        {
            int addr = wrap16(rs.Vj + rs.A);
            ROB[rs.rob_dest].dest = addr;
            ROB[rs.rob_dest].value = wrap16(rs.Vk);
            ROB[rs.rob_dest].ready = true;
        }
        else
        {
            return;  // Don't write yet
        }
    }
    else if (opname == "BEQ")
    {
        ROB[rs.rob_dest].value = (rs.Vj == rs.Vk) ? 1 : 0;
        ROB[rs.rob_dest].ready = true;
    }
    else if (opname == "CALL")
    {
        ROB[rs.rob_dest].ready = true;
        ROB[rs.rob_dest].br_target = ins.rs2_imm;
    }
    else if (opname == "RET")
    {
        ROB[rs.rob_dest].ready = true;
    }
    else
    {
        // ALU ops: ADD, SUB, NAND, MUL
        int result = 0;
        if (opname == "ADD")
            result = wrap16(rs.Vj + rs.Vk);
        else if (opname == "SUB")
            result = wrap16(rs.Vj - rs.Vk);
        else if (opname == "NAND")
            result = wrap16(~(rs.Vj & rs.Vk));
        else if (opname == "MUL")
            result = wrap16(rs.Vj * rs.Vk);

        ROB[rs.rob_dest].value = result;
        ROB[rs.rob_dest].dest = ins.rd;
        ROB[rs.rob_dest].ready = true;
    }

    ins.write = cycle_num;
    cdb_used = 1;

    // clear RS
    rs.clear();
}
// Commit stage: commit at most one ROB entry (in-order)
void do_commit() {
    if (rob_count == 0)
        return;

    ROBEntry& e = ROB[rob_head];

    // Can only commit a busy & ready instruction
    if (!e.busy || !e.ready)
        return;


    if (e.commit_remaining > 0)
    {
        e.commit_remaining--; // wait in ROB
        return;               // do NOT commit yet
    }
    // ----------------------------------------

    // Perform actual commit now
    int iid = e.instr_id;
    if (iid >= 0 && iid < (int)program.size())
    {
        program[iid].commit = cycle_num;
    }

    if (e.type == "REG")
    {
        // Write register result
        int rd = e.dest;
        if (rd > 0 && rd < NUM_REG)
        {
            regs[rd] = wrap16(e.value);
            if (reg_tag[rd] == rob_head)
                reg_tag[rd] = -1;
        }
    }
    else if (e.type == "STORE")
    {
        // After commit latency expires, write to memory
        int addr = e.dest;
        if (addr >= 0 && addr < MEM_SIZE)
        {
            memory_mem[addr] = wrap16(e.value);
        }
    }
    else if (e.type == "BR")
    {
        bool taken = (e.value != 0);
        int target = e.br_target;
        int pc_issue = e.pc_on_issue;

        if (taken)
        {
            mispredictions++;

            // Redirect PC to the correct target
            PC = target;

            // ===== FLUSH all SPECULATIVE younger instructions =====

            // Remove younger instruction timings
            for (auto& ins : program)
            {
                if (ins.addr > pc_issue)   // younger instructions ONLY
                {
                    ins.issue = -1;
                    ins.exec_start = -1;
                    ins.exec_end = -1;
                    ins.write = -1;
                    ins.commit = -1;
                    ins.rob_idx = -1;
                }
            }

            // Clear register tags
            for (int r = 0; r < NUM_REG; r++)
                reg_tag[r] = -1;

            // Clear RS entries
            for (auto& set : RS_sets)
                for (auto& rs : set.second)
                    rs.clear();

            // Clear ROB entirely (branch is already committing)
            for (int i = 0; i < ROB_SIZE; i++)
                ROB[i].clear();

            rob_head = rob_tail = rob_count = 0;

            // Rebuild fetch queue starting from correct target
            fetch_queue.clear();
            for (size_t i = 0; i < program.size(); i++)
                if (program[i].addr >= PC)
                    fetch_queue.push_back(i);

            // Branch is now committed, so return
            return;
        }

        // NOT taken → do nothing (PC already incremented during issue)
    }

    else if (e.type == "CALL")
    {
        // Save return address to R1 (PC+1 of CALL instruction)
        int return_addr = e.pc_on_issue + 1;
        regs[1] = wrap16(return_addr);

        // Jump to target address (absolute address from imm)
        PC = e.br_target;  // e.br_target contains the absolute target address

        // Clear timing information for all instructions that will be reissued
        for (auto& ins : program)
        {
            if (ins.addr >= PC)  // Instructions that will be reissued after CALL
            {
                ins.issue = -1;
                ins.exec_start = -1;
                ins.exec_end = -1;
                ins.write = -1;
                ins.commit = -1;
                ins.rob_idx = -1;
            }
        }

        // Clear register status tags for all registers
        for (int i = 0; i < NUM_REG; i++)
        {
            reg_tag[i] = -1;
        }

        // Clear all RS entries
        for (auto& set : RS_sets)
        {
            for (auto& rs : set.second)
            {
                rs.clear();
            }
        }

        // Clear all ROB entries
        for (int i = 0; i < ROB_SIZE; i++)
        {
            ROB[i].clear();
        }
        rob_head = 0;
        rob_tail = 0;
        rob_count = 0;

        // Rebuild fetch queue starting from target address
        fetch_queue.clear();
        for (size_t i = 0; i < program.size(); i++)
        {
            if (program[i].addr >= PC)
                fetch_queue.push_back((int)i);
        }

        // For CALL, we need to also clear the current ROB entry (the CALL itself)
        // Since we're clearing all ROB entries above, we don't need to move head
        // Just return without the normal cleanup
        return;
    }
    else if (e.type == "RET")
    {
        // Step 1: Determine target PC (value in R1)
        int target = regs[1];          // return address
        int ret_idx = e.instr_id;      // index of this RET in the program

        // Step 2: FLUSH all younger instructions (PC > RET)
        for (auto& ins : program)
        {
            if (ins.addr > program[ret_idx].addr)
            {
                ins.issue = -1;
                ins.exec_start = -1;
                ins.exec_end = -1;
                ins.write = -1;
                ins.commit = -1;
                ins.rob_idx = -1;
            }
        }

        // Step 3: Clear all register tags
        for (int r = 0; r < NUM_REG; r++)
            reg_tag[r] = -1;

        // Step 4: Clear all reservation stations
        for (auto& set : RS_sets)
            for (auto& rs : set.second)
                rs.clear();

        // Step 5: Clear all ROB entries
        for (int i = 0; i < ROB_SIZE; i++)
            ROB[i].clear();
        rob_head = rob_tail = rob_count = 0;

        // Step 6: Set PC to return address
        PC = target;

        // Step 7: Rebuild fetch queue starting from PC
        fetch_queue.clear();
        for (auto& ins : program)
            if (ins.addr >= PC)
                fetch_queue.push_back(ins.id);

        return;
    }

}
// utility: check if all instructions committed
bool all_committed()
{

    return program.back().commit != -1;

}

// single cycle step
bool step()
{
    if (program.empty())
        return false;
    if (all_committed())
        return false;

    ++cycle_num;

    // order: execute -> write -> commit -> issue (roughly)
    do_execute();
    do_write();
    do_commit();
    // ISSUE stage: single-issue
    for (int i = 0; i < ISSUE_WIDTH; ++i) ///////********** */
        do_issue();

    return true;
}

// ---------------- Reporting ----------------
void print_report()
{
    cout << "\n===== Simulation Results =====\n";
    cout << "Cycles: " << cycle_num << "\n";
    int committed = 0;
    for (auto& ins : program)
        if (ins.commit != -1)
            ++committed;
    double ipc = (cycle_num > 0) ? (double)committed / cycle_num : 0.0;
    cout << fixed << setprecision(3) << "IPC: " << ipc << "\n";
    cout << "Instructions: " << program.size() << "\n";
    cout << "Branches: " << branch_count << "  Mispredictions: " << mispredictions << "\n\n";

    cout << left << setw(5) << "ID" << setw(8) << "ADDR" << setw(8) << "OP" << setw(22) << "TEXT"
        << setw(8) << "Issue" << setw(10) << "ExecS" << setw(10) << "ExecE" << setw(8) << "Write" << setw(8) << "Commit" << "\n";
    for (auto& ins : program)
    {
        string opname = OPCODES.count(ins.opcode) ? OPCODES.at(ins.opcode).name : "UNK";
        cout << setw(5) << ins.id << setw(8) << ins.addr << setw(8) << opname << setw(22) << ins.text;
        auto f = [](int x) -> string
            { return x == -1 ? string("-") : to_string(x); };
        cout << setw(8) << f(ins.issue) << setw(10) << f(ins.exec_start) << setw(10) << f(ins.exec_end)
            << setw(8) << f(ins.write) << setw(8) << f(ins.commit) << "\n";
    }
    cout << "\nFinal registers (R0..R7):\n";
    for (int i = 0; i < NUM_REG; ++i)
        cout << "R" << i << ":" << wrap16(regs[i]) << (i == NUM_REG - 1 ? "\n" : "  ");
    cout << "\nMemory nonzero values (first 256 addresses):\n";
    int printed = 0;
    for (int i = 0; i < 256 && i < MEM_SIZE; ++i)
    {
        if (memory_mem[i] != 0)
        {
            cout << "[" << i << "]=" << memory_mem[i] << "  ";
            if (++printed % 8 == 0)
                cout << "\n";
        }
    }
    if (printed == 0)
        cout << "(none)\n";
}

int main()
{
    ios::sync_with_stdio(false);
    cin.tie(nullptr);

    // -------------------- Hardcoded file paths --------------------
    string progfile = "C:/Users/ASUS Zenbook/CLionProjects/untitled4/t.txt";
    string memfile = "C:/Users/ASUS Zenbook/CLionProjects/untitled4/m.txt";

    // -------------------- Load program --------------------
    if (!load_program_file(progfile))
    {
        cerr << "Failed to load program file: " << progfile << "\n";
        return 1;
    }

    // -------------------- Load memory (optional) --------------------
    if (!load_memory_file(memfile))
    {
        cerr << "Warning: Could not open memory file: " << memfile << "\n";
        // continue; memory stays zero
    }

    // -------------------- Initialize structures --------------------
    init_structures();

    // -------------------- Simulation loop --------------------
    const int max_cycles = DEFAULT_MAX_CYCLES;
    int steps = 0;
    while (steps < max_cycles)
    {
        if (!step())
            break;
        ++steps;
    }

    // -------------------- Print results --------------------
    print_report();

    return 0;
}