#include <stdint.h>
#include <stddef.h>

// ============================================================
// VGA CONSTANTS
// ============================================================

#define VGA_COLOR (0x0F << 8)
#define VGA_WIDTH 80
#define VGA_HEIGHT 25

uint16_t* VGA = (uint16_t*)0xB8000;
int cursor = 0;

// ============================================================
// GLOBAL VARIABLES
// ============================================================

int caps_lock = 0;

// Special key codes
#define KEY_UP     0x80
#define KEY_DOWN   0x81
#define KEY_LEFT   0x82
#define KEY_RIGHT  0x83
#define KEY_ENTER  0x84
#define KEY_BACKSPACE 0x85

// ============================================================
// PORT I/O
// ============================================================

void outb(uint16_t port, uint8_t val) {
    asm volatile ("outb %0, %1" : : "a"(val), "Nd"(port));
}

void outw(uint16_t port, uint16_t val) {
    asm volatile ("outw %0, %1" : : "a"(val), "Nd"(port));
}

uint8_t inb(uint16_t port) {
    uint8_t ret;
    asm volatile ("inb %1, %0" : "=a"(ret) : "Nd"(port));
    return ret;
}

// ============================================================
// CURSOR
// ============================================================

void sync_cursor() {
    int max = VGA_WIDTH * VGA_HEIGHT - 1;
    if (cursor < 0) cursor = 0;
    if (cursor > max) cursor = max;
    outb(0x3D4, 0x0F);
    outb(0x3D5, (uint8_t)(cursor & 0xFF));
    outb(0x3D4, 0x0E);
    outb(0x3D5, (uint8_t)((cursor >> 8) & 0xFF));
}

void block_cursor() {
    outb(0x3D4, 0x0A);
    outb(0x3D5, 0x00);
    outb(0x3D4, 0x0B);
    outb(0x3D5, 0x0F);
}

// ============================================================
// SCREEN FUNCTIONS
// ============================================================

void scroll() {
    for (int i = 0; i < VGA_WIDTH * (VGA_HEIGHT - 1); i++)
        VGA[i] = VGA[i + VGA_WIDTH];
    for (int i = VGA_WIDTH * (VGA_HEIGHT - 1); i < VGA_WIDTH * VGA_HEIGHT; i++)
        VGA[i] = VGA_COLOR | ' ';
    cursor = VGA_WIDTH * (VGA_HEIGHT - 1);
}

void putc(char c) {
    if (c == '\n') {
        cursor += VGA_WIDTH - (cursor % VGA_WIDTH);
    } else if (c == '\b') {
        if (cursor > 0) {
            cursor--;
            VGA[cursor] = VGA_COLOR | ' ';
        }
    } else {
        VGA[cursor] = VGA_COLOR | (uint8_t)c;
        cursor++;
    }
    if (cursor >= VGA_WIDTH * VGA_HEIGHT) scroll();
    sync_cursor();
}

void print(const char* s) {
    if (!s) return;
    while (*s) putc(*s++);
}

void clear_screen() {
    for (int i = 0; i < VGA_WIDTH * VGA_HEIGHT; i++)
        VGA[i] = VGA_COLOR | ' ';
    cursor = 0;
    sync_cursor();
}

void backspace() {
    if (cursor > 0) {
        cursor--;
        VGA[cursor] = VGA_COLOR | ' ';
    }
    sync_cursor();
}

// ============================================================
// KEYBOARD
// ============================================================

int shift = 0;

char keymap[128] = {
    0,27,'1','2','3','4','5','6','7','8','9','0','-','=',
    8,9,'q','w','e','r','t','y','u','i','o','p','[',']','\n',
    0,'a','s','d','f','g','h','j','k','l',';','\'','`',
    0,'\\','z','x','c','v','b','n','m',',','.','/',
    0,'*',0,' '
};

char keymap_shift[128] = {
    0,27,'!','@','#','$','%','^','&','*','(',')','_','+',
    8,9,'Q','W','E','R','T','Y','U','I','O','P','{','}','\n',
    0,'A','S','D','F','G','H','J','K','L',':','"','~',
    0,'|','Z','X','C','V','B','N','M','<','>','?',
    0,'*',0,' '
};

char keymap_caps[128] = {
    0,27,'1','2','3','4','5','6','7','8','9','0','-','=',
    8,9,'Q','W','E','R','T','Y','U','I','O','P','[',']','\n',
    0,'A','S','D','F','G','H','J','K','L',';','\'','`',
    0,'\\','Z','X','C','V','B','N','M',',','.','/',
    0,'*',0,' '
};

int get_key() {
    while (1) {
        if (!(inb(0x64) & 1)) continue;
        uint8_t sc = inb(0x60);
        
        // Shift keys
        if (sc == 0x2A || sc == 0x36) { shift = 1; continue; }
        if (sc == 0xAA || sc == 0xB6) { shift = 0; continue; }
        
        // Caps Lock
        if (sc == 0x3A) {
            caps_lock = !caps_lock;
            continue;
        }
        
        // Extended keys (arrow keys, etc.)
        if (sc == 0xE0) {
            while (!(inb(0x64) & 1)) continue;
            uint8_t ext_sc = inb(0x60);
            
            // Return special codes for arrow keys
            if (ext_sc == 0x48) return KEY_UP;
            if (ext_sc == 0x50) return KEY_DOWN;
            if (ext_sc == 0x4B) return KEY_LEFT;
            if (ext_sc == 0x4D) return KEY_RIGHT;
            continue;
        }
        
        // Ignore key releases
        if (sc & 0x80) continue;
        if (sc >= 128) continue;
        
        // Determine which keymap to use
        char res;
        if (caps_lock && !shift) {
            res = keymap_caps[sc];
        } else if (shift) {
            res = keymap_shift[sc];
        } else {
            res = keymap[sc];
        }
        
        if (res == 0) continue;
        return res;
    }
}

// ============================================================
// STRING HELPERS
// ============================================================

int streq(const char* a, const char* b) {
    if (!a || !b) return a == b;
    while (*a && *b) {
        if (*a != *b) return 0;
        a++; b++;
    }
    return *a == *b;
}

void clean(char* s) {
    if (!s) return;
    int i = 0;
    while (s[i]) {
        if (s[i] == '\n' || s[i] == '\r') s[i] = 0;
        i++;
    }
}

// ============================================================
// INPUT FUNCTIONS
// ============================================================

void read_line(char* buffer, int max_len) {
    char line[256];
    int pos = 0;
    int len = 0;
    int prompt_start = cursor;
    
    while (1) {
        int key = get_key();
        
        if (key == '\n' || key == KEY_ENTER) {
            putc('\n');
            break;
        }
        
        if (key == 8 || key == KEY_BACKSPACE) {
            if (pos > 0) {
                // Move cursor left
                cursor--;
                // Shift characters left
                for (int i = pos - 1; i < len - 1; i++) {
                    line[i] = line[i + 1];
                }
                len--;
                pos--;
                // Redraw from cursor position
                cursor = prompt_start + pos;
                sync_cursor();
                for (int i = pos; i < len; i++) {
                    VGA[cursor] = VGA_COLOR | (uint8_t)line[i];
                    cursor++;
                }
                VGA[cursor] = VGA_COLOR | ' ';
                cursor = prompt_start + pos;
                sync_cursor();
            }
            continue;
        }
        
        // Arrow keys
        if (key == KEY_LEFT) {
            if (pos > 0) {
                pos--;
                cursor = prompt_start + pos;
                sync_cursor();
            }
            continue;
        }
        
        if (key == KEY_RIGHT) {
            if (pos < len) {
                pos++;
                cursor = prompt_start + pos;
                sync_cursor();
            }
            continue;
        }
        
        if (key == KEY_UP || key == KEY_DOWN) {
            // History not implemented yet - just ignore
            continue;
        }
        
        // Regular character (must be printable)
        if (key >= 32 && key <= 126) {
            if (len < max_len - 1) {
                // Insert character at cursor position
                for (int i = len; i > pos; i--) {
                    line[i] = line[i - 1];
                }
                line[pos] = (char)key;
                len++;
                pos++;
                
                // Redraw from cursor position
                cursor = prompt_start + pos - 1;
                sync_cursor();
                for (int i = pos - 1; i < len; i++) {
                    VGA[cursor] = VGA_COLOR | (uint8_t)line[i];
                    cursor++;
                }
                cursor = prompt_start + pos;
                sync_cursor();
            }
        }
    }
    
    // Copy to buffer
    for (int i = 0; i < len; i++) {
        buffer[i] = line[i];
    }
    buffer[len] = 0;
}

// ============================================================
// COMMAND SYSTEM
// ============================================================

#define MAX_CMDS 64

typedef struct {
    char name[32];
    void (*func)(char* args);
} Command;

Command cmd_table[MAX_CMDS];
int cmd_count = 0;

void register_cmd(const char* name, void (*func)(char* args)) {
    if (cmd_count < MAX_CMDS) {
        int i = 0;
        while (name[i] && i < 31) {
            cmd_table[cmd_count].name[i] = name[i];
            i++;
        }
        cmd_table[cmd_count].name[i] = 0;
        cmd_table[cmd_count].func = func;
        cmd_count++;
    }
}

// ============================================================
// LIST COMMANDS
// ============================================================

void list_commands(void) {
    print("\n");
    print("Available commands:\n");
    for (int i = 0; i < cmd_count; i++) {
        print("  ");
        print(cmd_table[i].name);
        print("\n");
    }
    print("\n");
}

// ============================================================
// EXECUTE COMMAND
// ============================================================

void execute_command(const char* cmd_line) {
    while (*cmd_line == ' ') cmd_line++;
    
    if (!*cmd_line) return;
    
    char cmd_name[32];
    int i = 0;
    while (cmd_line[i] && cmd_line[i] != ' ' && cmd_line[i] != '\n' && cmd_line[i] != '\r' && i < 31) {
        cmd_name[i] = cmd_line[i];
        i++;
    }
    cmd_name[i] = 0;
    
    const char* args = cmd_line + i;
    while (*args == ' ') args++;
    
    for (int i = 0; i < cmd_count; i++) {
        const char* a = cmd_table[i].name;
        const char* b = cmd_name;
        int match = 1;
        while (*a && *b) {
            if (*a != *b) { match = 0; break; }
            a++; b++;
        }
        if (match && *a == 0) {
            cmd_table[i].func((char*)args);
            return;
        }
    }
    
    print("error: Command not found: ");
    print(cmd_name);
    print("\n");
}

// ============================================================
// SHELL
// ============================================================

void shell() {
    char buffer[256];
    while (1) {
        print("root:~# ");
        read_line(buffer, 256);
        clean(buffer);
        if (buffer[0]) {
            execute_command(buffer);
        }
    }
}

// ============================================================
// INIT
// ============================================================

extern void init_cmds(void);

// ============================================================
// KERNEL MAIN
// ============================================================

void kernel_main() {
    block_cursor();
    clear_screen();
    
    init_cmds();
    
    if (cmd_count == 0) {
        print("warning: No commands in /cmd\n");
    }
    
    shell();
}
