// cmd/reboot.c - Reboot the system
// NO includes needed!

extern void outb(unsigned short port, unsigned char val);
extern void print(const char* s);

void reboot(char* args) {
    
    // Method 1: Keyboard controller reset
    outb(0x64, 0xFE);  // 0xFE = system reset
    
    // Method 2: Triple fault
    asm volatile (
        "cli\n"
        "mov $0x1234, %ax\n"
        "mov $0x4321, %bx\n"
        "int $0x18\n"
    );
    
    // If all fails, halt
    asm volatile (
        "cli\n"
        "hlt\n"
    );
}
