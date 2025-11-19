# Basic x86-32 Kernel Project

## Overview

This is a **32-bit x86 bare-metal operating system kernel** built from scratch using Assembly and C. The kernel implements a command-line interface with comprehensive system monitoring and hardware introspection capabilities. It's designed to run on QEMU emulation or compatible x86-32 hardware.

**Authors:** Saksham & Aditi

---

## Table of Contents

1. [Features](#features)
2. [Project Structure](#project-structure)
3. [Technical Architecture](#technical-architecture)
4. [Build Instructions](#build-instructions)
5. [Running the Kernel](#running-the-kernel)
6. [Available Commands](#available-commands)
7. [Hardware Support](#hardware-support)
8. [System Specifications](#system-specifications)
9. [Design Decisions](#design-decisions)
10. [Future Improvements](#future-improvements)

---

## Features

### Core Functionality
- **32-bit Protected Mode** execution (x86 architecture)
- **VGA Text Mode Display** - 80x25 character display in color
- **Keyboard Input Handling** - Full ASCII keyboard support with Shift modifier
- **Command-line Interface** - UNIX-like shell prompt with command parsing
- **Hardware Detection** - Comprehensive system hardware enumeration

### System Monitoring
- CPU information (vendor, brand, feature flags)
- Memory detection and reporting
- Keyboard controller status monitoring
- VGA controller information
- RTC/CMOS time reading
- System uptime calculation
- I/O port mapping

### Hardware Capabilities
- CPUID support detection and execution
- PCI device enumeration
- CMOS/RTC time reading
- Keyboard interrupt handling
- Port-mapped I/O operations
- VGA register access

### Built-in Commands
- **System Info:** `sysinfo`, `cpuinfo`, `meminfo`, `memstat`, `uptime`
- **Device Status:** `kbdstat`, `vgainfo`, `devlist`, `portlist`
- **Utilities:** `echo`, `clear`, `add`, `sub`, `mul`, `div`
- **Help:** `info`

---

## Project Structure

### Files Overview

```
kernel/
├── kernel.c          # Main kernel logic and command implementations (~21KB)
├── kernel.asm        # Boot entry point in 32-bit assembly
├── port_io.asm       # Low-level port I/O functions
├── link.ld           # Linker script for ELF32-i386 format
├── code.txt          # Build and run instructions
└── README.md         # This file
```

### File Descriptions

#### `kernel.asm`
- **Purpose:** Bootstrap code for kernel startup
- **Content:**
  - Multiboot header with magic numbers (0x1BADB002) for bootloader recognition
  - Stack space allocation (8KB)
  - CPU initialization (CLI - disable interrupts)
  - Entry point to `kernelMain()` function
  - Halt instruction after kernel termination

#### `port_io.asm`
- **Purpose:** Low-level hardware I/O operations
- **Functions:**
  - `inb(port)` - Read single byte from I/O port
  - `outb(port, value)` - Write single byte to I/O port
- **Usage:** Essential for accessing hardware devices (keyboard, timer, VGA, etc.)

#### `link.ld`
- **Purpose:** Linker script for kernel binary layout
- **Specifications:**
  - Format: ELF32-i386 (32-bit x86)
  - Entry point: `start` symbol
  - Kernel base address: `0x100000` (1MB boundary - standard for kernels)
  - Sections: .text (code), .data (initialized data), .bss (uninitialized data)

#### `kernel.c`
- **Purpose:** Main kernel implementation
- **Size:** ~21KB (comprehensive for a basic kernel)
- **Major Components:**
  1. **VGA Display Functions**
     - Direct memory access to video buffer at 0xB8000
     - Text mode output with color attributes
     - Cursor position management
     - Screen scrolling when full

  2. **Keyboard Input**
     - Scancode-to-ASCII conversion
     - Shift key support
     - Two-array lookup for normal and shifted characters
     - PS/2 keyboard controller handling

  3. **CPUID Interface**
     - CPU vendor and brand retrieval
     - Feature flag detection
     - Support detection (FPU, TSC, MSR, MMX, SSE, SSE2, SSE3, etc.)

  4. **Memory Detection**
     - Runtime memory probing up to 256MB
     - Test pattern validation
     - Memory size in megabytes reporting

  5. **VGA Hardware Access**
     - CRT controller register reading
     - VGA mode detection (color vs. monochrome)
     - Display parameter reporting

  6. **RTC/CMOS Functions**
     - Real-time clock time reading
     - BCD to binary conversion
     - Boot time storage and uptime calculation

  7. **PCI Device Enumeration**
     - Configuration address space access
     - Bus/device/function scanning
     - Vendor/device ID reporting

  8. **Utility Functions**
     - String manipulation (strcmp, strcat, etc.)
     - Number conversion (itoa, atoi, uint_to_hex)
     - Command parsing and dispatch

#### `code.txt`
- **Purpose:** Complete build and execution instructions
- **Contains:** Step-by-step compilation, linking, and QEMU launch commands

---

## Technical Architecture

### Memory Layout

```
0x00000000 ┌─────────────────┐
           │   Real Mode     │ 
           │   (640KB)       │
0x0009FFFF └─────────────────┘
0x000A0000 ┌─────────────────┐
           │   Video RAM     │ (VGA: 0xB8000-0xBFFFF)
0x000BFFFF └─────────────────┘
0x00100000 ┌─────────────────┐ ← Kernel loaded here (1MB boundary)
           │   Kernel Code   │
           │   (kernel.bin)  │
0xXXXXXXXX └─────────────────┘
           │   Available RAM │
0xFFFFFFFF └─────────────────┘ (End of 4GB address space)
```

### Execution Flow

```
QEMU/Bootloader
       ↓
    [GRUB]
       ↓
  kernel.asm (start)
       ↓
  Initialize CPU
       ↓
  Call kernelMain()
       ↓
  kernelMain() initializes:
  - Clear screen
  - Get boot time from RTC
  - Print welcome message
       ↓
  Main Loop:
  - Read keyboard input
  - Convert scancode to ASCII
  - Buffer commands
  - Execute on Enter
  - Display results
```

### I/O Port Mapping

| Port Range | Device | Function |
|------------|--------|----------|
| 0x00-0x0F | DMA | Data transfer control |
| 0x20-0x21 | Master PIC | Interrupt controller |
| 0x40-0x43 | PIT | System timer |
| 0x60, 0x64 | Keyboard | Data and status |
| 0x70-0x71 | RTC/CMOS | Real-time clock |
| 0x3C0-0x3CF | VGA | Attribute controller |
| 0x3D4-0x3D5 | VGA | CRT controller |
| 0xCF8-0xCFC | PCI | Configuration access |

### Interrupt Handling

**Current Status:** Interrupts are **disabled** (CLI instruction in kernel.asm)
- No interrupt handlers implemented
- Keyboard uses polling instead of IRQ
- System is single-threaded

---

## Build Instructions

### Prerequisites

Install required build tools on Ubuntu/Debian:

```bash
sudo apt install -y build-essential nasm binutils-i686-linux-gnu \
    grub-pc-bin grub-common xorriso qemu-system-x86 file gcc-i686-linux-gnu
```

### Build Steps

```bash
# 1. Assemble boot code
nasm -f elf32 kernel.asm -o kernel_asm.o

# 2. Assemble port I/O functions
nasm -f elf32 port_io.asm -o port_io.o

# 3. Compile C code (32-bit, freestanding)
i686-linux-gnu-gcc -m32 -c kernel.c -o kernel_c.o -ffreestanding -O2 -Wall

# 4. Link all object files
ld -m elf_i386 -T link.ld -o kernel.bin kernel_asm.o kernel_c.o port_io.o

# 5. Verify kernel is valid
file kernel.bin

# 6. Create bootable ISO
mkdir -p iso/boot/grub
cp kernel.bin iso/boot/

# 7. Configure GRUB bootloader
cat > iso/boot/grub/grub.cfg << EOF
set timeout=0
set default=0
menuentry "My First OS" {
    multiboot /boot/kernel.bin
    boot
}
EOF

# 8. Create bootable ISO image
grub-mkrescue -o myos.iso iso

# 9. Run in QEMU
qemu-system-i386 -cdrom myos.iso
```

### Build Flags Explained

- `-m32`: Compile for 32-bit architecture
- `-ffreestanding`: No standard library, bare-metal environment
- `-O2`: Level 2 optimization
- `-Wall`: Enable all warnings
- `-T link.ld`: Use custom linker script

---

## Running the Kernel

### QEMU Command

```bash
qemu-system-i386 -cdrom myos.iso
```

### QEMU with Additional Options

```bash
# With specific RAM size
qemu-system-i386 -cdrom myos.iso -m 512M

# With additional CPU info
qemu-system-i386 -cdrom myos.iso -cpu core2duo

# With debugging
qemu-system-i386 -cdrom myos.iso -d guest_errors
```

### Expected Output

```
Made by Saksham & Aditi
Welcome to Basic Kernel!
Type 'info' to see available commands
> 
```

Then you can type commands.

---

## Available Commands

### System Information Commands

#### `sysinfo`
Displays basic system overview:
- OS name and version
- CPU vendor
- Total RAM
- Current system time

**Example:**
```
> sysinfo

=== SYSTEM INFORMATION ===

OS: Basic Kernel
Architecture: x86 (32-bit)
CPU: GenuineIntel
RAM: 512 MB
Time: 14:23:45
```

#### `cpuinfo`
Detailed CPU information including vendor, brand, and supported features:
- Vendor ID (Intel, AMD, etc.)
- CPU brand string
- Feature flags (FPU, TSC, MMX, SSE, SSE2, SSE3, etc.)
- Hex dump of feature flags

**Example:**
```
> cpuinfo

=== CPU INFORMATION ===

Vendor: GenuineIntel
Brand: Intel(R) Core(TM) i7-...
Features (EDX): 0x0183F9FF
Features (ECX): 0x00000001

Supported: FPU TSC MSR MMX SSE SSE2
```

#### `meminfo`
Memory layout and configuration:
- Total detected RAM
- Conventional memory (640KB)
- Video memory range
- Extended memory

**Example:**
```
> meminfo

=== MEMORY INFORMATION ===

Total RAM detected: 512 MB
Lower Memory: 640 KB (conventional)
Video Memory: 0xA0000-0xBFFFF (VGA)
Extended Memory: 511 MB
```

#### `memstat`
Memory statistics:
- Total available memory
- Kernel size
- Free memory

**Example:**
```
> memstat

=== MEMORY STATISTICS ===

Total: 524288 KB
Kernel: ~1 MB
Available: ~523264 KB
```

#### `uptime`
System uptime since kernel started:
- Current RTC time
- Boot time
- Uptime in hours, minutes, seconds
- Handles day wraparound

**Example:**
```
> uptime

=== SYSTEM UPTIME ===

Current RTC Time: 14:23:45
System Uptime: 2 hours, 15 minutes, 30 seconds
```

### Device Status Commands

#### `kbdstat`
Keyboard controller status:
- Status register value (hex)
- Status flags (OBF, IBF, SYS, CMD, AUXB, TIMEOUT, PERR)
- Individual bit interpretation

**Example:**
```
> kbdstat

=== KEYBOARD STATUS ===

Status Register: 0x00000005
Flags: OBF IBF
```

#### `vgainfo`
VGA controller information:
- Display mode (color/monochrome)
- Text mode dimensions
- Video memory address
- CMOS register values

**Example:**
```
> vgainfo

=== VGA INFORMATION ===

Mode: Color
Text Mode: 80x25
Video Memory: 0xB8000

CRET Registers:
 Horizontal Total: 159
 Vertical Total: 127

Misc Output: 0x000000A3
```

#### `devlist`
List all detected devices:
- Standard devices (PIC, PIT, Keyboard, VGA, RTC)
- PCI devices with vendor/device IDs

**Example:**
```
> devlist

=== DETECTED DEVICES ===

[Standard Devices]
 - PIC (8259): IRQ Controller
 - PIT (8253): Timer
 - Keyboard Controller (8042)
 - VGA Controller
 - RTC/CMOS

[PCI Devices]
Scanning PCI bus...
 Bus 0, Device 0: VID=0x8086, DID=0x1234
```

### Hardware Detection Commands

#### `portlist`
Complete I/O port map reference:
- DMA controller ports
- Interrupt controller ports
- Timer ports
- Keyboard ports
- RTC/CMOS ports
- VGA ports
- PCI configuration ports

**Example:**
```
> portlist

=== I/O PORT MAP ===

[DMA Controller]
 0x00-0x0F: DMA channels 0-3
 0xC0-0xDF: DMA channels 4-7

[Interrupt Controllers]
 0x20-0x21: Master PIC (8259)
 0xA0-0xA1: Slave PIC (8259)
...
```

### Utility Commands

#### `echo [text]`
Display text on screen:

**Example:**
```
> echo Hello, Kernel!

Hello, Kernel!
```

#### `clear`
Clear the screen and reset cursor

**Example:**
```
> clear

(screen clears)
>
```

#### `add [x] [y]`
Add two numbers:

**Example:**
```
> add 15 27

Sum: 42
```

#### `sub [x] [y]`
Subtract y from x:

**Example:**
```
> sub 50 20

Difference: 30
```

#### `mul [x] [y]`
Multiply two numbers:

**Example:**
```
> mul 6 7

Product: 42
```

#### `div [x] [y]`
Divide x by y (with zero-division check):

**Example:**
```
> div 100 4

Quotient: 25

> div 10 0

Error: Division by zero!
```

#### `info`
Display all available commands with descriptions:

```
> info

=== Available Commands ===

[Basic Commands]
clear - Clear the screen
echo - Display text
add - Add two numbers
...
```

---

## Hardware Support

### Supported Interfaces

#### CPU & Features
- ✅ CPUID instruction support detection
- ✅ CPU vendor identification
- ✅ CPU brand string retrieval
- ✅ Feature flag detection (FPU, TSC, MSR, MMX, SSE variants)
- ✅ 32-bit protected mode execution

#### Memory
- ✅ Memory size probing (up to 256MB)
- ✅ Physical address access
- ✅ Memory region identification

#### Display
- ✅ VGA text mode (80x25)
- ✅ Color text attributes
- ✅ Direct video memory access (0xB8000)
- ✅ VGA register access
- ✅ CRT controller status

#### Input
- ✅ PS/2 keyboard controller
- ✅ ASCII character decoding
- ✅ Shift key support
- ✅ Backspace handling
- ✅ Polling-based input (no IRQ)

#### System Devices
- ✅ RTC/CMOS reading
- ✅ PIC (Programmable Interrupt Controller) - basic I/O
- ✅ PIT (Programmable Interval Timer) - basic I/O
- ✅ PCI bus enumeration
- ✅ Device detection

#### Emulation Support
- ✅ QEMU x86-32 emulation
- ✅ QEMU CPU variants (core2duo, etc.)
- ✅ Bootloader (GRUB)

### Known Limitations

#### Not Implemented
- ❌ Interrupt handlers (interrupts disabled)
- ❌ Multitasking/processes
- ❌ Virtual memory/paging
- ❌ File system
- ❌ Network stack
- ❌ Sound/audio
- ❌ USB support
- ❌ Graphics beyond VGA text mode
- ❌ Dynamic memory allocation (malloc)
- ❌ Power management

#### Constraints
- Single core execution
- No privilege separation
- Limited stack (8KB)
- No exception handling
- Polling-only I/O
- Text-mode display only

---

## System Specifications

### Execution Environment

| Specification | Value |
|---|---|
| Processor Mode | 32-bit Protected Mode |
| Architecture | x86 (Intel-compatible) |
| Kernel Base Address | 0x100000 (1MB) |
| Stack Size | 8192 bytes (8KB) |
| Bootloader | GRUB (Multiboot standard) |
| Emulation | QEMU system x86-i386 |

### Memory Segments

| Segment | Address | Size | Purpose |
|---|---|---|---|
| Conventional | 0x000000 | 640 KB | Real mode legacy |
| Video Buffer | 0xA0000 | 128 KB | VGA text mode |
| Video Memory | 0xB8000 | 32 KB | Kernel text output |
| Kernel | 0x100000 | ~1-2 MB | Executable code |
| Available | Above kernel | Remainder | Future use |

### Keyboard Support

| Feature | Status |
|---|---|
| ASCII characters (a-z, 0-9) | ✅ Full support |
| Special characters (! @ # $ etc) | ✅ Full support |
| Shift key modifiers | ✅ Supported |
| Number pad | ⚠️ Partial |
| Function keys | ❌ Not supported |
| Alt/Ctrl | ❌ Not supported |
| Backspace | ✅ Supported |
| Enter/Return | ✅ Supported |

### Command Buffer

- **Size:** 80 characters (fits single terminal line)
- **Input method:** Character-by-character accumulation
- **Output:** On Enter keypress
- **Backspace:** Supported for editing

---

## Design Decisions

### Architecture Choices

#### 1. **32-bit Protected Mode**
- **Why:** Modern x86 standard, 4GB address space, hardware protection
- **Alternative:** 16-bit real mode (limited to 1MB)
- **Trade-off:** More complex but significantly more capable

#### 2. **C + Assembly Hybrid**
- **Why:** C for portability and readability, Assembly for hardware-specific tasks
- **Kernel core:** C (higher-level logic)
- **Boot/I/O:** Assembly (low-level operations)

#### 3. **VGA Direct Memory Access**
- **Why:** Simple, fast, no driver overhead
- **Memory:** 0xB8000 for 80x25 text mode
- **Format:** Each character is 2 bytes (ASCII + color attribute)

#### 4. **Polling-based Input**
- **Why:** Simpler than interrupt handlers, sufficient for demonstration
- **Trade-off:** Less efficient but no IRQ setup needed

#### 5. **Multiboot Bootloader**
- **Why:** Standard bootloader interface, works with GRUB
- **Magic:** 0x1BADB002 for bootloader recognition

#### 6. **Memory Probing Algorithm**
- **Why:** Runtime detection works with any QEMU RAM configuration
- **Method:** Test pattern write/read to different memory addresses
- **Limit:** Probes up to 256MB

### Optimizations

1. **Compact Keyboard Maps:** Statically-allocated scancode lookup arrays
2. **Inline String Functions:** No function call overhead for simple operations
3. **Direct Register Access:** Minimal abstraction for I/O ports
4. **Lazy Initialization:** Only detect devices when needed

### Code Organization

The kernel.c is organized into logical sections:
1. Utility functions (strings, math)
2. VGA display functions
3. Keyboard input functions
4. CPU identification (CPUID)
5. Memory detection
6. Hardware detection (VGA, keyboard, RTC, PCI)
7. Command implementations
8. Command dispatcher
9. Main kernel entry point

---

## Future Improvements

### Immediate Enhancements

1. **Interrupt Handlers**
   - Implement IDT (Interrupt Descriptor Table)
   - Set up IRQ handlers
   - Enable interrupts
   - Timer-based uptime instead of polling

2. **Enhanced Keyboard**
   - Function keys support
   - Alt/Ctrl modifiers
   - Command history/recall
   - Tab completion

3. **Memory Management**
   - Physical memory allocator
   - Paging (virtual memory)
   - Heap management (malloc/free)

### Medium-Term Goals

4. **Multi-tasking**
   - Task scheduler
   - Context switching
   - Process isolation

5. **File System**
   - FAT12/FAT32 support
   - Directory structure
   - File I/O functions

6. **Graphics**
   - VESA BIOS Extensions (VBE)
   - Graphical framebuffer
   - Bitmap rendering

7. **Device Drivers**
   - Serial port driver
   - ATA/SATA disk driver
   - USB controller support

### Long-Term Vision

8. **Advanced Features**
   - Virtual memory/paging
   - Protected memory domains
   - System call interface
   - Dynamic linking/modules

9. **System Services**
   - Shell implementation
   - Command interpreter
   - User authentication
   - System utilities

10. **Performance**
    - Assembly optimization
    - Boot-time reduction
    - Memory-efficient structures

---

## Compilation Artifacts

After building, you'll have:

```
kernel_asm.o      - Assembled boot code
kernel_c.o        - Compiled C code
port_io.o         - Assembled I/O functions
kernel.bin        - Final kernel binary (ELF32-i386)
iso/               - ISO directory structure
myos.iso          - Bootable ISO image
```

### File Sizes (Approximate)
- kernel.bin: ~10-15 KB
- myos.iso: ~15-20 MB (includes GRUB)

---

## Testing & Debugging

### QEMU Debugging

```bash
# Enable debug output
qemu-system-i386 -cdrom myos.iso -d guest_errors -D qemu.log

# Enable CPU tracing (verbose, slow)
qemu-system-i386 -cdrom myos.iso -d cpu_exec

# Monitor mode (press Ctrl+Alt+2)
qemu-system-i386 -cdrom myos.iso -monitor stdio
```

### Manual Testing Checklist

- [ ] Kernel boots and displays welcome message
- [ ] `sysinfo` command works
- [ ] `cpuinfo` displays CPU vendor
- [ ] `meminfo` shows correct RAM size
- [ ] `echo` command works
- [ ] Math commands (`add`, `sub`, `mul`, `div`) work
- [ ] Keyboard input works for all characters
- [ ] Backspace editing works
- [ ] `clear` command works
- [ ] `uptime` shows positive uptime

---

## References & Resources

### x86-32 Documentation
- Intel 64 and IA-32 Architectures Software Developer's Manual
- OSDev.org - Operating System Development
- Multiboot Specification

### Bootloader
- GNU GRUB Documentation
- Multiboot Protocol Reference

### Kernel Development
- Linux Kernel Documentation
- Operating Systems: Three Easy Pieces (OSTEPBook)

---

## License

Educational project - free to modify and distribute for learning purposes.

---

## Contact & Support

**Project Authors:** Saksham & Aditi

For questions or improvements, refer to the source files and comments within kernel.c.

---

## Quick Reference

### Common Tasks

**Compile only (no ISO):**
```bash
make kernel.bin
```

**Run with 256MB RAM:**
```bash
qemu-system-i386 -cdrom myos.iso -m 256M
```

**Check CPU features on host:**
```bash
cpuid | head -20
```

**Extract kernel binary:**
```bash
file kernel.bin
strings kernel.bin | head
```

---

Generated: November 2025
