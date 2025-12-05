#include "FunctionalUnit.h"

FunctionalUnit::FunctionalUnit(const string& type, int cycles, int numRS)
    : type(type), numCycles(cycles), numReservationStation(numRS) {
        for (int i = 0; i < numRS; i++)
            numReservationStation[i] = ReservationStation();
        
    }


string FunctionalUnit::getType() {
    return type; }

int FunctionalUnit::getNumCycles() {
    return numCycles;
}
vector<ReservationStation>& FunctionalUnit::getReservationStations() {
    return numReservationStation;
}
bool FunctionalUnit::freeRS() {
    for (auto& rs : numReservationStation) {
        if (!rs.busy) {
            return true;
        }
    }
    return false;
}
int FunctionalUnit::freeRSIndex() {
    for (size_t i = 0; i < numReservationStation.size(); i++) {
        if (!numReservationStation[i].busy) {
            return static_cast<int>(i);
        }
    }
    return -1; // No free RS
}
bool FunctionalUnit:: executeCycle(int *Memory, int &pc, bool & takenBranch, bool &target) {
    bool anyExecuting = false;
    for (auto& rs : numReservationStation) {
        if (rs.busy && !rs.resultReady) {
            rs.lat--;
            if (rs.lat <= 0) {
                if(type == "LOAD") {
                    int addr = rs.Vj + rs.A;
                    rs.result = (addr>0 && addr <1024) ? Memory[addr] : 0;
                    rs.resultReady = true;
                } else if (type == "STORE"){
                    rs.result = rs.Vj + rs.A;
                    rs.resultReady = true;
                }
                else if (type == "BEQ"){
                    if (rs.Vj == rs.Vk) {
                        takenBranch = true;
                        target = rs.A;
                        pc = rs.A; // Update PC to branch target
                    } else {
                        takenBranch = false;
                    }
                    rs.resultReady = true;
                }
                else if (type == "CALL"){
                    rs.result = pc; // Store return address
                    pc = rs.A; // Jump to function address
                    rs.resultReady = true;
                
                }
                else if (type == "ret"){
                    pc = rs.Vj; // Jump to return address
                    rs.resultReady = true;
                }
                else if(type == "NAND" || type == "ADD" || type == "SUB" || type == "MUL"){
               switch (rs.op) {
                    case 1: // ADD
                        rs.result = rs.Vj + rs.Vk;
                        break;
                    case 2: // SUB
                        rs.result = rs.Vj - rs.Vk;
                        break;
                    case 3: // MUL
                        rs.result = (rs.Vj * rs.Vk) & 0xFFFF; // lower 16-bit 
                        break;
                    case 4: // NANA
                        rs.result = ~(rs.Vj & rs.Vk) & 0xFFFF; // lower 16-bit
                        break;
                    default:
                        rs.result = 0; // Unknown operation
                }
                rs.resultReady = true;
            }
            anyExecuting = true;
        }
    }
}
    return anyExecuting;
}
bool FunctionalUnit::ReadyResult() {
    for (auto& rs : numReservationStation) {
        if (rs.busy && rs.resultReady) {
            return true;
        }
    }
    return false;
}
vector <pair<int, int>> FunctionalUnit::getReadyResults() {
    vector<pair<int, int>> results;
    for (auto& rs : numReservationStation) {
        if (rs.busy && rs.resultReady) {
            results.emplace_back(rs.Dest, rs.result);
        }
    }
    return results;
}
void FunctionalUnit::clearRS(int index) {
    for (auto& rs : numReservationStation) {
        if (&rs - &numReservationStation[0] == index) {
            rs.clear();
            break;
        }
    }
}
void FunctionalUnit::clearAll() {
    for (auto& rs : numReservationStation) {
        rs.clear();
    }
}

    
  