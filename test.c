#include "cyhal.h"
#include "cybsp.h"
#include "cy_retarget_io.h"
#include "ili9341.h"

/*******************************************************************************
* Macros
*******************************************************************************/
#define SYSTICK_MAX_CNT			(1 << 24) - 1		// max value for 24-bit reload register
#define BUF_ROWS	320
/*******************************************************************************
* Function Prototypes
*******************************************************************************/
static void isr_systick(void);
uint32_t get_micros(void);
uint32_t get_millis(void);
uint16_t generate_color(uint32_t x, uint32_t y, uint32_t w, uint32_t h);
static inline __attribute__((always_inline)) uint32_t get_ticks(void);
/*******************************************************************************
* Global Variables
*******************************************************************************/
volatile uint32_t systick_wrap = 0;	// SysTick wraps every 1s (NOTE: uint64_t not supported!!)
cyhal_spi_t mSPI;
uint16_t disp_buf[BUF_ROWS*240] = {0};

/*******************************************************************************
* Function Name: main
*******************************************************************************/
int main(void)
{
    cy_rslt_t result;

    /* Initialize the device and board peripherals */
    result = cybsp_init();

    /* Enable global interrupts */
    __enable_irq();

    /* Initialize retarget-io to use the debug UART port */
    cy_retarget_io_init(CYBSP_DEBUG_UART_TX, CYBSP_DEBUG_UART_RX, CY_RETARGET_IO_BAUDRATE);

    /* Initialize the User LED */
    cyhal_gpio_init(CYBSP_USER_LED, CYHAL_GPIO_DIR_OUTPUT, CYHAL_GPIO_DRIVE_STRONG, CYBSP_LED_STATE_OFF);

    /* Initialize lcd control pins */
	Cy_GPIO_Pin_FastInit(DISP_CTL_PORT, DISP_CS_NUM, CY_GPIO_DM_STRONG_IN_OFF, 1, P5_7_GPIO);
	Cy_GPIO_Pin_FastInit(DISP_CTL_PORT, DISP_DC_NUM, CY_GPIO_DM_STRONG_IN_OFF, 1, P5_4_GPIO);
#ifdef DISP_RST_NUM
	Cy_GPIO_Pin_FastInit(DISP_CTL_PORT, DISP_RST_NUM, CY_GPIO_DM_STRONG_IN_OFF, 1, P5_5_GPIO);
#endif

    /* \x1b[2J\x1b[;H - ANSI ESC sequence for clear screen */
    printf("\x1b[2J\x1b[;H");

    printf("****************** "
           "ILI9341 Test! "
           "****************** \r\n\n");


    // CLK_TIMER set to 1MHz using device configurator
    // Interrupt fires after systick counter reached the end
    // Note: Device Configurator shows wrong freq for CLK_TIMER.
    // Actual freq of CLK_TIMER = Source Clk / Divider
    Cy_SysTick_Init(CY_SYSTICK_CLOCK_SOURCE_CLK_TIMER, SYSTICK_MAX_CNT);
    Cy_SysTick_SetCallback(0, isr_systick);


    ili_bus_init();
    cyhal_system_delay_ms(10);
    ili_init();
	ili_fill_screen(0xff80);

	uint16_t w = 0, h = 0;
	uint8_t r = 0;
	ili_get_display_size(&w, &h, &r);

	uint32_t frames = 200;
	uint32_t i_max = frames/10;
	uint32_t len = w * h;


	// Loop to fill the display buffer with color gradient
	for (uint32_t y = 0; y < h; y++)
	{
		for (uint32_t x = 0; x < w; x++)
		{
			// Calculate RGB565 color based on position
			uint16_t color = generate_color(x, y, (uint32_t)w, (uint32_t)h);
			// Set the color in the display buffer
			disp_buf[y * w + x] = color;
		}
	}

	/* Sets the area where pixels will be drawn */
	ili_set_address_window(0, 0, w, h);

	cyhal_gpio_write(CYBSP_USER_LED, CYBSP_LED_STATE_ON);
	uint32_t start = get_millis();

	// Drawing 200 frames. Loop is unrolled for faster execution
    for (int i = 0; i < i_max; ++i)
    {
    	/* Now passing the buffer. Area is already set*/

    	ili_draw_pixels_buffer(disp_buf, len);
    	ili_draw_pixels_buffer(disp_buf, len);
    	ili_draw_pixels_buffer(disp_buf, len);
    	ili_draw_pixels_buffer(disp_buf, len);
    	ili_draw_pixels_buffer(disp_buf, len);

    	ili_draw_pixels_buffer(disp_buf, len);
		ili_draw_pixels_buffer(disp_buf, len);
		ili_draw_pixels_buffer(disp_buf, len);
		ili_draw_pixels_buffer(disp_buf, len);
		ili_draw_pixels_buffer(disp_buf, len);
    }

    uint32_t end = get_millis();
    cyhal_gpio_write(CYBSP_USER_LED, CYBSP_LED_STATE_OFF);
    uint32_t delta = end - start;

    float fps = ((float)frames / (float)delta) * 1000.;

    printf("\r\nFrames   : %ld", frames);
    printf("\r\nDelta(ms): %ld", delta);
    printf("\r\nFPS      : %f", fps);
    printf("\r\n");
}

// Function to generate RGB565 color based on position
uint16_t generate_color(uint32_t x, uint32_t y, uint32_t w, uint32_t h)
{
    // Calculate RGB components based on position
    uint32_t red = (x * 31) / (w - 1);
    uint32_t green = ((x + y) * 63) / (w + h - 2);
    uint32_t blue = (y * 31) / (h - 1);

    // Combine RGB components into RGB565 format
    return (red << 11) | (green << 5) | blue;
}

// See: https://community.infineon.com/t5/PSoC-6/SysTick-for-timing-measurement/m-p/347523#M12764
static inline __attribute__((always_inline)) uint32_t get_ticks(void)
{
	// SYSTICK_MAX_CNT - SYSTICK_VAL --> Since SysTick is down counting
	// systick_wrap * (1 + SYSTICK_MAX_CNT) ---> Number of previous ticks
	return ((SYSTICK_MAX_CNT - SYSTICK_VAL) + (systick_wrap * (1 + SYSTICK_MAX_CNT)));
}

uint32_t get_micros(void)
{
	// As systick clock=1MHz, 1 tick per per microsecond.
	return get_ticks();
}

uint32_t get_millis(void)
{
	// As systick clock=1MHz, 1000 ticks per per millisecond.
	return (get_ticks()/1000);
}

void isr_systick(void)
{
    ++systick_wrap;
}

/* [] END OF FILE */
