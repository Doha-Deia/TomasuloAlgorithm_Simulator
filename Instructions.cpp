#include "Instructions.h"
#include <sstream>
#include <string>
using namespace std;

Instructions:: Instructions() 
    : opcode(0), rs1(0), rs2(0), rd(0), immediate(0), address(0),
      robIndex(-1), issued(false), executing(false), written(false), committed(false),
      issueCycle(-1), execStartCycle(-1), execEndCycle(-1), writeCycle(-1), commitCycle(-1) 
{}

void Instructions:: setInstruction(int op, int s1, int s2, int d, int imm, int addr) {
    opcode = op;
    rs1 = s1;
    rs2 = s2;
    rd = d;
    immediate = imm;
    address = addr;
}
void Instructions::parse(const string& text, int op) {
    opcode = op;
    string rest = text;
    
    // Trim and remove commas
    while(rest.find(',') != string::npos) {
        rest.erase(rest.find(','), 1);
    }
    
    if(op == 1 || op == 2) { // LOAD or STORE
        parseLoadStore(rest);
    } else if(op == 4 || op == 5 || op == 6 || op == 7) { // ADD, SUB, NAND, MUL
        parseArithmetic(rest);
    } else if(op == 3) { // BEQ
        parseBranch(rest);
    } else if(op == 8 || op == 9) { // CALL, RET
        parseCallReturn(rest);
    }
}
void Instructions::parseLoadStore(const string& rest) {
    stringstream ss(rest);
    string token1, token2;
    ss >> token1 >> token2;
    
    if(opcode == 1) { // LOAD
        rd = stoi(token1.substr(1));  // take the number from the r
        size_t paren = token2.find('(');
        immediate = stoi(token2.substr(0, paren));
        rs1 = stoi(token2.substr(paren+2, token2.find(')')-paren-2));

    } else if(opcode == 2) { // STORE
        rs1 = stoi(token1.substr(1));  // value
        size_t paren = token2.find('(');
        immediate = stoi(token2.substr(0, paren));
        rs2 = stoi(token2.substr(paren+1, token2.find(')')-paren-1));
    }
}
void Instructions:: parseArithmetic(const string& rest) {
    // ADD R1, R2, R3
    // SUB R1, R2, R3
    // MUL R1, R2, R3
    // NAND R1, R2, R3
    stringstream ss(rest);
    string rdStr, rs1Str, rs2Str;
    ss >> rdStr >> rs1Str >> rs2Str;

    rd = stoi(rdStr.substr(1));   // R1 -> 1
    rs1 = stoi(rs1Str.substr(1)); // R2 -> 2
    rs2 = stoi(rs2Str.substr(1)); // R3 -> 3
}
void Instructions:: parseBranch(const string& rest) {
    // BEQ R1, R2, LABEL
    stringstream ss(rest);
    string rs1Str, rs2Str, labelStr;
    ss >> rs1Str >> rs2Str >> labelStr;

    rs1 = stoi(rs1Str.substr(1)); // R1 -> 1
    rs2 = stoi(rs2Str.substr(1)); // R2 -> 2
    immediate = stoi(labelStr);    
}
void Instructions:: parseCallReturn(const string& rest) {
    // CALL LABEL
    // RET R1
    stringstream ss(rest);
    string firstStr;
    ss >> firstStr;

    if (firstStr == "RET") {
       rs1 = 1;
    } else {
        immediate = stoi(firstStr);
    }
}
