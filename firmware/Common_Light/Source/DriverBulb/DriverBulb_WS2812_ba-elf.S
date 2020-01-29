// Define output data function
.global WS2812_vOutputData
.func WS2812_vOutputData

// Put function into data section so it is placed in RAM
.data
WS2812_vOutputData:
b.movi     r6,0x0
b.movi     r7,0x0
.LbitLoop:
b.andi     r8,r7,0x7
b.slli     r6,r6,0x1
b.sw       0xffffa004(r16),r5
b.bnei     r8,0x0,.LskipLoad
b.lbz      r6,0x0(r3)
b.addi     r3,r3,0x1
.LskipLoad:
b.extbs    r8,r6
b.bltsi    r8,0x0,.LoneBit
// Send a zero
b.sw       0xffffa004(r16),r0
b.nop
b.nop
b.nop
b.nop
b.nop
b.addi     r7,r7,0x1
b.sfne     r7,r4
b.bf       .LbitLoop
b.jr       r9
.LoneBit:
// Send a one
b.nop
b.nop
b.nop
b.addi     r7,r7,0x1
b.sfne     r7,r4
b.sw       0xffffa004(r16),r0
b.nop
b.bf       .LbitLoop
b.jr       r9
.endfunc
