# simple.s
# ------------------------------------
# A very simple RISC-V program
# Demonstrates ADDI, ADD, SW, LW, BEQ
# ------------------------------------

# Initialize registers
ADDI x1, x0, 3      # x1 = 3
ADDI x2, x0, 7      # x2 = 7

# Add and store result
ADD  x3, x1, x2     # x3 = 3 + 7 = 10
SW   x3, 0(x0)      # memory[0] = 10

# Load back to a new register
LW   x4, 0(x0)      # x4 = memory[0] = 10

# Check if x4 == x3, then branch to label 'equal'
BEQ  x4, x3, equal

# If not equal (won’t happen here), this executes:
ADDI x5, x0, 100    # (skipped, since branch taken)

equal:
ADDI x6, x0, 999    # x6 = 999 to mark branch success

# End
