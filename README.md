# RISC-V Processor Simulator

## Project Overview
This repository contains two cycle-accurate RISC-V processor simulators implemented in C++:
- **Single-Stage Processor**: Executes one instruction per cycle (no pipelining)
- **Five-Stage Pipelined Processor**: Classic IF/ID/EX/MEM/WB pipeline with hazard handling

Each simulator executes a program encoded in instruction and data memory files, and outputs:
- Register state per cycle (`RFOutput.txt`)
- Full microarchitectural state per cycle (`StateResult.txt`)
- Final data memory contents (`DmemResult.txt`)

---

## Build and Run Instructions

### Single-Stage Processor
```bash
g++ -std=c++17 -O2 single_stage.cpp -o rv32i_single
./rv32i_single imem.txt dmem.txt
```

### Five-Stage Pipelined Processor
```bash
g++ -std=c++17 -O2 pipeline_stage.cpp -o rv32i_pipeline
./rv32i_pipeline imem.txt dmem.txt
```

> Replace the `.cpp` file names with yours if different.


## Input Files
- `imem.txt`: Byte-wise big-endian instruction memory (4 lines = 1 instruction)
- `dmem.txt`: Byte-wise big-endian data memory


## Output Files
- `RFOutput.txt`: Register file contents per cycle
- `StateResult.txt`: Pipeline or processor state per cycle
- `DmemResult.txt`: Final data memory contents after execution


## Supported Instructions (RV32I Subset)

| Type | Mnemonics |
|------|-----------|
| R    | `ADD SUB XOR OR AND` |
| I    | `ADDI XORI ORI ANDI LW` |
| S    | `SW` |
| B    | `BEQ BNE` |
| J    | `JAL` |
| Halt | `HALT` (custom encoding to stop execution) |


## Pipeline Design (Five-Stage)

| Stage | Function |
|-------|----------|
| IF    | Instruction fetch via PC |
| ID/RF | Decode + register read |
| EX    | ALU or branch address computation |
| MEM   | Load/store from data memory |
| WB    | Write results to destination registers |

### Pipeline Features
- **NOP bit** in every stage register
- **Forwarding** (EX→ID, MEM→ID) for RAW hazard resolution
- **Stalling** when forwarding isn't sufficient
- **Static branch prediction** (not taken)
  - Misfetches flushed with a single bubble


## Simulation Notes
- Register x0 is **hardwired to 0**—no writes allowed
- Execution stops when a `HALT` instruction reaches the ID/RF stage
- All memory is byte-addressable and **big-endian**


## Author
*Kahlia Gronthos*  
**Course:** Computing Systems Architecture – NYU Tandon ECE-GY 6913  
**Semester:** Spring 2024
