// VGA text mode base address
char *videomem = (char *)0xB8000;
int cursor_pos = 0;

// Port I/O functions (defined in port_io.asm)
extern unsigned char inb(unsigned short port);
extern void outb(unsigned short port, unsigned char value);

int shift_pressed = 0;
char command_buffer[80];
int command_pos = 0;
unsigned char boot_hour = 0, boot_minute = 0, boot_second = 0;
// ============================================================================
// UTILITY FUNCTIONS
// ============================================================================

void strcat_simple(char *dest, const char *src) {
    while (*dest) dest++;
    while (*src) *dest++ = *src++;
    *dest = '\0';
}

int strcmp(const char *str1, const char *str2) {
    while (*str1 && *str1 == *str2) { str1++; str2++; }
    return *str1 - *str2;
}

int starts_with(const char *str, const char *prefix) {
    while (*prefix) if (*str++ != *prefix++) return 0;
    return 1;
}

int atoi(const char *str) {
    int num = 0, sign = 1, i = 0;
    if (str[0] == '-') { sign = -1; i = 1; }
    while (str[i] >= '0' && str[i] <= '9') num = num * 10 + (str[i++] - '0');
    return num * sign;
}

void itoa(int num, char *str) {
    int i = 0, is_negative = (num < 0);
    if (is_negative) num = -num;
    if (!num) str[i++] = '0';
    else while (num > 0) { str[i++] = (num % 10) + '0'; num /= 10; }
    if (is_negative) str[i++] = '-';
    str[i] = '\0';
    for (int j = 0; j < i / 2; j++) {
        char temp = str[j];
        str[j] = str[i - 1 - j];
        str[i - 1 - j] = temp;
    }
}

void uint_to_hex(unsigned int num, char *str) {
    const char hex[] = "0123456789ABCDEF";
    str[0] = '0'; str[1] = 'x';
    for (int i = 7; i >= 0; i--) str[2 + (7 - i)] = hex[(num >> (i * 4)) & 0xF];
    str[10] = '\0';
}

// ============================================================================
// VGA FUNCTIONS
// ============================================================================

void print(const char *str) {
    int i = 0;
    while (str[i]) {
        if (str[i] == '\n') {
            cursor_pos = ((cursor_pos / 160) + 1) * 160;
        } else {
            videomem[cursor_pos] = str[i];
            videomem[cursor_pos + 1] = 0x02;
            cursor_pos += 2;
        }
        i++;
        if (cursor_pos >= 4000) {
            for (int j = 0; j < 3840; j++) videomem[j] = videomem[j + 160];
            for (int j = 3840; j < 4000; j += 2) { videomem[j] = ' '; videomem[j + 1] = 0x02; }
            cursor_pos = 3840;
        }
    }
}

void clear_screen() {
    for (int i = 0; i < 4000; i += 2) {
        videomem[i] = ' ';
        videomem[i + 1] = 0x02;
    }
    cursor_pos = 0;
}

void backspace() {
    if (cursor_pos > 0) {
        cursor_pos -= 2;
        videomem[cursor_pos] = ' ';
        videomem[cursor_pos + 1] = 0x02;
    }
}

// ============================================================================
// KEYBOARD FUNCTIONS
// ============================================================================

unsigned char read_key() {
    return (inb(0x64) & 0x01) ? inb(0x60) : 0;
}

char scancode_to_ascii(unsigned char sc) {
    // Handle shift press/release
    if (sc == 0x2A || sc == 0x36) { shift_pressed = 1; return 0; }
    if (sc == 0xAA || sc == 0xB6) { shift_pressed = 0; return 0; }
    if (sc == 0x0E) return '\b';
    if (sc & 0x80) return 0;
    
    // Compact scancode maps
    static const char normal[] = {
        0,0,'1','2','3','4','5','6','7','8','9','0','-','=',0,0,
        'q','w','e','r','t','y','u','i','o','p','[',']','\n',0,
        'a','s','d','f','g','h','j','k','l',';','\'','`',0,'\\',
        'z','x','c','v','b','n','m',',','.','/',0,0,0,' ',0
    };
    static const char shifted[] = {
        0,0,'!','@','#','$','%','^','&','*','(',')','_','+',0,0,
        'Q','W','E','R','T','Y','U','I','O','P','{','}','\n',0,
        'A','S','D','F','G','H','J','K','L',':','"','~',0,'|',
        'Z','X','C','V','B','N','M','<','>','?',0,0,0,' ',0
    };
    
    return (sc < sizeof(normal)) ? (shift_pressed ? shifted[sc] : normal[sc]) : 0;
}

// ============================================================================
// CPUID FUNCTIONS
// ============================================================================

int cpuid_supported() {
    unsigned int before, after;
    asm volatile(
        "pushfl\n\t"
        "popl %0\n\t"
        "movl %0, %1\n\t"
        "xorl $0x200000, %0\n\t"
        "pushl %0\n\t"
        "popfl\n\t"
        "pushfl\n\t"
        "popl %0\n\t"
        : "=r"(after), "=r"(before)
    );
    return ((before ^ after) & 0x200000) != 0;
}

void cpuid(unsigned int leaf, unsigned int *eax, unsigned int *ebx, unsigned int *ecx, unsigned int *edx) {
    asm volatile("cpuid" : "=a"(*eax), "=b"(*ebx), "=c"(*ecx), "=d"(*edx) : "a"(leaf), "c"(0));
}

void get_cpu_vendor(char *vendor) {
    unsigned int eax, ebx, ecx, edx;
    cpuid(0, &eax, &ebx, &ecx, &edx);
    for (int i = 0; i < 4; i++) {
        vendor[i] = (ebx >> (i * 8)) & 0xFF;
        vendor[i + 4] = (edx >> (i * 8)) & 0xFF;
        vendor[i + 8] = (ecx >> (i * 8)) & 0xFF;
    }
    vendor[12] = '\0';
}

void get_cpu_brand(char *brand) {
    unsigned int eax, ebx, ecx, edx;
    cpuid(0x80000000, &eax, &ebx, &ecx, &edx);
    if (eax < 0x80000004) { brand[0] = '\0'; return; }
    
    for (int i = 0; i < 3; i++) {
        cpuid(0x80000002 + i, &eax, &ebx, &ecx, &edx);
        for (int j = 0; j < 4; j++) {
            brand[i * 16 + j] = (eax >> (j * 8)) & 0xFF;
            brand[i * 16 + j + 4] = (ebx >> (j * 8)) & 0xFF;
            brand[i * 16 + j + 8] = (ecx >> (j * 8)) & 0xFF;
            brand[i * 16 + j + 12] = (edx >> (j * 8)) & 0xFF;
        }
    }
    brand[48] = '\0';
}

void get_cpu_features(unsigned int *ecx, unsigned int *edx) {
    unsigned int eax, ebx;
    cpuid(1, &eax, &ebx, ecx, edx);
}

// ============================================================================
// MEMORY DETECTION
// ============================================================================

unsigned int probe_memory() {
    unsigned int *test_addr, mb_count;
    unsigned int test_pattern = 0xAA55AA55;
    
    for (mb_count = 1; mb_count < 256; mb_count++) {
        test_addr = (unsigned int *)(0x100000 + (mb_count * 0x100000));
        unsigned int original = *test_addr;
        *test_addr = test_pattern;
        for (int i = 0; i < 100; i++) asm volatile("nop");
        if (*test_addr != test_pattern) { *test_addr = original; break; }
        *test_addr = original;
    }
    return mb_count;
}

// ============================================================================
// VGA HARDWARE DETECTION
// ============================================================================

unsigned char read_vga_register(unsigned short index_port, unsigned char index) {
    outb(index_port, index);
    return inb(index_port + 1);
}

void get_vga_info(unsigned char *mode, unsigned char *width, unsigned char *height) {
    unsigned char h_total = read_vga_register(0x3D4, 0x01);
    unsigned char v_total = read_vga_register(0x3D4, 0x12);
    unsigned char misc = inb(0x3CC);
    *mode = (misc & 0x01) ? 1 : 0;
    *width = h_total + 1;
    *height = v_total;
}

// ============================================================================
// KEYBOARD CONTROLLER STATUS
// ============================================================================

unsigned char get_keyboard_status() {
    return inb(0x64);
}

void decode_keyboard_status(unsigned char status, char *buffer) {
    buffer[0] = '\0';
    if (status & 0x01) strcat_simple(buffer, "OBF ");
    if (status & 0x02) strcat_simple(buffer, "IBF ");
    if (status & 0x04) strcat_simple(buffer, "SYS ");
    if (status & 0x08) strcat_simple(buffer, "CMD ");
    if (status & 0x20) strcat_simple(buffer, "AUXB ");
    if (status & 0x40) strcat_simple(buffer, "TIMEOUT ");
    if (status & 0x80) strcat_simple(buffer, "PERR ");
}

// ============================================================================
// CMOS/RTC TIME READING
// ============================================================================

unsigned char read_cmos(unsigned char reg) {
    outb(0x70, reg);
    return inb(0x71);
}

unsigned char bcd_to_bin(unsigned char bcd) {
    return ((bcd >> 4) * 10) + (bcd & 0x0F);
}

void get_rtc_time(unsigned char *hour, unsigned char *minute, unsigned char *second) {
    while (read_cmos(0x0A) & 0x80);
    *second = read_cmos(0x00);
    *minute = read_cmos(0x02);
    *hour = read_cmos(0x04);
    unsigned char status_b = read_cmos(0x0B);
    if (!(status_b & 0x04)) {
        *second = bcd_to_bin(*second);
        *minute = bcd_to_bin(*minute);
        *hour = bcd_to_bin(*hour);
    }
}

// ============================================================================
// PCI DEVICE ENUMERATION
// ============================================================================

unsigned int pci_config_read(unsigned char bus, unsigned char device, unsigned char func, unsigned char offset) {
    unsigned int address = (((unsigned int)bus) << 16) | (((unsigned int)device) << 11) | 
                          (((unsigned int)func) << 8) | (offset & 0xFC) | 0x80000000;
    outb(0xCF8, address & 0xFF);
    outb(0xCF9, (address >> 8) & 0xFF);
    outb(0xCFA, (address >> 16) & 0xFF);
    outb(0xCFB, (address >> 24) & 0xFF);
    return inb(0xCFC) | ((unsigned int)inb(0xCFD) << 8) | 
           ((unsigned int)inb(0xCFE) << 16) | ((unsigned int)inb(0xCFF) << 24);
}

int pci_device_exists(unsigned char bus, unsigned char device, unsigned char func) {
    unsigned int vendor_device = pci_config_read(bus, device, func, 0);
    return (vendor_device != 0xFFFFFFFF && vendor_device != 0);
}

// ============================================================================
// COMMAND IMPLEMENTATIONS
// ============================================================================

void cmd_cpuinfo() {
    if (!cpuid_supported()) { print("\nCPUID not supported!"); return; }
    
    print("\n=== CPU INFORMATION ===");
    char vendor[13], brand[49], hex_str[11];
    get_cpu_vendor(vendor);
    print("\nVendor: "); print(vendor);
    
    get_cpu_brand(brand);
    if (brand[0]) { print("\nBrand: "); print(brand); }
    
    unsigned int ecx, edx;
    get_cpu_features(&ecx, &edx);
    print("\nFeatures (EDX): "); uint_to_hex(edx, hex_str); print(hex_str);
    print("\nFeatures (ECX): "); uint_to_hex(ecx, hex_str); print(hex_str);
    
    print("\n\nSupported: ");
    if (edx & (1 << 0)) print("FPU ");
    if (edx & (1 << 4)) print("TSC ");
    if (edx & (1 << 5)) print("MSR ");
    if (edx & (1 << 23)) print("MMX ");
    if (edx & (1 << 25)) print("SSE ");
    if (edx & (1 << 26)) print("SSE2 ");
    if (ecx & (1 << 0)) print("SSE3 ");
}

void cmd_meminfo() {
    print("\n=== MEMORY INFORMATION ===");
    unsigned int total_mb = probe_memory();
    char mem_str[20];
    print("\nTotal RAM detected: "); itoa(total_mb, mem_str); print(mem_str); print(" MB");
    print("\nLower Memory: 640 KB (conventional)");
    print("\nVideo Memory: 0xA0000-0xBFFFF (VGA)");
    print("\nExtended Memory: "); itoa(total_mb - 1, mem_str); print(mem_str); print(" MB");
}

void cmd_memstat() {
    print("\n=== MEMORY STATISTICS ===");
    unsigned int total_mb = probe_memory();
    char stat_str[20];
    print("\nTotal: "); itoa(total_mb * 1024, stat_str); print(stat_str); print(" KB");
    print("\nKernel: ~1 MB");
    print("\nAvailable: ~"); itoa(total_mb * 1024 - 1024, stat_str); print(stat_str); print(" KB");
}

void cmd_kbdstat() {
    print("\n=== KEYBOARD STATUS ===");
    unsigned char status = get_keyboard_status();
    char hex_str[11], status_flags[100];
    print("\nStatus Register: "); uint_to_hex(status, hex_str); print(hex_str);
    decode_keyboard_status(status, status_flags);
    print("\nFlags: "); print(status_flags);
    print("\n\nBit Details:");
    print("\n Bit 0 (OBF): "); print((status & 0x01) ? "Output buffer full" : "Empty");
    print("\n Bit 1 (IBF): "); print((status & 0x02) ? "Input buffer full" : "Empty");
    print("\n Bit 2 (SYS): "); print((status & 0x04) ? "System flag set" : "Clear");
}

void cmd_vgainfo() {
    print("\n=== VGA INFORMATION ===");
    unsigned char mode, width, height;
    char reg_str[20];
    get_vga_info(&mode, &width, &height);
    print("\nMode: "); print(mode ? "Color" : "Monochrome");
    print("\nText Mode: 80x25");
    print("\nVideo Memory: 0xB8000");
    print("\n\nCRTC Registers:");
    print("\n Horizontal Total: "); itoa(read_vga_register(0x3D4, 0x00), reg_str); print(reg_str);
    print("\n Vertical Total: "); itoa(read_vga_register(0x3D4, 0x06), reg_str); print(reg_str);
    print("\n\nMisc Output: "); uint_to_hex(inb(0x3CC), reg_str); print(reg_str);
}

void cmd_devlist() {
    print("\n=== DETECTED DEVICES ===");
    print("\n\n[Standard Devices]");
    print("\n - PIC (8259): IRQ Controller");
    print("\n - PIT (8253): Timer");
    print("\n - Keyboard Controller (8042)");
    print("\n - VGA Controller");
    print("\n - RTC/CMOS");
    print("\n\n[PCI Devices]");
    print("\nScanning PCI bus...");
    
    int device_count = 0;
    for (unsigned char bus = 0; bus < 2; bus++) {
        for (unsigned char device = 0; device < 32; device++) {
            if (pci_device_exists(bus, device, 0)) {
                unsigned int vendor_device = pci_config_read(bus, device, 0, 0);
                char dev_str[20];
                print("\n Bus "); itoa(bus, dev_str); print(dev_str);
                print(", Device "); itoa(device, dev_str); print(dev_str);
                print(": VID="); uint_to_hex(vendor_device & 0xFFFF, dev_str); print(dev_str);
                print(", DID="); uint_to_hex((vendor_device >> 16) & 0xFFFF, dev_str); print(dev_str);
                device_count++;
            }
        }
    }
    if (!device_count) print("\n No PCI devices detected");
}

void cmd_uptime() {
    print("\n=== SYSTEM UPTIME ===");
    unsigned char hour, minute, second;
    char time_str[20];
    get_rtc_time(&hour, &minute, &second);
    
    // Display current RTC time
    print("\nCurrent RTC Time: "); itoa(hour, time_str); print(time_str); print(":");
    if (minute < 10) print("0"); itoa(minute, time_str); print(time_str); print(":");
    if (second < 10) print("0"); itoa(second, time_str); print(time_str);
    
    // Calculate uptime (time since boot)
    int total_seconds_now = (hour * 3600) + (minute * 60) + second;
    int total_seconds_boot = (boot_hour * 3600) + (boot_minute * 60) + boot_second;
    int uptime_seconds = total_seconds_now - total_seconds_boot;
    
    // Handle day wraparound (if current time < boot time, day changed)
    if (uptime_seconds < 0) {
        uptime_seconds += 86400; // Add 24 hours in seconds
    }
    
    // Convert to hours, minutes, seconds
    int up_hours = uptime_seconds / 3600;
    int up_minutes = (uptime_seconds % 3600) / 60;
    int up_secs = uptime_seconds % 60;
    
    // Display uptime
    print("\nSystem Uptime: ");
    itoa(up_hours, time_str); print(time_str); print(" hours, ");
    itoa(up_minutes, time_str); print(time_str); print(" minutes, ");
    itoa(up_secs, time_str); print(time_str); print(" seconds");
}

void cmd_sysinfo() {
    print("\n=== SYSTEM INFORMATION ===");
    print("\n\nOS: Basic Kernel");
    print("\nArchitecture: x86 (32-bit)");
    
    if (cpuid_supported()) {
        char vendor[13];
        get_cpu_vendor(vendor);
        print("\nCPU: "); print(vendor);
    }
    
    char mem_str[20], time_str[20];
    unsigned int total_mb = probe_memory();
    print("\nRAM: "); itoa(total_mb, mem_str); print(mem_str); print(" MB");
    
    unsigned char hour, minute, second;
    get_rtc_time(&hour, &minute, &second);
    print("\nTime: "); itoa(hour, time_str); print(time_str); print(":");
    if (minute < 10) print("0"); itoa(minute, time_str); print(time_str); print(":");
    if (second < 10) print("0"); itoa(second, time_str); print(time_str);
}

void cmd_portlist() {
    print("\n=== I/O PORT MAP ===");
    print("\n\n[DMA Controller]");
    print("\n 0x00-0x0F: DMA channels 0-3");
    print("\n 0xC0-0xDF: DMA channels 4-7");
    print("\n\n[Interrupt Controllers]");
    print("\n 0x20-0x21: Master PIC (8259)");
    print("\n 0xA0-0xA1: Slave PIC (8259)");
    print("\n\n[Timer]");
    print("\n 0x40-0x43: PIT (8253)");
    print("\n\n[Keyboard]");
    print("\n 0x60: Data port");
    print("\n 0x64: Command/Status port");
    print("\n\n[RTC/CMOS]");
    print("\n 0x70: Index register");
    print("\n 0x71: Data register");
    print("\n\n[VGA]");
    print("\n 0x3C0-0x3CF: VGA registers");
    print("\n 0x3D4-0x3D5: CRT controller");
    print("\n\n[PCI]");
    print("\n 0xCF8: Config address");
    print("\n 0xCFC: Config data");
}

// ============================================================================
// COMMAND DISPATCHER
// ============================================================================

void parse_two_numbers(const char *cmd, int start_pos, int *num1, int *num2) {
    int i = start_pos;
    while (cmd[i] == ' ') i++;
    *num1 = atoi(cmd + i);
    while (cmd[i] != ' ' && cmd[i]) i++;
    while (cmd[i] == ' ') i++;
    *num2 = atoi(cmd + i);
}

void execute_command() {
    command_buffer[command_pos] = '\0';
    
    // Simple commands
    if (strcmp(command_buffer, "clear") == 0) { clear_screen(); }
    else if (starts_with(command_buffer, "echo ")) { print("\n"); print(command_buffer + 5); }
    
    // Math commands
    else if (starts_with(command_buffer, "add ") || starts_with(command_buffer, "sub ") || 
             starts_with(command_buffer, "mul ") || starts_with(command_buffer, "div ")) {
        int num1, num2;
        char op = command_buffer[0];
        parse_two_numbers(command_buffer, 4, &num1, &num2);
        
        if (op == 'd' && num2 == 0) {
            print("\nError: Division by zero!");
        } else {
            int result = (op == 'a') ? num1 + num2 : (op == 's') ? num1 - num2 :
                        (op == 'm') ? num1 * num2 : num1 / num2;
            char result_str[20];
            itoa(result, result_str);
            print("\n");
            print((op == 'a') ? "Sum: " : (op == 's') ? "Difference: " :
                  (op == 'm') ? "Product: " : "Quotient: ");
            print(result_str);
        }
    }
    
    // Info command
    else if (strcmp(command_buffer, "info") == 0) {
        print("\n=== Available Commands ===");
        print("\n\n[Basic Commands]");
        print("\nclear - Clear the screen");
        print("\necho - Display text");
        print("\nadd - Add two numbers");
        print("\nsub - Subtract y from x");
        print("\nmul - Multiply two numbers");
        print("\ndiv - Divide x by y");
        print("\n\n[System Monitoring]");
        print("\nsysinfo - System overview");
        print("\nuptime - System uptime");
        print("\nmemstat - Memory statistics");
        print("\n\n[Device Management]");
        print("\nkbdstat - Keyboard status");
        print("\nvgainfo - VGA information");
        print("\ndevlist - List devices");
        print("\n\n[Hardware Detection]");
        print("\ncpuinfo - CPU information");
        print("\nmeminfo - Memory map");
        print("\nportlist - I/O port list");
    }
    
    // System commands - using function pointer approach would add overhead, stick with if-else
    else if (strcmp(command_buffer, "cpuinfo") == 0) cmd_cpuinfo();
    else if (strcmp(command_buffer, "meminfo") == 0) cmd_meminfo();
    else if (strcmp(command_buffer, "memstat") == 0) cmd_memstat();
    else if (strcmp(command_buffer, "kbdstat") == 0) cmd_kbdstat();
    else if (strcmp(command_buffer, "vgainfo") == 0) cmd_vgainfo();
    else if (strcmp(command_buffer, "devlist") == 0) cmd_devlist();
    else if (strcmp(command_buffer, "uptime") == 0) cmd_uptime();
    else if (strcmp(command_buffer, "sysinfo") == 0) cmd_sysinfo();
    else if (strcmp(command_buffer, "portlist") == 0) cmd_portlist();
    
    // Unknown command
    else {
        print("\nUnknown command: ");
        print(command_buffer);
        print("\nType 'info' for available commands");
    }
    
    print("\n> ");
    command_pos = 0;
}

// ============================================================================
// MAIN KERNEL ENTRY POINT
// ============================================================================

void kernelMain(void) {
    clear_screen();
    get_rtc_time(&boot_hour, &boot_minute, &boot_second);
    print("Made by Saksham & Aditi\n");
    print("Welcome to Basic Kernel!\n");
    print("Type 'info' to see available commands\n");
    print("> ");
    
    char buffer[2] = {0, 0};
    while (1) {
        unsigned char scancode = read_key();
        if (scancode) {
            char c = scancode_to_ascii(scancode);
            if (c) {
                if (c == '\n') {
                    execute_command();
                } else if (c == '\b') {
                    if (command_pos > 0) {
                        command_pos--;
                        backspace();
                    }
                } else if (command_pos < 79) {
                    command_buffer[command_pos++] = c;
                    buffer[0] = c;
                    print(buffer);
                }
            }
        }
    }
}