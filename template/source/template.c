#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>

static uint32_t *porta = (uint32_t *)0xFF80002C;
static uint32_t pctra_clr = 0xFFF0FFFF;
static uint32_t pctra_set = 0x00A0000;
static uint32_t *videobase = (uint32_t *)0xA05F8000;
static uint32_t *cvbsbase = (uint32_t *)0xA0702C00;
uint32_t *vram_l = (uint32_t *)0xA5000000;
uint16_t *vram_s = (uint16_t *)0xA5000000;

uint32_t vram_size = 0;
uint32_t vram_config = 0;

/* Check type of A/V cable connected
        0 = VGA
        1 = ---
        2 = RGB
        3 = Composite
   From Marcus Comstedt's video.s
*/
uint16_t check_video_cable()
{
    /* Set PORT8 and PORT9 to input */
    *porta = (*porta & pctra_clr) | pctra_set;
    /* Read PORT8 and PORT9 */
    return ((*(uint16_t*)(porta + 1)) >> 8) & 3;
}

/* From Marcus Comstedt's video.s and old libdream

      Set up video registers to the desired
      video mode (only 640*480 supported right now)

      Note:	This function does not currently initialize
            all registers, but assume that the boot ROM
        has set up reasonable defaults for syncs etc.

      TODO:	PAL

      cabletype (0=VGA, 2=RGB, 3=Composite)
      pixel mode (0=RGB555, 1=RGB565, 3=RGB888)
*/
void init_video(uint16_t cable_type, int pixel_mode)
{
    int disp_lines, size_modulo, il_flag;

    // Looks up bytes per pixel as a shift value
    unsigned char bppshifttab[] = { 1, 1, 0, 2 };
    unsigned long bppshift = bppshifttab[pixel_mode];

    // Save the video memory size
    vram_size = (640*480*4) << bppshift;
    vram_config = pixel_mode;

    // Set border color
    *(videobase + 0x10) = 0;

    // Set pixel clock and color mode
    pixel_mode = (pixel_mode << 2) | 1; // Color mode and DE
    disp_lines = (240 / 2) << 1;
    if (cable_type != 3) {
        // Double # of display lines for VGA and S-Video
        disp_lines <<= 1;       // Double line count
        pixel_mode |= 0x800000;     // Set clock double
    }
    *(videobase + 0x11) = pixel_mode;

    // Set video base address (right at the top)
    *(videobase + 0x14) = 0;

    // Video base address for short fields should be offset by one line
    *(videobase + 0x15) = 640 << bppshift;

    // Set screen size and modulo, and interlace flag
    il_flag = 0x100;    // Set Video Output Enable
    size_modulo = 0;
    if (cable_type != 0) {  // non-VGA => interlace
        // Add one line to offset => display every other line
        size_modulo = (640/4) << bppshift;
        il_flag |= 0x50;    // Enable interlace
    }
    *(videobase + 0x17) = ((size_modulo+1) << 20) | (disp_lines << 10)
        | (((640/4) << bppshift) - 1);
    *(videobase + 0x34) = il_flag;

    // Set vertical pos and border
    *(videobase + 0x37) = (24 << 16) | (24 + disp_lines);
    *(videobase + 0x3c) = (24 << 16) | 24;

    // Horizontal pos
    *(videobase + 0x3b) = 0xa4;

    // Select RGB/CVBS
    if (cable_type == 3)
        *cvbsbase = 3 << 8;
    else
        *cvbsbase = 0;
}

int main(int argc, char **argv)
{
    uint16_t type = check_video_cable();
    int i;
    init_video(type, 1); // 1 means RGB565

    for (i = 0; i < vram_size; i++)
        vram_s[i] = 0x588F;

    for(;;);

	return 0;
}
