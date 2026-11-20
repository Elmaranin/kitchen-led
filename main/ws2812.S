/* This routine uses the Xtensa call0 ABI and runs from instruction RAM. */
    .section .iram1.text,"ax",@progbits
    .align 4
    .global ws2812_write_parallel
    .type ws2812_write_parallel,@function

/* a2 points to zero masks. a3 holds the bit count. a4 holds all output bits. */
ws2812_write_parallel:
    addi a1, a1, -16
    s32i a12, a1, 0
    movi a12, 0
    movi a5, 0x60000304
    movi a9, 64
    movi a10, 100
    movi a11, 20
.Lbit:
    l32i a8, a2, 0
    memw
    rsr a6, ccount
    s32i a4, a5, 0
.Lzero:
    rsr a7, ccount
    sub a7, a7, a6
    /* The shorter zero threshold leaves time for the branch and GPIO write. */
    bltu a7, a11, .Lzero
    memw
    s32i a8, a5, 4
    /* This sample includes the zero-bit clear instruction and any CPU delay. */
    rsr a7, ccount
    sub a7, a7, a6
    bgeu a12, a7, .Lone
    mov a12, a7
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
    mov a2, a12
    l32i a12, a1, 0
    addi a1, a1, 16
    ret
    .size ws2812_write_parallel, .-ws2812_write_parallel
