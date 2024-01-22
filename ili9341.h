#ifndef _ILI9341_H_
#define _ILI9341_H_

#include "platform_mtb_psoc6_spi.h"

/* Mode: 0 (CPOL 0, CPHA 0 */
#define SPI_CPOL        0
#define SPI_CPHA        0
#define IS_LSBFIRST     0   /* Bit order: MSB First */


#ifndef ILI_SPI_FREQ
    #define ILI_SPI_FREQ 20000000UL    /* 20MHz */
#endif


#if defined(ILI_BUS_TYPE_PARALLEL8) || defined(ILI_BUS_TYPE_SPI)
    #define _ILI_DC_CMD()   ILI_PLATFORM_DC_LOW()
    #define _ILI_DC_DATA()  ILI_PLATFORM_DC_HIGH()


    #if defined(ILI_BUS_TYPE_PARALLEL8)
		#define _ILI_WRITE8(byte)  ili_platform_parallel_send8(byte)
        #if defined(ILI_PLATFORM_WR_LOW) && defined(ILI_PLATFORM_WR_HIGH)
            // WR_STROBE is used to repeatedly and rapidly send the last byte. It's optional.
            #define _ILI_PLATFORM_WR_STROBE()   {ILI_PLATFORM_WR_LOW(); ILI_PLATFORM_WR_HIGH();}
        #endif

        #if defined(ILI_PLATFORM_RD_LOW) && defined(ILI_PLATFORM_RD_HIGH)
            // RD_STROBE is not used by the driver yet. Maybe in future!!
            #define _ILI_PLATFORM_RD_STROBE()   {ILI_PLATFORM_RD_LOW(); ILI_PLATFORM_RD_HIGH();}
        #endif
	#elif defined(ILI_BUS_TYPE_SPI)
		#define _ILI_WRITE8(byte)  ili_platform_spi_send8(byte)
	#endif


#else
    #error Bus type undefined. Do either `#define ILI_BUS_TYPE_PARALLEL8` or `#define ILI_BUS_TYPE_SPI`
#endif /* defined(ILI_BUS_TYPE_PARALLEL8) || defined(ILI_BUS_TYPE_SPI) */


__attribute__((always_inline)) static inline void _ili_write_command_8bit(uint8_t cmd)
{
    _ILI_DC_CMD();
    _ILI_WRITE8(cmd);
}

/*
 * inline function to send 8 bit data to the display
 * User need not call it
 */
__attribute__((always_inline)) static inline void _ili_write_data_8bit(uint8_t dat)
{
    _ILI_DC_DATA();
    _ILI_WRITE8(dat);
}

/*
 * inline function to send 16 bit data to the display
 * User need not call it
 */
__attribute__((always_inline)) static inline void _ili_write_data_16bit(uint16_t dat)
{
    _ILI_DC_DATA();
    _ILI_WRITE8((uint8_t)(dat >> 8));
    _ILI_WRITE8((uint8_t)dat);
}

/*
* function prototypes
*/

/**
 * Initialize the SPI or Parallel bus. Must be called before ili_init()
 */
void ili_bus_init(void);

/**
 * De-initialize the SPI or Parallel bus. Useful when bus is being shared with other devices
 */
void ili_bus_deinit(void);

/**
 * Initialize the display driver.  Can be called only if ili_bus_init() is called
 */
void ili_init(void);

/**
 * Set an area for drawing on the display with start row,col and end row,col.
 * User don't need to call it usually, call it only before some functions who don't call it by default.
 * @param x1 start column address.
 * @param y1 start row address.
 * @param x2 end column address.
 * @param y2 end row address.
 */
void ili_set_address_window(uint16_t x1, uint16_t y1, uint16_t x2, uint16_t y2);

/**
 * Fills `len` number of pixels with `color`.
 * Call ili_set_address_window() before calling this function.
 * @param color 16-bit RGB565 color value
 * @param len 32-bit number of pixels
 */
void ili_fill_color(uint16_t color, uint32_t len);

/**
 * Fills `len` number of pixels with `color`.
 * Call ili_set_address_window() before calling this function.
 * @param color_buffer Buffer of 16-bit RGB565 color values
 * @param len 32-bit number of pixels
 */
void ili_draw_pixels_buffer(uint16_t *color_buffer, uint32_t len);

/**
 * Draw a line from (x0,y0) to (x1,y1) with `width` and `color`.
 * @param x0 start column address.
 * @param y0 start row address.
 * @param x1 end column address.
 * @param y1 end row address.
 * @param width width or thickness of the line
 * @param color 16-bit RGB565 color of the line
 */
void ili_draw_line(uint16_t x0, uint16_t y0, uint16_t x1, uint16_t y1, uint8_t width, uint16_t color);

/**
 * Experimental
 * Draw a rectangle without filling it
 * @param x start column address.
 * @param y start row address
 * @param w Width of rectangle
 * @param h height of rectangle
 */
void ili_draw_rectangle(uint16_t x, uint16_t y, uint16_t w, uint16_t h, uint16_t color);

/**
 * Rotate the display clockwise or anti-clockwie set by `rotation`
 * @param rotation Type of rotation. Supported values 0, 1, 2, 3
 */
void ili_rotate_display(uint8_t rotation);

/**
 * Get display's current width, height, and rotation
 * @param width Pointer to width data
 * @param height Pointer to height data
 * @param rotation Pointer to rotation data (0,1,2,3)
 */
void ili_get_display_size(uint16_t *width, uint16_t *height, uint8_t *rotation);

/**
 * Fills a rectangular area with `color`.
 * Before filling, performs area bound checking
 * @param x Start col address
 * @param y Start row address
 * @param w Width of rectangle
 * @param h Height of rectangle
 * @param color 16-bit RGB565 color
 */
void ili_fill_rect(uint16_t x, uint16_t y, uint16_t w, uint16_t h, uint16_t color);

/*
 * Same as `ili_fill_rect()` but does not do bound checking, so it's slightly faster
 */
void ili_fill_rect_fast(uint16_t x, uint16_t y, uint16_t w, uint16_t h, uint16_t color);

/**
 * Fill the entire display (screen) with `color`
 * @param color 16-bit RGB565 color
 */
void ili_fill_screen(uint16_t color);

/**
 * Draw a pixel at a given position with `color`
 * @param x Start col address
 * @param y Start row address
 */
void ili_draw_pixel(uint16_t x, uint16_t y, uint16_t color);


/* --------------------- Private functions -------------------- */
/*
 * Called by ili_draw_line().
 * User need not call it
 */
void _ili_plot_line_low(uint16_t x0, uint16_t y0, uint16_t x1, uint16_t y1, uint8_t width, uint16_t color);

/*
 * Called by ili_draw_line().
 * User need not call it
 */
void _ili_plot_line_high(uint16_t x0, uint16_t y0, uint16_t x1, uint16_t y1, uint8_t width, uint16_t color);

/*
 * Called by ili_draw_line().
 * User need not call it
 */
void _ili_draw_fast_h_line(uint16_t x0, uint16_t y0, uint16_t x1, uint8_t width, uint16_t color);

/*
 * Called by ili_draw_line().
 * User need not call it
 */
void _ili_draw_fast_v_line(uint16_t x0, uint16_t y0, uint16_t y1, uint8_t width, uint16_t color);


/* --------------------- ILI9341 registers -------------------- */
#define ILI_NOP     0x00
#define ILI_SWRESET 0x01
#define ILI_RDDID   0xD3
#define ILI_RDDST   0x09

#define ILI_SLPIN   0x10
#define ILI_SLPOUT  0x11
#define ILI_PTLON   0x12
#define ILI_NORON   0x13

#define ILI_RDMODE  0x0A
#define ILI_RDMADCTL  0x0B
#define ILI_RDPIXFMT  0x0C
#define ILI_RDIMGFMT  0x0D
#define ILI_RDSELFDIAG  0x0F

#define ILI_INVOFF  0x20
#define ILI_INVON   0x21
#define ILI_GAMMASET 0x26
#define ILI_DISPOFF 0x28
#define ILI_DISPON  0x29

#define ILI_CASET   0x2A
#define ILI_PASET   0x2B
#define ILI_RAMWR   0x2C
#define ILI_RAMRD   0x2E

#define ILI_PTLAR   0x30
#define ILI_MADCTL  0x36
#define ILI_PIXFMT  0x3A

#define ILI_FRMCTR1 0xB1
#define ILI_FRMCTR2 0xB2
#define ILI_FRMCTR3 0xB3
#define ILI_INVCTR  0xB4
#define ILI_DFUNCTR 0xB6

#define ILI_PWCTR1  0xC0
#define ILI_PWCTR2  0xC1
#define ILI_PWCTR3  0xC2
#define ILI_PWCTR4  0xC3
#define ILI_PWCTR5  0xC4
#define ILI_VMCTR1  0xC5
#define ILI_VMCTR2  0xC7

#define ILI_RDID1   0xDA
#define ILI_RDID2   0xDB
#define ILI_RDID3   0xDC
#define ILI_RDID4   0xDD

#define ILI_GMCTRP1 0xE0
#define ILI_GMCTRN1 0xE1

// #define ILI_PWCTR6  0xFC
#define ILI_MAD_RGB 0x00
#define ILI_MAD_BGR 0x08
#define ILI_MAD_COLOR_ORDER ILI_MAD_BGR



#define	ILI_R_POS_RGB   11	// Red last bit position for RGB display
#define	ILI_G_POS_RGB   5 	// Green last bit position for RGB display
#define	ILI_B_POS_RGB   0	// Blue last bit position for RGB display

#define	ILI_RGB(R,G,B) \
	(((uint16_t)(R >> 3) << ILI_R_POS_RGB) | \
	((uint16_t)(G >> 2) << ILI_G_POS_RGB) | \
	((uint16_t)(B >> 3) << ILI_B_POS_RGB))

#define	ILI_R_POS_BGR   0	// Red last bit position for BGR display
#define	ILI_G_POS_BGR   5 	// Green last bit position for BGR display
#define	ILI_B_POS_BGR   11	// Blue last bit position for BGR display

#define	ILI_BGR(R,G,B) \
	(((uint16_t)(R >> 3) << ILI_R_POS_BGR) | \
	((uint16_t)(G >> 2) << ILI_G_POS_BGR) | \
	((uint16_t)(B >> 3) << ILI_B_POS_BGR))

#define ILI_SWAP(a, b)		{uint16_t temp; temp = a; a = b; b = temp;}

//------------------------------------------------------------------------
#endif /* _ILI9341_H_ */
