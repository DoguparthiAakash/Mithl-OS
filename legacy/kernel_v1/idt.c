#include "idt.h"
#include "string.h"
#include "console.h"
#include "ports.h"

idt_entry_t idt_entries[256];
idt_ptr_t   idt_ptr;

extern void idt_load(uint32_t);

// Extern ISRs from assembly
extern void isr0(); extern void isr1(); extern void isr2(); extern void isr3();
extern void isr4(); extern void isr5(); extern void isr6(); extern void isr7();
extern void isr8(); extern void isr9(); extern void isr10(); extern void isr11();
extern void isr12(); extern void isr13(); extern void isr14(); extern void isr15();
extern void isr16(); extern void isr17(); extern void isr18(); extern void isr19();
extern void isr20(); extern void isr21(); extern void isr22(); extern void isr23();
extern void isr24(); extern void isr25(); extern void isr26(); extern void isr27();
extern void isr28(); extern void isr29(); extern void isr30(); extern void isr31();
extern void isr128(); // Syscall

static void idt_set_gate(uint8_t num, uint32_t base, uint16_t sel, uint8_t flags)
{
    idt_entries[num].base_lo = base & 0xFFFF;
    idt_entries[num].base_hi = (base >> 16) & 0xFFFF;
    
    idt_entries[num].sel     = sel;
    idt_entries[num].always0 = 0;
    // We must uncomment the OR below when using user mode.
    // It sets the interrupt gate's privilege level to 3.
    idt_entries[num].flags   = flags /* | 0x60 */;
}

#include "graphics.h"

// Helper to stringify hex and draw
static void panic_draw_hex(uint32_t n, int x, int y) {
    char *hex_chars = "0123456789ABCDEF";
    char buffer[11];
    buffer[0] = '0'; buffer[1] = 'x'; buffer[10] = 0;
    
    for (int i = 0; i < 8; i++) {
        buffer[9-i] = hex_chars[n & 0xF];
        n >>= 4;
    }
    draw_text(buffer, x, y, COLOR_WHITE, 16);
}

static void panic_draw_dec(uint32_t n, int x, int y) {
    if (n == 0) {
        draw_text("0", x, y, COLOR_WHITE, 16);
        return;
    }
    char buffer[32];
    int i = 0;
    while (n > 0) {
        buffer[i++] = (n % 10) + '0';
        n /= 10;
    }
    // Reverse
    for (int j = 0; j < i / 2; j++) {
        char t = buffer[j];
        buffer[j] = buffer[i - 1 - j];
        buffer[i - 1 - j] = t;
    }
    buffer[i] = 0;
    draw_text(buffer, x, y, COLOR_WHITE, 16);
}

static const char *exception_messages[] = {
    "Division By Zero",
    "Debug",
    "Non Maskable Interrupt",
    "Breakpoint",
    "Into Detected Overflow",
    "Out of Bounds",
    "Invalid Opcode",
    "No Coprocessor",
    "Double Fault",
    "Coprocessor Segment Overrun",
    "Bad TSS",
    "Segment Not Present",
    "Stack Fault",
    "General Protection Fault",
    "Page Fault",
    "Unknown Interrupt",
    "Coprocessor Fault",
    "Alignment Check",
    "Machine Check",
    "Reserved",
    "Reserved",
    "Reserved",
    "Reserved",
    "Reserved",
    "Reserved",
    "Reserved",
    "Reserved",
    "Reserved",
    "Reserved",
    "Reserved",
    "Reserved"
};

void isr_handler(registers_t *regs)
{
    // LOG TO SERIAL FIRST (Reliable)
    serial_write("\n\n=== KERNEL PANIC ===\n");
    serial_write("Exception: ");
    if (regs->int_no < 32)
        serial_write(exception_messages[regs->int_no]);
    else
        serial_write("Unknown");
    serial_write("\n");
    
    // Print details (Manual hex conversion for serial since no printf)
    // TODO: proper hex print helper for serial
    
    // Special handling for Page Fault (14)
    if (regs->int_no == 14) {
        uint32_t cr2;
        asm volatile("mov %%cr2, %0" : "=r"(cr2));
        serial_write("Page Fault at Address (CR2): ");
        
        char *hex_chars = "0123456789ABCDEF";
        char buffer[12];
        buffer[0] = '0'; buffer[1] = 'x'; buffer[10] = '\n'; buffer[11] = 0;
        for (int i = 0; i < 8; i++) {
            buffer[9-i] = hex_chars[cr2 & 0xF];
            cr2 >>= 4;
        }
        serial_write(buffer);
        
        // Also print EIP to see where we were
        serial_write("EIP: ");
        uint32_t eip = regs->eip;
        for (int i = 0; i < 8; i++) {
            buffer[9-i] = hex_chars[eip & 0xF];
            eip >>= 4;
        }
        serial_write(buffer);
    }

    // 1. Draw Red Background for Panic (Might fail if no graphics/paging)
    // draw_rect_filled((rect_t){0, 0, 1024, 768}, 0xFF880000);

    // 2. Draw Title
    // draw_text("=== KERNEL PANIC ===", 50, 50, COLOR_WHITE, 24);

    // 3. Draw Exception Name
    // draw_text("Exception:", 50, 100, COLOR_WHITE, 16);
    // if (regs->int_no < 32)
    //     draw_text(exception_messages[regs->int_no], 180, 100, COLOR_WHITE, 16);
    // else
    //     draw_text("Unknown Exception", 180, 100, COLOR_WHITE, 16);

    // 4. Draw Details
    draw_text("ISR:", 50, 130, COLOR_WHITE, 16);
    panic_draw_dec(regs->int_no, 100, 130);

    draw_text("Error Code:", 50, 160, COLOR_WHITE, 16);
    panic_draw_hex(regs->err_code, 180, 160);

    draw_text("EIP:", 50, 190, COLOR_WHITE, 16);
    panic_draw_hex(regs->eip, 180, 190);

    draw_text("CS:", 50, 220, COLOR_WHITE, 16);
    panic_draw_hex(regs->cs, 180, 220);

    draw_text("EFLAGS:", 50, 250, COLOR_WHITE, 16);
    panic_draw_hex(regs->eflags, 180, 250);

    draw_text("System Halted.", 50, 300, COLOR_WHITE, 24);

    // Force swap to ensure it is visible (if double buffered)
    graphics_swap_buffers();

    // Halt
    for (;;) { __asm__ volatile("hlt"); }
}

void idt_init(void)
{
    idt_ptr.limit = sizeof(idt_entry_t) * 256 - 1;
    idt_ptr.base  = (uint32_t)&idt_entries;

    memset(&idt_entries, 0, sizeof(idt_entry_t) * 256);

    // 0x08 is the kernel code segment
    idt_set_gate( 0, (uint32_t)isr0 , 0x08, 0x8E); // Div0
    idt_set_gate( 1, (uint32_t)isr1 , 0x08, 0x8E);
    idt_set_gate( 2, (uint32_t)isr2 , 0x08, 0x8E);
    idt_set_gate( 3, (uint32_t)isr3 , 0x08, 0x8E);
    idt_set_gate( 4, (uint32_t)isr4 , 0x08, 0x8E);
    idt_set_gate( 5, (uint32_t)isr5 , 0x08, 0x8E);
    idt_set_gate( 6, (uint32_t)isr6 , 0x08, 0x8E); // Inv Opcode
    idt_set_gate( 7, (uint32_t)isr7 , 0x08, 0x8E);
    idt_set_gate( 8, (uint32_t)isr8 , 0x08, 0x8E); // Double Fault
    idt_set_gate( 9, (uint32_t)isr9 , 0x08, 0x8E);
    idt_set_gate(10, (uint32_t)isr10, 0x08, 0x8E);
    idt_set_gate(11, (uint32_t)isr11, 0x08, 0x8E);
    idt_set_gate(12, (uint32_t)isr12, 0x08, 0x8E); // Stack
    idt_set_gate(13, (uint32_t)isr13, 0x08, 0x8E); // GPF
    idt_set_gate(14, (uint32_t)isr14, 0x08, 0x8E); // Page Fault
    idt_set_gate(15, (uint32_t)isr15, 0x08, 0x8E);
    idt_set_gate(16, (uint32_t)isr16, 0x08, 0x8E);
    idt_set_gate(17, (uint32_t)isr17, 0x08, 0x8E);
    idt_set_gate(18, (uint32_t)isr18, 0x08, 0x8E);
    idt_set_gate(19, (uint32_t)isr19, 0x08, 0x8E);
    idt_set_gate(20, (uint32_t)isr20, 0x08, 0x8E);
    idt_set_gate(21, (uint32_t)isr21, 0x08, 0x8E);
    idt_set_gate(22, (uint32_t)isr22, 0x08, 0x8E);
    idt_set_gate(23, (uint32_t)isr23, 0x08, 0x8E);
    idt_set_gate(24, (uint32_t)isr24, 0x08, 0x8E);
    idt_set_gate(25, (uint32_t)isr25, 0x08, 0x8E);
    idt_set_gate(26, (uint32_t)isr26, 0x08, 0x8E);
    idt_set_gate(27, (uint32_t)isr27, 0x08, 0x8E);
    idt_set_gate(28, (uint32_t)isr28, 0x08, 0x8E);
    idt_set_gate(29, (uint32_t)isr29, 0x08, 0x8E);
    idt_set_gate(30, (uint32_t)isr30, 0x08, 0x8E);
    idt_set_gate(31, (uint32_t)isr31, 0x08, 0x8E);
    
    extern void irq0();
    idt_set_gate(32, (uint32_t)irq0,  0x08, 0x8E);
    
    // Syscall Gate (0x80)
    // Flags: Present(0x80) | DPL3(0x60) | Interrupt Gate(0xE) = 0xEE
    idt_set_gate(128, (uint32_t)isr128, 0x08, 0xEE); 

    idt_load((uint32_t)&idt_ptr);
}
