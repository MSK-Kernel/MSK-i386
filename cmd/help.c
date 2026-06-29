// cmd/help.c - Shows all available commands

extern void print(const char* s);
extern int cmd_count;

// These are from kernel.c
typedef struct {
    char name[32];
    void (*func)(char* args);
} Command;

extern Command cmd_table[];

void help_cmd(char* args) {
    print("\nAvailable commands:");
    if (cmd_count == 0) {
        return;
    }
    
    for (int i = 0; i < cmd_count; i++) {
        print("\n");
        print(cmd_table[i].name);
    }
    
    print("\n");
}
