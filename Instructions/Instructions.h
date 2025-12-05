    #ifndef INSTRUCTIONS_H
    #define INSTRUCTIONS_H
    #include <string>
    using namespace std;

    class Instructions {
        public:
            Instructions();
            
            // Instruction fields
            int opcode;      // Operation code
            int rs1;        // Source register 1
            int rs2;        // Source register 2
            int rd;         // Destination register
            int immediate;  // Immediate value
            int address;    // Memory address (for load/store)

            int robIndex;       // ROB entry assigned to this instruction
            bool issued;
            bool executing;
            bool written;
            bool committed;

            int issueCycle;
            int execStartCycle;
            int execEndCycle;
            int writeCycle;
            int commitCycle;    
            void setInstruction(int op, int s1, int s2, int d, int imm, int addr);
            
            void parse(const string& text, int op) ;
            void parseLoadStore(const string& rest);
            void parseArithmetic(const string& rest);
            void parseBranch(const string& rest);
            void parseCallReturn(const string& rest);
            
    };

    #endif //INSTRUCTIONS_H