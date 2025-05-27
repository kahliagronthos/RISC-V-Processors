#!/bin/bash

# Compile test.cpp
g++ -w -o RV32_DMEM five_stage.cpp

# Check if compilation was successful
if [ $? -eq 0 ]; then
    # Run the compiled executable
    ./RV32_DMEM "sample_testcases/input/testcase1"
else
    echo "Compilation failed."
fi
