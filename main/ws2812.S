/* This routine uses the Xtensa call0 ABI and runs from instruction RAM. */
    .section .iram1.text,"ax",@progbits
    .align 4
    .global ws2812_write_parallel
    .type ws2812_write_parallel,@function

/* a2 points to zero masks. a3 holds the bit count. a4 holds all output bits. */
ws2812_write_parallel:
    movi a5, 0x60000304
    movi a9, 64
    movi a10, 100
.Lbit:
    l32i a8, a2, 0
    memw
    s32i a4, a5, 0
    rsr a6, ccount
.Lzero:
    rsr a7, ccount
    sub a7, a7, a6
    bltui a7, 32, .Lzero
    memw
    s32i a8, a5, 4
.Lone:
    rsr a7, ccount
    sub a7, a7, a6
    bltu a7, a9, .Lone
    memw
    s32i a4, a5, 4
.Lperiod:
    rsr a7, ccount
    sub a7, a7, a6
    bltu a7, a10, .Lperiod
    addi a2, a2, 4
    addi a3, a3, -1
    bnez a3, .Lbit
    memw
    ret
    .size ws2812_write_parallel, .-ws2812_write_parallel
