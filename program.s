# Simple demo program
# x1 = 5
ADDI x1, x0, 5
# x2 = 10
ADDI x2, x0, 10
# x3 = x1 + x2 = 15
ADD  x3, x1, x2
# mem[0] = x3 (i.e., 15)
SW   x3, 0(x0)
# load it back to x4
LW   x4, 0(x0)

# A small loop to decrement x1 until zero
loop:
ADDI x1, x1, -1
BEQ  x1, x0, end
BEQ  x0, x0, loop   # unconditional branch via BEQ x0,x0,label

end:
# done
