/*
MIT License

Copyright (c) 2020-2024 Avra Mitra

Permission is hereby granted, free of charge, to any person obtaining a copy
of this software and associated documentation files (the "Software"), to deal
in the Software without restriction, including without limitation the rights
to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
copies of the Software, and to permit persons to whom the Software is
furnished to do so, subject to the following conditions:

The above copyright notice and this permission notice shall be included in all
copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
SOFTWARE.
*/
#include <stdlib.h>
#include <ili9341.h>

/* Number of pixels in the temporary display buffer.
 * The temporary buffer is used only by `ili_fill_color()` function
 */
#define ILI_TMP_DISP_BUF_PX_CNT    256

//TFT width and height default global variables
static uint16_t g_ili_tftwidth = 240;
static uint16_t g_ili_tftheight = 320;
static uint8_t  g_rotation = 0;

#if defined(ILI_BUS_TYPE_SPI)
/*used by `ili_fill_color()` function*/
static uint16_t g_tmp_disp_buffer[ILI_TMP_DISP_BUF_PX_CNT];
#endif

void ili_bus_init()
{
#if defined(ILI_BUS_TYPE_SPI)
	ili_platform_spi_init(ILI_SPI_FREQ, SPI_CPOL, SPI_CPHA, IS_LSBFIRST); // Freq: 20MHz (default), CPOL: 0, CPHA: 0, Bit order: MSB First)
#else
	ili_platform_parallel_init();
#endif
#if defined(ILI_PLATFORM_CS_LOW)
	ILI_PLATFORM_CS_LOW();
#endif

}

void ili_bus_deinit()
{
	ILI_PLATFORM_CS_HIGH();
#if defined(ILI_BUS_TYPE_SPI)
	ili_platform_spi_deinit();
#else
	ili_platform_parallel_deinit();
#endif
}


/**
 * Set an area for drawing on the display with start row,col and end row,col.
 * User don't need to call it usually, call it only before some functions who don't call it by default.
 * @param x start column address.
 * @param y start row address.
 * @param w width.
 * @param h Height.
 */
void ili_set_address_window(uint16_t x, uint16_t y, uint16_t w, uint16_t h)
{
    uint16_t x2 = x + w - 1;
    uint16_t y2 = y + h - 1;

    _ili_write_command_8bit(ILI_CASET);
    _ILI_DC_DATA();
    _ILI_WRITE8((uint8_t)(x >> 8));
    _ILI_WRITE8((uint8_t)x);
    _ILI_WRITE8((uint8_t)(x2 >> 8));
    _ILI_WRITE8((uint8_t)x2);


    _ili_write_command_8bit(ILI_PASET);
    _ILI_DC_DATA();
    _ILI_WRITE8((uint8_t)(y >> 8));
    _ILI_WRITE8((uint8_t)y);
    _ILI_WRITE8((uint8_t)(y2 >> 8));
    _ILI_WRITE8((uint8_t)y2);

    _ili_write_command_8bit(ILI_RAMWR);
}


/**
 * Draw a bitmap image on the display
 * @param color_buffer Pointer to the 16-bit color color_buffer
 * @param len Number of pixels in buffer
 */
void ili_draw_pixels_buffer(uint16_t *color_buffer, uint32_t len)
{
    _ILI_DC_DATA();

#if defined(ILI_BUS_TYPE_SPI)
    ili_platform_spi_send_buffer16(color_buffer, len);

#elif defined(ILI_BUS_TYPE_PARALLEL8)
    uint32_t tmp_len = (len >> 2) << 2; // Getting closest len divisible by 4. [Same as: (uint32_t)(len / 4) * 4]
    for (uint32_t i = 0; i < tmp_len; i+=4)
    {
        // Unrolled loop to send burst of 4 pixels (64 bits) fast
        _ILI_WRITE8((uint8_t)(color_buffer[i+0]>>8)); _ILI_WRITE8((uint8_t)color_buffer[i+0]); //1
        _ILI_WRITE8((uint8_t)(color_buffer[i+1]>>8)); _ILI_WRITE8((uint8_t)color_buffer[i+1]); //2
        _ILI_WRITE8((uint8_t)(color_buffer[i+2]>>8)); _ILI_WRITE8((uint8_t)color_buffer[i+2]); //3
        _ILI_WRITE8((uint8_t)(color_buffer[i+3]>>8)); _ILI_WRITE8((uint8_t)color_buffer[i+3]); //4
    }
    tmp_len = (len & 3);    // len % 4
    for (uint32_t i = 0; i < tmp_len; i++)
    {
    	ILI_DISP_BUFFER_PIXELS_COUNT    _ILI_WRITE8((uint8_t)(color_buffer[i]>>8)); _ILI_WRITE8((uint8_t)color_buffer[i]); //1
    }
#endif


}


/**
 * Fills `len` number of pixels with `color`.
 * Call ili_set_address_window() before calling this function.
 * @param color 16-bit RGB565 color value
 * @param len 32-bit number of pixels
 */
void ili_fill_color(uint16_t color, uint32_t len)
{
    _ILI_DC_DATA();

#if defined(ILI_BUS_TYPE_SPI)
    uint32_t xfer_px_cnt = (len < ILI_TMP_DISP_BUF_PX_CNT) ? len : ILI_TMP_DISP_BUF_PX_CNT;

    for (uint32_t i = 0; i < xfer_px_cnt; i++)
	{
    	g_tmp_disp_buffer[i] = color;
	}
	while (len)
	{
		xfer_px_cnt = (len < xfer_px_cnt) ? len : xfer_px_cnt;
		ili_platform_spi_send_buffer16(g_tmp_disp_buffer, xfer_px_cnt);
		len -= xfer_px_cnt;
	}

#else /* when ILI_BUS_TYPE_PARALLEL8 */
	// [IMPORTANT]: The colorhigh == colorlow check is only applicable for parallel interface
	// If higher byte and lower byte are identical,
	// just strobe the WR pin to send the previous data
	uint8_t color_high = color >> 8;
	uint8_t color_low = color;
#ifdef _ILI_PLATFORM_WR_STROBE()
	if (color_high == color_low)
	{
        // Write only the first pixel
        _ILI_WRITE8(color_high); _ILI_WRITE8(color_low);
        --len;
		uint32_t tmp_len = (len >> 2) << 2; // Getting closest len divisible by 4. [Same as: (uint32_t)(len / 4) * 4]
        for (uint32_t i = 0; i < tmp_len; i+=4)
        {
            // Unrolled loop to send burst of 4 pixels (64 bytes) fast
            _ILI_WR_STROBE(); _ILI_WR_STROBE(); //1
            _ILI_WR_STROBE(); _ILI_WR_STROBE(); //2
            _ILI_WR_STROBE(); _ILI_WR_STROBE(); //3
            _ILI_WR_STROBE(); _ILI_WR_STROBE(); //4
        }
        tmp_len = (len & 3);    // len % 4
        for (uint32_t i = 0; i < tmp_len; i++)
        {
            _ILI_WR_STROBE(); _ILI_WR_STROBE(); //1
        }
	}

	// If higher and lower bytes are different, send those bytes
	else
	{
#endif /*_ILI_PLATFORM_WR_STROBE*/
		uint32_t tmp_len = (len >> 2) << 2; // Getting closest len divisible by 4. [Same as: (uint32_t)(len / 4) * 4]
        for (uint32_t i = 0; i < tmp_len; i+=4)
        {
            // Unrolled loop to send burst of 4 pixels (64 bytes) fast
            _ILI_WRITE8(color_high); _ILI_WRITE8(color_low); //1
            _ILI_WRITE8(color_high); _ILI_WRITE8(color_low); //2
            _ILI_WRITE8(color_high); _ILI_WRITE8(color_low); //3
            _ILI_WRITE8(color_high); _ILI_WRITE8(color_low); //4
        }
        tmp_len = (len & 3);    // len % 4
        for (uint32_t i = 0; i < tmp_len; i++)
        {
            _ILI_WRITE8((uint8_t)(color_buffer[i]>>8)); _ILI_WRITE8((uint8_t)color_buffer[i]); //1
        }
	}

#endif /*ILI_BUS_TYPE_SPI*/

}


/**
 * Fills a rectangular area with `color`.
 * Before filling, performs area bound checking
 * @param x Start col address
 * @param y Start row address
 * @param w Width of rectangle
 * @param h Height of rectangle
 * @param color 16-bit RGB565 color
 */
void ili_fill_rect(uint16_t x, uint16_t y, uint16_t w, uint16_t h, uint16_t color)
{
	if (x >= g_ili_tftwidth || y >= g_ili_tftheight || w == 0 || h == 0)
		return;
	if (x + w - 1 >= g_ili_tftwidth)
		w = g_ili_tftwidth - x;
	if (y + h - 1 >= g_ili_tftheight)
		h = g_ili_tftheight - y;

	ili_set_address_window(x, y, w, h);
	ili_fill_color(color, (uint32_t)w * (uint32_t)h);
}


/*
 * Same as `ili_fill_rect()` but does not do bound checking, so it's slightly faster
 */
void ili_fill_rect_fast(uint16_t x1, uint16_t y1, uint16_t w, uint16_t h, uint16_t color)
{
	ili_set_address_window(x1, y1, w, h);
	ili_fill_color(color, (uint32_t)w * (uint32_t)h);
}


/**
 * Fill the entire display (screen) with `color`
 * @param color 16-bit RGB565 color
 */
void ili_fill_screen(uint16_t color)
{
	ili_set_address_window(0, 0, g_ili_tftwidth, g_ili_tftheight);
	ili_fill_color(color, (uint32_t)g_ili_tftwidth * (uint32_t)g_ili_tftheight);
}


/**
 * Draw a rectangle
*/
void ili_draw_rectangle(uint16_t x, uint16_t y, uint16_t w, uint16_t h, uint16_t color)
{
	// Perform bound checking
	if (x >= g_ili_tftwidth || y >= g_ili_tftheight || w == 0 || h == 0)
		return;
	if (x + w - 1 >= g_ili_tftwidth)
		w = g_ili_tftwidth - x;
	if (y + h - 1 >= g_ili_tftheight)
		h = g_ili_tftheight - y;

	_ili_draw_fast_h_line(x, y, x+w-1, 1, color);
	_ili_draw_fast_h_line(x, y+h, x+w-1, 1, color);
	_ili_draw_fast_v_line(x, y, y+h-1, 1, color);
	_ili_draw_fast_v_line(x+w, y, y+h-1, 1, color);


}

/*
 * Called by ili_draw_line().
 * User need not call it
 */
void _ili_plot_line_low(uint16_t x0, uint16_t y0, uint16_t x1, uint16_t y1, uint8_t width, uint16_t color)
{
	int16_t dx = x1 - x0;
	int16_t dy = y1 - y0;
	int8_t yi = 1;
	uint8_t pixels_per_point = width * width;	//no of pixels making a point. if line width is 1, this var is 1. if 2, this var is 4 and so on
	uint8_t color_high = (uint8_t)(color >> 8);
	uint8_t color_low = (uint8_t)color;
	if (dy < 0)
	{
		yi = -1;
		dy = -dy;
	}

	int16_t D = 2*dy - dx;
	uint16_t y = y0;
	uint16_t x = x0;

	while (x <= x1)
	{
		ili_set_address_window(x, y, width, width);
		//Drawing all the pixels of a single point

		_ILI_DC_DATA();
		for (uint8_t pixel_cnt = 0; pixel_cnt < pixels_per_point; pixel_cnt++)
		{
			_ILI_WRITE8(color_high);
			_ILI_WRITE8(color_low);
		}

		if (D > 0)
		{
			y = y + yi;
			D = D - 2*dx;
		}
		D = D + 2*dy;
		x++;
	}
}


/*
 * Called by ili_draw_line().
 * User need not call it
 */
void _ili_plot_line_high(uint16_t x0, uint16_t y0, uint16_t x1, uint16_t y1, uint8_t width, uint16_t color)
{
	int16_t dx = x1 - x0;
	int16_t dy = y1 - y0;
	int8_t xi = 1;
	uint8_t pixels_per_point = width * width;	//no of pixels making a point. if line width is 1, this var is 1. if 2, this var is 4 and so on
	uint8_t color_high = (uint8_t)(color >> 8);
	uint8_t color_low = (uint8_t)color;

	if (dx < 0)
	{
		xi = -1;
		dx = -dx;
	}

	int16_t D = 2*dx - dy;
	uint16_t y = y0;
	uint16_t x = x0;

	while (y <= y1)
	{
		ili_set_address_window(x, y, width, width);
		//Drawing all the pixels of a single point

		_ILI_DC_DATA();
		for (uint8_t pixel_cnt = 0; pixel_cnt < pixels_per_point; pixel_cnt++)
		{
			_ILI_WRITE8(color_high);
			_ILI_WRITE8(color_low);
		}

		if (D > 0)
		{
			x = x + xi;
			D = D - 2*dy;
		}
		D = D + 2*dx;
		y++;
	}
}


/**
 * Draw a line from (x0,y0) to (x1,y1) with `width` and `color`.
 * @param x0 start column address.
 * @param y0 start row address.
 * @param x1 end column address.
 * @param y1 end row address.
 * @param width width or thickness of the line
 * @param color 16-bit RGB565 color of the line
 */
void ili_draw_line(uint16_t x0, uint16_t y0, uint16_t x1, uint16_t y1, uint8_t width, uint16_t color)
{
	/*
	* Brehensen's algorithm is used.
	* Not necessarily start points has to be less than end points.
	*/

	if (x0 == x1)	//vertical line
	{
		_ili_draw_fast_v_line(x0, y0, y1, width, color);
	}
	else if (y0 == y1)		//horizontal line
	{
		_ili_draw_fast_h_line(x0, y0, x1, width, color);
	}

	else
	{
		if (abs(y1 - y0) < abs(x1 - x0))
		{
			if (x0 > x1)
				_ili_plot_line_low(x1, y1, x0, y0, width, color);
			else
				_ili_plot_line_low(x0, y0, x1, y1, width, color);
		}

		else
		{
			if (y0 > y1)
				_ili_plot_line_high(x1, y1, x0, y0, width, color);
			else
				_ili_plot_line_high(x0, y0, x1, y1, width, color) ;
		}
	}

}


/*
 * Called by ili_draw_line().
 * User need not call it
 */
void _ili_draw_fast_h_line(uint16_t x0, uint16_t y0, uint16_t x1, uint8_t width, uint16_t color)
{
	/*
	* Draw a horizontal line very fast
	*/
	if (x0 < x1)
		ili_set_address_window(x0, y0, x1-x0+1, width);	//as it's horizontal line, y1=y0.. must be.
	else
		ili_set_address_window(x1, y0, x0-x1+1, width);

	ili_fill_color(color, (uint32_t)width * (uint32_t)abs(x1 - x0 + 1));
}


/*
 * Called by ili_draw_line().
 * User need not call it
 */
void _ili_draw_fast_v_line(uint16_t x0, uint16_t y0, uint16_t y1, uint8_t width, uint16_t color)
{
	/*
	* Draw a vertical line very fast
	*/
	if (y0 < y1)
		ili_set_address_window(x0, y0, width, y1-y0+1);	//as it's vertical line, x1=x0.. must be.
	else
		ili_set_address_window(x0, y1, width, y0-y1+1);

	ili_fill_color(color, (uint32_t)width * (uint32_t)abs(y1 - y0 + 1));
}


/**
 * Draw a pixel at a given position with `color`
 * @param x Start col address
 * @param y Start row address
 */
void ili_draw_pixel(uint16_t x, uint16_t y, uint16_t color)
{
	/*
	* Why?: This function is mainly added in the driver so that  ui libraries can use it.
	*/

	ili_set_address_window(x, y, 1, 1);
    _ILI_DC_DATA();
    _ILI_WRITE8((uint8_t)(color >> 8));
    _ILI_WRITE8((uint8_t)color);
}



/**
 * Rotate the display clockwise or anti-clockwie set by `rotation`
 * @param rotation Type of rotation. Supported values 0, 1, 2, 3
 */
void ili_rotate_display(uint8_t rotation)
{
    /*
    *(uint8_t)rotation: Rotation Type
    *         0 : Default landscape
    *         1 : Potrait 1
    *         2 : Landscape 2
    *         3 : Potrait 2
    */

//    uint16_t new_height = 240;
//    uint16_t new_width = 320;

	uint16_t new_height = 320;
	uint16_t new_width = 240;

    switch (rotation)
    {
        case 0:
            _ili_write_command_8bit(ILI_MADCTL);		//Memory Access Control
            _ili_write_data_8bit(0x40 | ILI_MAD_COLOR_ORDER);				//MX: 1, MY: 0, MV: 0	(Portrait 1. Default)
            g_ili_tftheight = new_height;
            g_ili_tftwidth = new_width;
            break;
        case 1:
            _ili_write_command_8bit(ILI_MADCTL);		//Memory Access Control
            _ili_write_data_8bit(0x20 | ILI_MAD_COLOR_ORDER);				//MX: 0, MY: 0, MV: 1	(Landscape 1)
            g_ili_tftheight = new_width;
            g_ili_tftwidth = new_height;
            break;
        case 2:
            _ili_write_command_8bit(ILI_MADCTL);		//Memory Access Control
            _ili_write_data_8bit(0x80 | ILI_MAD_COLOR_ORDER);				//MX: 0, MY: 1, MV: 0	(Portrait 2)
            g_ili_tftheight = new_height;
            g_ili_tftwidth = new_width;
            break;
        case 3:
            _ili_write_command_8bit(ILI_MADCTL);		//Memory Access Control
            _ili_write_data_8bit(0xE0 | ILI_MAD_COLOR_ORDER);				//MX: 1, MY: 1, MV: 1	(Landscape 2)
            g_ili_tftheight = new_width;
            g_ili_tftwidth = new_height;
            break;
    }
}

uint8_t ili_display_get_rotation()
{
	return g_rotation;
}

void ili_get_display_size(uint16_t *width, uint16_t *height, uint8_t *rotation)
{
	*rotation = g_rotation;
	*width = g_ili_tftwidth;
	*height = g_ili_tftheight;
}

/**
 * Initialize the display driver
 */
#define _INIT_MODE 1
void ili_init()
{
// Hardware reset is not mandatory if software rest is done
#if defined(ILI_PLATFORM_RST_LOW) && defined(ILI_PLATFORM_RST_HIGH)
    ILI_PLATFORM_RST_HIGH();
    ili_platform_delay(5);
    ILI_PLATFORM_RST_LOW();
    ili_platform_delay(10);
    ILI_PLATFORM_RST_HIGH();
#else
	_ili_write_command_8bit(ILI_SWRESET);
#endif
	ili_platform_delay(150);

#if _INIT_MODE == 1
	_ili_write_command_8bit(0xEF);
	_ili_write_data_8bit(0x03);
	_ili_write_data_8bit(0x80);
	_ili_write_data_8bit(0x02);

	_ili_write_command_8bit(0xCF);
	_ili_write_data_8bit(0x00);
	_ili_write_data_8bit(0XC1);
	_ili_write_data_8bit(0X30);

	_ili_write_command_8bit(0xED);
	_ili_write_data_8bit(0x64);
	_ili_write_data_8bit(0x03);
	_ili_write_data_8bit(0X12);
	_ili_write_data_8bit(0X81);

	_ili_write_command_8bit(0xE8);
	_ili_write_data_8bit(0x85);
	_ili_write_data_8bit(0x00);
	_ili_write_data_8bit(0x78);

	_ili_write_command_8bit(0xCB);
	_ili_write_data_8bit(0x39);
	_ili_write_data_8bit(0x2C);
	_ili_write_data_8bit(0x00);
	_ili_write_data_8bit(0x34);
	_ili_write_data_8bit(0x02);

	_ili_write_command_8bit(0xF7);
	_ili_write_data_8bit(0x20);

	_ili_write_command_8bit(0xEA);
	_ili_write_data_8bit(0x00);
	_ili_write_data_8bit(0x00);

	_ili_write_command_8bit(ILI_PWCTR1);    //Power control
	_ili_write_data_8bit(0x23);   //VRH[5:0]

	_ili_write_command_8bit(ILI_PWCTR2);    //Power control
	_ili_write_data_8bit(0x10);   //SAP[2:0];BT[3:0]

	_ili_write_command_8bit(ILI_VMCTR1);    //VCM control
	_ili_write_data_8bit(0x3e);
	_ili_write_data_8bit(0x28);

	_ili_write_command_8bit(ILI_VMCTR2);    //VCM control2
	_ili_write_data_8bit(0x86);  //--

	_ili_write_command_8bit(ILI_MADCTL);    // Memory Access Control
	_ili_write_data_8bit(0x40 | ILI_MAD_COLOR_ORDER); // Rotation 0 (portrait mode) //40 = RGB, 48 = BGR


	_ili_write_command_8bit(ILI_PIXFMT);
	_ili_write_data_8bit(0x55);

	_ili_write_command_8bit(ILI_FRMCTR1);
	_ili_write_data_8bit(0x00);
	_ili_write_data_8bit(0x13); // 0x18 79Hz, 0x1B default 70Hz, 0x13 100Hz

	_ili_write_command_8bit(ILI_DFUNCTR);    // Display Function Control
	_ili_write_data_8bit(0x08);
	_ili_write_data_8bit(0x82);
	_ili_write_data_8bit(0x27);

	_ili_write_command_8bit(0xF2);    // 3Gamma Function Disable
	_ili_write_data_8bit(0x00);

	_ili_write_command_8bit(ILI_GAMMASET);    //Gamma curve selected
	_ili_write_data_8bit(0x01);

	_ili_write_command_8bit(ILI_GMCTRP1);    //Set Gamma
	_ili_write_data_8bit(0x0F);
	_ili_write_data_8bit(0x31);
	_ili_write_data_8bit(0x2B);
	_ili_write_data_8bit(0x0C);
	_ili_write_data_8bit(0x0E);
	_ili_write_data_8bit(0x08);
	_ili_write_data_8bit(0x4E);
	_ili_write_data_8bit(0xF1);
	_ili_write_data_8bit(0x37);
	_ili_write_data_8bit(0x07);
	_ili_write_data_8bit(0x10);
	_ili_write_data_8bit(0x03);
	_ili_write_data_8bit(0x0E);
	_ili_write_data_8bit(0x09);
	_ili_write_data_8bit(0x00);

	_ili_write_command_8bit(ILI_GMCTRN1);    //Set Gamma
	_ili_write_data_8bit(0x00);
	_ili_write_data_8bit(0x0E);
	_ili_write_data_8bit(0x14);
	_ili_write_data_8bit(0x03);
	_ili_write_data_8bit(0x11);
	_ili_write_data_8bit(0x07);
	_ili_write_data_8bit(0x31);
	_ili_write_data_8bit(0xC1);
	_ili_write_data_8bit(0x48);
	_ili_write_data_8bit(0x08);
	_ili_write_data_8bit(0x0F);
	_ili_write_data_8bit(0x0C);
	_ili_write_data_8bit(0x31);
	_ili_write_data_8bit(0x36);
	_ili_write_data_8bit(0x0F);

	_ili_write_command_8bit(ILI_SLPOUT);    //Exit Sleep
	_ili_write_data_8bit(0x80);
	ili_platform_delay(120);

	_ili_write_command_8bit(ILI_DISPON);    //Display on
	_ili_write_data_8bit(0x80);
	ili_platform_delay(120);

#elif _INIT_MODE == 2
	_ili_write_command_8bit(0xCF);
	_ili_write_data_8bit(0x00);
	_ili_write_data_8bit(0XC1);
	_ili_write_data_8bit(0X30);

	_ili_write_command_8bit(0xED);
	_ili_write_data_8bit(0x64);
	_ili_write_data_8bit(0x03);
	_ili_write_data_8bit(0X12);
	_ili_write_data_8bit(0X81);

	_ili_write_command_8bit(0xE8);
	_ili_write_data_8bit(0x85);
	_ili_write_data_8bit(0x00);
	_ili_write_data_8bit(0x78);

	_ili_write_command_8bit(0xCB);
	_ili_write_data_8bit(0x39);
	_ili_write_data_8bit(0x2C);
	_ili_write_data_8bit(0x00);
	_ili_write_data_8bit(0x34);
	_ili_write_data_8bit(0x02);

	_ili_write_command_8bit(0xF7);
	_ili_write_data_8bit(0x20);

	_ili_write_command_8bit(0xEA);
	_ili_write_data_8bit(0x00);
	_ili_write_data_8bit(0x00);

	_ili_write_command_8bit(ILI_PWCTR1); //Power control
	_ili_write_data_8bit(0x10); //VRH[5:0]

	_ili_write_command_8bit(ILI_PWCTR2); //Power control
	_ili_write_data_8bit(0x00); //SAP[2:0];BT[3:0]

	_ili_write_command_8bit(ILI_VMCTR1); //VCM control
	_ili_write_data_8bit(0x30);
	_ili_write_data_8bit(0x30);

	_ili_write_command_8bit(ILI_VMCTR2); //VCM control2
	_ili_write_data_8bit(0xB7); //--

	_ili_write_command_8bit(ILI_PIXFMT);
	_ili_write_data_8bit(0x55);

	_ili_write_command_8bit(0x36); // Memory Access Control
	_ili_write_data_8bit(0x08); // Rotation 0 (portrait mode)

	_ili_write_command_8bit(ILI_FRMCTR1);
	_ili_write_data_8bit(0x00);
	_ili_write_data_8bit(0x1A);

	_ili_write_command_8bit(ILI_DFUNCTR); // Display Function Control
	_ili_write_data_8bit(0x08);
	_ili_write_data_8bit(0x82);
	_ili_write_data_8bit(0x27);

	_ili_write_command_8bit(0xF2); // 3Gamma Function Disable
	_ili_write_data_8bit(0x00);

	_ili_write_command_8bit(0x26); //Gamma curve selected
	_ili_write_data_8bit(0x01);

	_ili_write_command_8bit(0xE0); //Set Gamma
	_ili_write_data_8bit(0x0F);
	_ili_write_data_8bit(0x2A);
	_ili_write_data_8bit(0x28);
	_ili_write_data_8bit(0x08);
	_ili_write_data_8bit(0x0E);
	_ili_write_data_8bit(0x08);
	_ili_write_data_8bit(0x54);
	_ili_write_data_8bit(0xA9);
	_ili_write_data_8bit(0x43);
	_ili_write_data_8bit(0x0A);
	_ili_write_data_8bit(0x0F);
	_ili_write_data_8bit(0x00);
	_ili_write_data_8bit(0x00);
	_ili_write_data_8bit(0x00);
	_ili_write_data_8bit(0x00);

	_ili_write_command_8bit(0XE1); //Set Gamma
	_ili_write_data_8bit(0x00);
	_ili_write_data_8bit(0x15);
	_ili_write_data_8bit(0x17);
	_ili_write_data_8bit(0x07);
	_ili_write_data_8bit(0x11);
	_ili_write_data_8bit(0x06);
	_ili_write_data_8bit(0x2B);
	_ili_write_data_8bit(0x56);
	_ili_write_data_8bit(0x3C);
	_ili_write_data_8bit(0x05);
	_ili_write_data_8bit(0x10);
	_ili_write_data_8bit(0x0F);
	_ili_write_data_8bit(0x3F);
	_ili_write_data_8bit(0x3F);
	_ili_write_data_8bit(0x0F);

	_ili_write_command_8bit(0x2B);
	_ili_write_data_8bit(0x00);
	_ili_write_data_8bit(0x00);
	_ili_write_data_8bit(0x01);
	_ili_write_data_8bit(0x3f);

	_ili_write_command_8bit(0x2A);
	_ili_write_data_8bit(0x00);
	_ili_write_data_8bit(0x00);
	_ili_write_data_8bit(0x00);
	_ili_write_data_8bit(0xef);

	_ili_write_command_8bit(ILI_SLPOUT); //Exit Sleep
	ili_platform_delay(120);

	_ili_write_command_8bit(ILI_DISPON); //Display on
	ili_platform_delay(120);
#endif /* _INIT_MODE == 2 */


}
