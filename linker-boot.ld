ENTRY(__start)

MEMORY {
    boot (w!x) : ORIGIN = 0x2000000, LENGTH = 256K
    wram (w!x) : ORIGIN = 0x3000000, LENGTH = 32K
    rom (rx)   : ORIGIN = 0x8000000, LENGTH = 32M
}

SECTIONS {
    .text : {
        KEEP(target/crt0.o(.text));
        *(.text .text.*);
        . = ALIGN(4);
    } >boot = 0xff

    .rodata : {
        *(.rodata .rodata.*);
        . = ALIGN(4);
    } >boot = 0xff

    .data : {
        __data_start = ABSOLUTE(.);
        *(.data .data.*);
        . = ALIGN(4);
        __data_end = ABSOLUTE(.);
    } >wram AT>boot = 0xff

    .bss : {
        __bss_start = ABSOLUTE(.);
        *(.bss .bss.*);
        . = ALIGN(4);
        __bss_end = ABSOLUTE(.);
    } >wram

    __sidata = LOADADDR(.data);
    __wram_end = ORIGIN(wram) + LENGTH(wram) -1 ;
}
