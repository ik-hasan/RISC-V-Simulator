# mini.s
# ----------------------------
# Very simple RISC-V program
# Only 3 instructions
# ----------------------------

ADDI x1, x0, 5      # x1 = 0 + 5 = 5
ADDI x2, x1, 2      # x2 = x1 + 2 = 7
ADD  x3, x1, x2     # x3 = 5 + 7 = 12
