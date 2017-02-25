#include <stddef.h>
#include <stdint.h>

#include <std.h>

#include <arch/i386/timer.h>

#include <tty.h>

#include <cpu/io.h>
#include <cpu/idt.h> 

#include <devices/ata.h>

const uint16_t device_offsets[2][2] = {
    { ATA_0_MASTER, ATA_0_SLAVE },
    { ATA_1_MASTER, ATA_1_SLAVE }
};

uint16_t * ATA::readPIO(int bus, int drive, int size) {
    uint16_t buffer[size/2] = {0};

    uint16_t buffer_byte[size] = {0};

    for(int i = 0; i < size; i++) {
        buffer_byte[i] = inports(device_offsets[bus][drive] + ATA_DATA);

    }

    for(int i = 27; i < 47; i++) {
        // uint16_t is actually TWO char's long!
        terminal_printf("%c", (int)(buffer_byte[i] >> 8));
        terminal_printf("%c", (int)(buffer_byte[i] & 0xFF));
    }

    terminal_writestring("\n");
    return buffer;
}

void ATA::writePIO(int bus, int drive, uint16_t * buffer, int size) {
    for(int i = size; i > 0; i-=2) {
        outports(device_offsets[bus][drive] + ATA_DATA, *buffer);
        *buffer++;
    }
}

void ATA::resetATA(int bus, int drive) {
    // allows us to send more commands after the ATA_STATUS command
    uint16_t base_offset = device_offsets[bus][drive];

    outportb(base_offset + ATA_CTRL, ATA_CTRL_RST);
    outportb(base_offset + ATA_CTRL, 0);
}

bool ATA::wait(int bus, int drive, int mask, int waitForState) {
    uint8_t state;

    uint16_t base_offset = device_offsets[bus][drive];

    uint64_t time = 0;

    while(true) {
        state = inportb(base_offset + ATA_STATUS);

        if((state&mask) == waitForState) { return true; }

        if(time > 10) { return false; }

        PIT::sleep(1);
        time++;
    }

    return false;
}

bool ATA::initializeDevice(int bus, int drive) {
    uint16_t base_offset = device_offsets[bus][drive];

    int flags = ATA_F_ECC | ATA_F_LBA | ATA_F_512_SEC;
	if(drive == 1) flags |= ATA_F_SLAVE;

    // send command ATA_COMMAND_IDENTIFY to the device to grab some of the information
    ATA::wait(bus, drive, ATA_STATUS_BSY, 0);

	outportb(base_offset + ATA_CTRL, 0);
	outportb(base_offset + ATA_COUNT, 0);
	outportb(base_offset + ATA_SECTOR, 0);
	outportb(base_offset + ATA_CYL_LO, 0);
	outportb(base_offset + ATA_CYL_HI, 0);
	outportb(base_offset + ATA_FDH, flags);
	outportb(base_offset + ATA_COMMAND, ATA_COMMAND_IDENTIFY);

    ATA::wait(bus, drive, ATA_STATUS_DRQ, ATA_STATUS_DRQ);

    uint16_t *buffer = ATA::readPIO(bus, drive, 512);

    // terminal_writestring(">>> LE FINIooooo");
    // update_buffer();

    return true;
}

std::vector<ATA_Device> ATA::findATA() {
    std::vector<ATA_Device> found_devices = std::vector<ATA_Device>();

    for(int bus = 0; bus < 2; bus++) {
        for(int drive = 0; drive < 2; drive++) {
            uint16_t base_offset = device_offsets[bus][drive];

            uint8_t connected = inportb(base_offset + ATA_STATUS);
            if(connected == 0xFF) {
        //        terminal_printf("[ATA] Device (%d,%d) not found\n", bus, drive);
                continue;
            } 

            terminal_printf("[ATA] Device (%d,%d): ", bus, drive);

            // We have found an attached ATA device on this bus, let's grab some info on it :)
            ATA::resetATA(bus, drive);

            ATA::initializeDevice(bus, drive);
          
        }
    }
    return found_devices;
}

void init_ata() {
    //timer_phase(1000);
	//set_irq_handler(0, (isr_t)&timer_handler);

    terminal_writestring("\n------------------------------------\n");
    terminal_writestring("Identifying ATA devices...\n");
    std::vector<ATA_Device> ata_devices = ATA::findATA();
    terminal_writestring("\n------------------------------------\n");
}