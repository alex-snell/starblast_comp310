/*
 * Basic template for a reto video game. This C file instructs the GRUB
 * bootloader to put the screen into framebuffer mode. It has a couple basic
 * functions that parse the multiboot header that GRUB generates to determine
 * the dimensions of the screen and the memory address where the framebuffer
 * lives.
 *
 * When GRUB boots our kernel, it reads the multiboot header out of our kernel
 * binary. The multiboot header contains information about our kernel, including
 * some requests that our kernel can make for GRUB to initialize the PC's
 * hardware. This project's multiboot header requests GRUB to set up the screen
 * in framebuffer mode (graphics mode) with a screen dimension of 1024x768. GRUB
 * will configure the hardware according to our kernel's request and generate a
 * multiboot information structure somewhere in memory which tells our kernel
 * how the hardware is actually configured. The multiboot information structure
 * tells us (1) the framebuffer address, (2) the actual dimensions of the
 * screen, and (3) the color depth (number of bits per pixel).
 *
 * This file also provides the drawPixel() function which sets the RGB color of
 * a pixel at a given (x,y) location on the screen.
 *
 */



#include <stdint.h>
#include "video.h"
#include "timer.h"
#include "input.h"
#include "game.h"

#define INFO_TYPE_KERNEL_LOAD_ADDR 0x15
#define INFO_TYPE_CMD_LINE 1
#define INFO_TYPE_BOOTLOADER_NAME 2
#define INFO_TYPE_MEM_MAP 6
#define INFO_EFI_ENTRY_ADDRESS 9
#define INFO_TYPE_MEM_INFO 4
#define INFO_TYPE_BIOS_BOOT_DEVICE 5
#define INFO_TYPE_FRAMEBUFFER_INFO 8
#define INFO_TYPE_ACPI_OLD_RSDP 14
#define INFO_TYPE_DONE 0



// The magic field should contain this.
#define MULTIBOOT2_HEADER_MAGIC         0xe85250d6

#define MULTIBOOT2_FLAGS_VIDINFO        (1<<2)
#define MULTIBOOT2_HEADER_VIDINFO_TAG   5

// Addresses of the partition entries in the MBR partition table.
#define PARTITION_ENTRY_1 (struct partitionEntry*)(0x7dbe)
#define PARTITION_ENTRY_2 (struct partitionEntry*)(0x7dce)
#define PARTITION_ENTRY_3 (struct partitionEntry*)(0x7dde)
#define PARTITION_ENTRY_4 (struct partitionEntry*)(0x7dee)



struct multiboot_header {
  /* Must be MULTIBOOT_MAGIC - see above.  */
  uint32_t magic;
  /* Feature flags.  */
  uint32_t flags;
  uint32_t length;
  /* The above fields plus this one must equal 0 mod 2^32. */
  uint32_t checksum;
};
struct multiboot_framebuffer_tag {
  uint16_t type;
  uint16_t flags;
  uint32_t size;
  uint32_t width;
  uint32_t height;
  uint32_t depth;
  uint32_t x;
};
struct multiboot_tag {
  uint16_t type;
  uint16_t flags;
  uint32_t size;
};

struct multiboot_header mbh  __attribute__((section(".multiboot")))= {
    .magic = MULTIBOOT2_HEADER_MAGIC,
    .flags = 0, // Tell GRUB we are adding info about video info
    .length = 16,
    .checksum = -(16+MULTIBOOT2_HEADER_MAGIC)
};
struct multiboot_framebuffer_tag  gfxtag __attribute__((section(".multiboot")))= {
    .type = MULTIBOOT2_HEADER_VIDINFO_TAG,
    .flags = 1,
    .size = sizeof(struct multiboot_framebuffer_tag),
    .width = 1024,
    .height = 768,
    .depth = 32
};
struct multiboot_tag terminator_tag  __attribute__((section(".multiboot")))= {
    .type = 0,
    .flags = 0,
    .size = sizeof(struct multiboot_tag)
};

uint8_t inb (uint16_t _port) {
    uint8_t rv;
    __asm__ __volatile__ ("inb %1, %0" : "=a" (rv) : "dN" (_port));
    return rv;
}

uint32_t *framebuffer;
static uint32_t framebufferPitch;
static uint32_t framebufferWidth;
static uint32_t framebufferHeight;
static uint32_t framebufferBitsPerPixel;

uint32_t getFramebufferBitsPerPixel() {
    return framebufferBitsPerPixel;
}


void setFramebufferBitsPerPixel(uint8_t bpp) {
    framebufferBitsPerPixel = bpp;
}

void setFramebufferPitch(uint32_t pitch) {
    framebufferPitch = pitch;
}

uint32_t getFramebufferWidth() {
    return framebufferWidth;
}


void setFramebufferWidth(uint32_t width) {
    framebufferWidth = width;
}

uint32_t  getFramebufferHeight() {
    return framebufferHeight;
}
void setFramebufferHeight(uint32_t height) {
    framebufferHeight = height;
}

void setFramebufferAddress(void *base) {
    framebuffer = base;
}


/*
 * parseMultiboot2Info
 *
 * Parses the multiboot 2 information structure to read relevant hardware
 * information.
 */
uint32_t *pMultibootInfo;
int parseMultiboot2Info() {
    uint32_t *p = pMultibootInfo + 2; // skip past initial tag
    unsigned int k = 0;
    unsigned int totalStructSize = *pMultibootInfo;

    while(p < (uint32_t*)((uint8_t*)pMultibootInfo + totalStructSize)){
        unsigned int type = *p;
        unsigned int size = *(p+1);
        switch(type){
        case INFO_TYPE_BOOTLOADER_NAME:
            break;
        case INFO_TYPE_CMD_LINE:
            break;
        case INFO_TYPE_KERNEL_LOAD_ADDR:
            break;
        case INFO_TYPE_MEM_MAP:
            for(k = 0; k < (size-16)/24; k++){
                unsigned int region_base = *(p+4+(k*6));
                unsigned int region_length = *(p+4+(k*6)+2);
                unsigned int region_type = *(p+4+(k*6)+4);
            }
            break;
        case INFO_TYPE_BIOS_BOOT_DEVICE:
            break;
        case INFO_TYPE_ACPI_OLD_RSDP: // Unsupported tags...
        case INFO_TYPE_MEM_INFO:
        case INFO_EFI_ENTRY_ADDRESS:
            break;
        case INFO_TYPE_FRAMEBUFFER_INFO:
            setFramebufferAddress((void*)*(p+2));
            setFramebufferPitch(*(p+4));
            setFramebufferWidth(*(p+5));
            setFramebufferHeight(*(p+6));
            setFramebufferBitsPerPixel(*(p+7) & 0xff);
            break;
        case INFO_TYPE_DONE: // We're done reading the multiboot info struct when we get a type of 0
            return 0;
        default:
            break;
        }
        while(((uint32_t)size & ~7) != (uint32_t)size){ // Round size up to the next 8-byte boundary
            size++;
        }
        p += (size/sizeof(unsigned int)); // skip p past the tag
    }
    return 0;
}


// drawPixel
// 
// Draws one pixel on the screen at the given (x,y) location. The color is a
// 3-byte RGB color value. Each byte controls the intensity of the red, green,
// and blue for the given pixel. For example, RGB for light orange is 0xE09667
//
//          ----- Red intensity
//          |
//          |  ---- Green intensity
//          |  |
//          |  |  --- Blue intensity
//          |  |  |
//          /\ /\ /\
// color: 0xE0 96 67
//
// For example, to color a pixel light orange at location (5,5):
//
//   drawPixel(5,5,0xE09667);
//
void drawPixel(int x, int y, int color) {
    framebuffer[x+(y*framebufferWidth)] = (uint32_t)color;
}

void main() {

    // Bottom must be kept as first line of main() function. Don't put anything
    // above this. This assembly code reads the address of the multiboot header
    // from EBX. Grub places the address in EBX before calling our main function.
    asm("mov %%ebx,%0"
        : "=r"(pMultibootInfo)
        :
        :);

    // Parse the multiboot struct. This is a data structure built by GRUB to
    // communicate key information about the hardware. It tells us the address
    // of the framebuffer and the size of the screen.
    parseMultiboot2Info();
    //Visual additions
    video_init();
    timer_init();
    input_init();
    while(1){
	input_poll();
        game_update_and_render();
        video_flip();
        timer_wait_frame();
    }
}
