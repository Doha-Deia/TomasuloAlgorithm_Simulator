#include "Instructions.h"
#include <sstream>
#include <algorithm>
#include <cctype>

Instructions::Instructions()
    : opcode(0), rs1(0), rs2(0), rd(0), immediate(0),
      robIndex(-1), issued(false), executing(false),
      written(false), committed(false),
      issueCycle(-1), execStartCycle(-1),
      execEndCycle(-1), writeCycle(-1), commitCycle(-1) {}

Instructions::Instructions(int op, int r1, int r2, int d, int imm)
    : opcode(op), rs1(r1), rs2(r2), rd(d), immediate(imm),
      robIndex(-1), issued(false), executing(false),
      written(false), committed(false),
      issueCycle(-1), execStartCycle(-1),
      execEndCycle(-1), writeCycle(-1), commitCycle(-1) {}