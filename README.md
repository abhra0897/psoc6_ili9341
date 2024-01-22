# PSoC6-ILI9341-Driver

## ILI9341 Parallel & SPI Display Driver for Infineon/Cypress PSoC6
This is a (fast?) display driver for interfacing ILI9341 LCD display with PSoC6 microcontroller over an 8bit parallel (8080-II/I) and serial (SPI) bus. It's mainly written for my personal usage. This driver is written using [ModusToolbox SDK](https://www.infineon.com/cms/en/design-support/tools/sdk/modustoolbox-software/)

This driver utilises Peripheral Driver Library or PDL for faster IO access. Using the HAL library also works, but that might be a little bit slower. I tried to keep a moderate balance between portability and speed.

All the hardware-specific operations such as GPIO, SPI, delay etc are in separate platform-specific files. So, just by providing a few function implementations and a few macro definitions, the library can be ported to other hardwares. See [Porting](#porting) section.

- For lightweight GUI, see: [LameUI](https://github.com/abhra0897/LameUI)
- For XPT2046 based touch input, see: [xpt2046-multiplatform](https://github.com/abhra0897/xpt2046-multiplatform)

### Files
| File                                                   | Details                                                                                                                                                                                        |
|--------------------------------------------------------|------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------|
| [ili9341.h](./ili9341.h)                               | Core library header, platform-independent. To be included by user application. It includes **platform-specific header** at the top.                                                            |
| [ili9341.c](./ili9341.c)                               | Core library source. No need to modify it                                                                                                                                                      |
| [platform_mtb_psoc6_spi.h](./platform_mtb_psoc6_spi.h) | **Platform-specific header** for PSoC6 to use SPI bus. To be included by  `ili9341.h` only. It provides Macros and functions that are needed by core lib. Configure the macros here as needed. |
| [platform_mtb_psoc6_spi.c](./platform_mtb_psoc6_spi.c) | Platform-specific source for PSoC6 to use SPI bus.                                                                                                                                             |
| platform_mtb_psoc6_parallel.h                          | [TO BE IMPLEMENTED] **Platform-specific header** for PSoC6 to use Parallel bus.                                                                                                                |
| platform_mtb_psoc6_parallel.c                          | [TO BE IMPLEMENTED]                                                                                                                                                                            |

### Wiring (SPI)
Tested using CY8CKIT-062S2-43012 devkit, featuring a  PSoC™ 6 CY8C62xA MCU.

Connections between CY8CKIT-062S2-43012 and ILI9341 display. I used SPI3 (SCB6) of the PSoC6, user may change SPI and respective pins as needed.

| ILI9341   | CY8CKIT-062S2-43012 | Comment                                           |
|-----------|---------------------|---------------------------------------------------|
| SDI(MOSI) | P12_0/D11           | SPI data out from mcu                             |
| SDO(MISO) | (~P12_1/D12~)       | Not used by driver                                |
| SCK       | P12_2/D13           | SPI clock                                         |
| DC        | P5_4/D4             | Data/command select                               |
| RST       | P5_5/D5             | (optional. Keep floating if unused) Display reset |
| CS        | P5_7/D7             | (optional. Connect to GND if unused) Chip select  |
| LED       | V3.3                | Display backlight pin                             |
| VCC       | V3.3                | VCC                                               |
| GND       | GND                 | GND                                               |



### Configuration
All configuration options are in the `platform_mtb_psoc6_spi.h` file.

- `#define ILI_BUS_TYPE_SPI` to select SPI bus. `#define ILI_BUS_TYPE_PARALLEL` to use parallel bus.
- `#define ILI_SPI_FREQ  40000000UL` to set SPI frequency to 40MHz

### Porting
To port this driver to other platforms, user needs to provide some macros and functions that are needed by `ili9341.c/h` files. The required platform-specific functions and macros are in [`platform_mtb_psoc6_spi.h`](./platform_mtb_psoc6_spi.h) (for SPI) and in `platform_mtb_psoc6_parallel.h` (for parallel bus. TBD).

A table of required macros and functions to be provided by user is given below. Some of the macros/functions are optional, some of them are only applicable in one bus type and not in the other.

| Name                                                                                             | Details                                                     | Mandatory? | Applicable Bus |
|--------------------------------------------------------------------------------------------------|-------------------------------------------------------------|------------|----------------|
| `#define ILI_BUS_TYPE_SPI`                                                                       | Sets the bus type to SPI                                    | Yes        | SPI            |
| `#define ILI_BUS_TYPE_PARALLEL8`                                                                 | Sets the bus type to 8-bit parallel                         | Yes        | Parallel       |
| `#define ILI_SPI_FREQ 40000000UL`                                                                | Sets SPI frequency, Default value is in `ili9341.h`         | No         | SPI, Parallel  |
| `#define ILI_PLATFORM_DC_HIGH()` <br>`#define ILI_PLATFORM_DC_LOW()`                             | Sets DC pin high or low                                     | Yes        | SPI, Parallel  |
| `#define ILI_PLATFORM_CS_HIGH()` <br>`#define ILI_PLATFORM_CS_LOW()`                             | Sets DC pin high or low                                     | No         | SPI, Parallel  |
| `#define ILI_PLATFORM_RST_HIGH()` <br>`#define ILI_PLATFORM_RST_LOW()`                           | Sets RST pin high or low                                    | No         | SPI, Parallel  |
| `#define ILI_PLATFORM_WR_HIGH()` <br>`#define ILI_PLATFORM_WR_LOW()`                             | Sets WR pin high or low                                     | No         | Parallel       |
| `#define ILI_PLATFORM_RD_HIGH()` <br>`#define ILI_PLATFORM_RD_LOW()`                             | Sets RD pin high or low                                     | No         | Parallel       |
| `void ili_platform_spi_init(uint64_t spi_freq, uint8_t cpol, uint8_t cpha, uint8_t is_lsbfirst)` | Initialize the SPI bus along with DC, RST, CS pins          | Yes        | SPI            |
| `void ili_platform_spi_deinit(void)`                                                             | De-init the spi bus                                         | Yes        | SPI            |
| `void ili_platform_spi_send8(uint8_t byte)`                                                      | Send a byte (8 bits) using SPI                              | Yes        | SPI            |
| `void ili_platform_spi_send_buffer16(uint16_t *buf, uint32_t items_count);`                      | Send a buffer of type `uint16_t` using SPI                  | Yes        | SPI            |
| `void ili_platform_delay(uint64_t ms)`                                                           | Delay specified milliseconds                                | Yes        | SPI, Parallel  |
| `void ili_platform_parallel_init(void)`                                                          | initialize parallel bus data pins, DC, CS, RST, WR, RD pins | Yes        | Parallel       |
| `void ili_platform_parallel_deinit(void)`                                                        | De-init the parallel bus                                    | Yes        | Parallel       |
| `void ili_platform_parallel_send8(uint8_t byte)`                                                 | Send a byte (8 bits) using parallel bus                     | Yes        | Parallel       |

### Benchmarks
- **SPI**:
    - FPS Test at 40MHz, `ili_draw_pixels_buffer()`, pre-filled buffer of 240*320:
        - Frames   : 200
        - Delta(ms): 6161
        - FPS      : 32.462261

    - FPS Test at 40MHz, `ili_fill_color()`, ILI_TMP_DISP_BUF_PX_CNT=256:
        - Frames   : 200
        - Delta(ms): 6260
        - FPS      : 31.948883
- **Parallel 8-bit**: Not implemented yet


### Example
See the example code [test.c](test.c). Compile using ModusToolbox IDE. This code requires `CLK_TIMER` to be set to 1MHz using Device Configurator of ModusToolbox. Simply set Timer Divider of CLK_TIMER to 8.

Depending on the version of Device Configurator, there might be a bug that'll cause the resultant frequency to be displayed as 125KHz instead of 1MHz which is the actual frequency. See this [forum post](https://community.infineon.com/t5/ModusToolbox/Device-Configurator-Shows-Wrong-Frequency-of-CLK-TIMER-for-PSoC6/m-p/654980#M6799).


### API

```C

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

```
### TO DO

 - [ ] Add more example code
 - [ ] Add Parallel bus support
 - [ ] Add DMA support

### License
All source codes of the root directory and example directory are licensed under MIT License, unless the source file has no other license asigned for it. See [MIT License](LICENSE).
