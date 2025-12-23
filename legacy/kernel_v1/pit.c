#include "pit.h"
#include "ports.h"
#include "process.h"
#include "console.h"

#include "console.h"
#include "sched_polaris.h"

// PIT Ports
#define PIT_CMD_PORT 0x43
#define PIT_CH0_PORT 0x40

// PIC Ports
#define PIC1_CMD 0x20
#define PIC1_DATA 0x21
#define PIC2_CMD 0xA0
#define PIC2_DATA 0xA1

#define ICW1_INIT 0x10
#define ICW1_ICW4 0x01
#define ICW4_8086 0x01

static void io_wait(void) {
    outb(0x80, 0);
}

void pic_remap(void) {
    uint8_t a1, a2;

    // Save masks
    a1 = inb(PIC1_DATA);
    a2 = inb(PIC2_DATA);

    // Start Init
    outb(PIC1_CMD, ICW1_INIT | ICW1_ICW4);
    io_wait();
    outb(PIC2_CMD, ICW1_INIT | ICW1_ICW4);
    io_wait();

    // Offsets (32 = 0x20, 40 = 0x28)
    outb(PIC1_DATA, 0x20); // Master -> 32
    io_wait();
    outb(PIC2_DATA, 0x28); // Slave  -> 40
    io_wait();

    // Wiring
    outb(PIC1_DATA, 4);
    io_wait();
    outb(PIC2_DATA, 2);
    io_wait();

    // Mode
    outb(PIC1_DATA, ICW4_8086);
    io_wait();
    outb(PIC2_DATA, ICW4_8086);
    io_wait();

    // Restore masks (or set new ones)
    // For now, we only want Timer (IRQ0). Mask everything else for safety if polling others.
    // Keyboard (IRQ1) is polled, but if we enable interrupts, polling might get interrupted?
    // Actually, keyboard status port reading doesn't require IRQ.
    // But if we mask IRQ1, the controller buffers it?
    // Let's Unmask Timer(0) and maybe Keyboard(1) eventually.
    // For now, just Timer. 0xFE = 11111110 (Bit 0 clear)
    outb(PIC1_DATA, 0xFE); 
    outb(PIC2_DATA, 0xFF);
}

void pit_init(uint32_t frequency) {
    pic_remap();
    
    // Set frequency
    uint32_t divisor = 1193180 / frequency;
    
    outb(PIT_CMD_PORT, 0x36); // Mode 3 (Square Wave)
    outb(PIT_CH0_PORT, divisor & 0xFF);
    outb(PIT_CH0_PORT, (divisor >> 8) & 0xFF);
    
    console_write("[INFO] PIT Initialized.\n");
}

void timer_handler(void) {
    // Acknowledge PIC (Master)
    outb(PIC1_CMD, 0x20);
    
    // Switch Task
    // Switch Task
    process_schedule();
    // sched_schedule_polaris(); // Integration: Uncomment this and comment process_schedule to switch.
}
