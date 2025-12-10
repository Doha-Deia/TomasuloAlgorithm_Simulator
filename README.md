**Project Name:** Tomasulo Algorithm Simulator
**Course:** CSCE 3301 – Computer Architecture
**Term:** Fall 2025

---


## 📘 Project Overview

This project implements a simulator for an out-of-order 16-bit RISC processor using **Tomasulo’s Algorithm with speculation**.
The simulator executes assembly-like instructions based on a simplified version of the **RiSC-16 ISA**.

The simulator supports:

* Load/Store instructions
* BEQ conditional branching
* CALL and RET
* Arithmetic and logic instructions (ADD, SUB, NAND, MUL)
* Out-of-order issue, execute, write, and commit
* Speculative execution with an always-not-taken branch predictor
* Reservation stations, ROB, register status table, and functional units

---

## 🧩 Features Implemented

* Full 4-stage Tomasulo pipeline simulation: **Issue → Execute → Write → Commit**
* 8-entry **Reorder Buffer (ROB)**
* Reservation station and functional unit simulation per instruction type
* Speculative branch execution using **always-not-taken prediction**
* Support for all required instructions with correct execution latencies
* Memory model with word addressing (16-bit addresses)
* Performance metrics calculation:

  * Issue/Execute/Write/Commit cycles per instruction
  * Total number of cycles
  * IPC
  * Branch misprediction rate

---
## ✅ Functionality Status
The simulator successfully implements the following behaviors correctly:

**CALL and RET instructions:** Supports function calls and returns using R1 as return address.
                              Correctly handles speculative execution and flushes younger instructions on mispredicted branches.

**Arithmetic instructions:** ADD, SUB, MUL, NAND are executed with proper latencies.
                           Out-of-order execution and result forwarding via the CDB works as expected.

**Load/Store instructions:** LOAD instructions read from memory and update registers correctly.
                           STORE instructions write to memory only upon commit, respecting in-order semantics.

**Branching (BEQ):** Branches are speculatively executed using an always-not-taken predictor.
                  Mispredictions correctly flush younger instructions and update PC.

**Reservation Stations and ROB:** Reservation stations per functional unit family are allocated and freed properly.
                                 ROB tracks instructions for correct in-order commit and register mapping.

**Pipeline stages:** Issue → Execute → Write → Commit works with proper handling of instruction dependencies.
                     Single-issue per cycle is respected, but can be modified via configuration.

**Performance metrics:** Instruction-level cycle counts (issue/execute/write/commit) are accurately tracked.
                        IPC, total cycles, branch counts, and mispredictions are calculated and displayed.

---
## ⚠️ Known Limitations
* Speculative execution relies solely on **always-not-taken prediction**. More advanced prediction is not implemented.
* Multi-issue per cycle is not fully tested.
* Handled to specific memory and immediate bit size
---

## 🧪 Test Programs

Located in the **/Test/** directory.

Provided test programs include:

1. **ArithmeticTest.asm**
   Covers: ADD, SUB, NAND, MUL
2. **LoadStoreTest.asm**
   Covers: LOAD, STORE
3. **LoopAndBranchTest.asm**
   Covers: BEQ, branching, loop behavior (required)

Each program includes sample initial memory values when needed.

---

## ▶️ How to Run the Simulator

1. Go to the **/Source Code/** directory.

2. Compile using your environment (example below may change based on your language):

   ```
   main.cpp -o mysimulator.exe
   ```

3. Run the simulator:

   ```
   mysimulator.exe
   ```

4. Provide:

   * Starting address of the program
   * Instructions to load
   * Initial memory values

5. After the simulation completes, the tool prints all required metrics.

---

## 📄 Assumptions

* Instruction fetch & decode take 0 cycles (given).
* All instructions are preloaded into the instruction queue.
* R0 is always 0 and cannot be modified.
* Memory size is 128 KB (word-addressable).
* No floating-point operations.
* No I/O instructions.
* No exceptions or interrupts.
* Each reservation station corresponds to exactly one functional unit.

---
