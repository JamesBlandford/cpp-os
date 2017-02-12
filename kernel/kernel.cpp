#include <stddef.h> 
#include <stdint.h> 

#include <string.h> 
#include <tty.h>

#include <cpu/multiboot.h>

#include <gfx/vesa.h>

extern "C"
void kernel_main(multiboot_info_t * mb_info, uint32_t stack_size, uintptr_t esp) {
	terminal_initialize(); 
 
	terminal_writestring("Hello, kernel World!\n"); 

	//gdt_install();
	//idt_install();
}