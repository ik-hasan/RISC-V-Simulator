# RISC-V 5-Stage Pipeline Simulator

A lightweight RISC-V instruction simulator written in C++ that models the classic 5-stage processor pipeline: **IF → ID → EX → MEM → WB**. Supports a core subset of RISC-V instructions and provides detailed cycle-by-cycle execution traces.

---

## Features

- **5-Stage Pipeline Simulation** — Fetch, Decode, Execute, Memory, Write-Back modeled explicitly per instruction
- **Supported Instructions**
  - R-type: `ADD`, `SUB`
  - I-type: `ADDI`, `LW`
  - S-type: `SW`
  - B-type: `BEQ` (with label resolution)
- **Label & Branch Support** — Two-pass parser resolves branch labels before execution
- **Cycle-accurate Logging** — Every stage prints what it does each cycle
- **Register File** — 32 general-purpose registers (`x0`–`x31`); `x0` is hardwired to 0
- **Data Memory** — 1024-word flat memory with bounds checking
- **Register Dump** — Final register state written to `reg_values.dat` for plotting with gnuplot

---

## Project Structure

```
.
├── riscv_simulator.cpp   # Main simulator source
├── simple.s              # Default test assembly file
├── mini.s                # Minimal test program
├── plot.gnu              # Gnuplot script for register visualization
└── reg_values.dat        # Output: register values after execution (auto-generated)
```

---

## Building & Running

### Compile
```bash
g++ riscv_simulator.cpp -o riscv_sim
```

### Run with a specific assembly file
```bash
./riscv_sim mini.s
```

### Run with default file (`simple.s`)
```bash
./riscv_sim
```

### Plot register values (requires gnuplot)
```bash
gnuplot -p plot.gnu
```

---

## Assembly File Format

Write plain-text `.s` files with one instruction per line. Labels must end with `:` on their own line or the line before the target instruction.

```asm
# Example: sum loop
      addi x1, x0, 5       # x1 = 5
      addi x2, x0, 0       # x2 = 0 (accumulator)
loop:
      add  x2, x2, x1      # x2 += x1
      addi x1, x1, -1      # x1--
      beq  x1, x0, done    # if x1 == 0, exit
      beq  x0, x0, loop    # unconditional loop (beq x0,x0)
done:
      sw   x2, 0(x0)       # store result to MEM[0]
```

### Memory Addressing
Memory uses **word indexing**: byte address is divided by 4 internally.

```asm
lw  x3, 8(x1)    # loads MEM[(x1 + 8) / 4]
sw  x3, 0(x0)    # stores x3 to MEM[0]
```

---

## Sample Output

```
Loaded 6 instructions.

Cycle 1: IF  - Fetched "addi x1, x0, 5" (PC=0)
Cycle 2: ID  - Decoding -> op=ADDI, rd=x1, rs1=x0, imm=5
Cycle 3: EX  - ADDI -> 0 + 5 = 5
Cycle 4: MEM - (no memory op)
Cycle 5: WB  - x1 = 5
...

=== HALT ===
Total cycles: 30, instructions executed: 6

--- Final State ---
Registers:
x1=5    x2=15
Memory[words 0..7]:
[0]=15  [1]=0  ...
```

---

## Planned Improvements

- [ ] Hazard detection unit with stall insertion
- [ ] Data forwarding (EX-EX, MEM-EX paths)
- [ ] Extend instruction set (JAL, JALR, SLT, AND, OR, XOR)
- [ ] Separate instruction and data memory
- [ ] GUI or web-based pipeline visualizer

---

## Requirements

- C++11 or later (`g++` recommended)
- gnuplot (optional, for register plots)

---
