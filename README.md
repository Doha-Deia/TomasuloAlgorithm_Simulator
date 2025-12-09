Here is a clean **README.txt** you can copy into your GitHub repository.
If you'd like it in a different structure (Markdown, more sections, or customized to your team), just tell me!

---

# README.txt

**Project Name:** femTomas – Tomasulo Algorithm Simulator
**Course:** CSCE 3301 – Computer Architecture
**Term:** Fall 2025

---

## 🧑‍🤝‍🧑 Team Members

* **Student 1 Name – ID**
* **Student 2 Name – ID**
* **Student 3 Name – ID**

*(Replace with your actual group details.)*

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
   g++ -std=c++17 -O2 simulator.cpp -o femTomas
   ```

3. Run the simulator:

   ```
   ./femTomas
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
