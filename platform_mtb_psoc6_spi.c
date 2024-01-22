#include <platform_mtb_psoc6_spi.h>
#include <stdio.h>

// TODO:
// 1. Make spi_send functions inline?
// 2. Add spi_send16 function? Will make line and pixel drawing faster

/* CLK_PERI is set at 50MHz. Can be changed in the device configurator */
#define _CLK_PERI  50000000UL

/* Read Cy_SCB_SPI_Init() in cy_scb_spi.h file to understand this */
/* Width should be 4, 8, or 16 */
/* NOTE: is_bytemode is 0 or 1. Any other value will mess up SPI settings*/
#define _SPI_SET_TX_WIDTH(width, is_bytemode) \
	/*Disable SPI*/ \
	SCB_CTRL(DISP_SPI_SCB) &= (uint32_t) ~SCB_CTRL_ENABLED_Msk; \
	/* Enable or disable bytemode */ \
	SCB_CTRL(DISP_SPI_SCB) =(SCB_CTRL(DISP_SPI_SCB) & (uint32_t) ~SCB_CTRL_BYTE_MODE_Msk) | (is_bytemode << SCB_CTRL_BYTE_MODE_Pos); \
	/*Changing to 8-bit width from 16*/ \
	SCB_RX_CTRL(DISP_SPI_SCB) = (SCB_RX_CTRL(DISP_SPI_SCB) & (uint32_t) ~0xFUL) | _VAL2FLD(SCB_RX_CTRL_DATA_WIDTH, ((width) - 1UL));\
	/*Changing to 8-bit width from 16*/ \
	SCB_TX_CTRL(DISP_SPI_SCB) = (SCB_TX_CTRL(DISP_SPI_SCB) & (uint32_t) ~0xFUL) | _VAL2FLD(SCB_TX_CTRL_DATA_WIDTH, ((width) - 1UL)); \
	/*Enable SPI*/ \
	SCB_CTRL(DISP_SPI_SCB) |= SCB_CTRL_ENABLED_Msk; \


static cy_stc_scb_spi_config_t g_spi_config;


// Only called if SPI_IS_SHARED is defined
void ili_platform_spi_init(uint64_t spi_freq, uint8_t cpol, uint8_t cpha, uint8_t is_lsbfirst)
{
	/**
	 * We can do the init using HAL too instead of PDL, but HAL sets oversample to minimum 4,
	 * As result, max achievable SPI freq is 25MHz at 100MHz clk_peri.
	 * If we use PDL to init, we can set oversample to 2 given that MISO is not used.
	 * Also, we can fine tune the clock divider using PDL better. Thats's why we used PDL to
	 * init the SCB to SPI mode
	 */
//	// --------- Init using HAL ---------
//	cyhal_spi_init(&mSPI, SPI_MOSI, SPI_MISO, SPI_CLK, NC, NULL,
//			8, CYHAL_SPI_MODE(cpol, cpha, is_lsbfirst), false);
//	cyhal_spi_set_frequency(&mSPI, spi_freq);


    // --------- Init using PDL ---------
    (void)spi_freq; // unused for now, using fixed 25MHz instead

	g_spi_config.spiMode  = CY_SCB_SPI_MASTER;
	g_spi_config.subMode  = CY_SCB_SPI_MOTOROLA;
	g_spi_config.sclkMode = (cpol << 1) | cpha /*CY_SCB_SPI_CPHA0_CPOL0*/;
	g_spi_config.oversample = 2UL; /* If using MISO; minimum is 4; else min is 2 */
	g_spi_config.rxDataWidth              = 8UL;
	g_spi_config.txDataWidth              = 8UL;
	g_spi_config.enableMsbFirst           = !is_lsbfirst;
	g_spi_config.enableInputFilter        = false;
	g_spi_config.enableFreeRunSclk        = false;
	g_spi_config.enableMisoLateSample     = false;
	g_spi_config.enableTransferSeperation = false;
	g_spi_config.ssPolarity               = CY_SCB_SPI_ACTIVE_LOW;
	g_spi_config.enableWakeFromSleep      = false;
	g_spi_config.rxFifoTriggerLevel  = 0UL;
	g_spi_config.rxFifoIntEnableMask = 0UL;
	g_spi_config.txFifoTriggerLevel  = 0UL;
	g_spi_config.txFifoIntEnableMask = 0UL;
	g_spi_config.masterSlaveIntEnableMask = 0UL;

    Cy_SCB_SPI_Init(DISP_SPI_SCB, &g_spi_config, NULL);

    /* Configure SCB6 pins for SPI Master operation */
    /* Connect SCB6 SPI function to pins */
    Cy_GPIO_Pin_FastInit(DISP_SPI_PORT, DISP_SPI_MISO_NUM, CY_GPIO_DM_HIGHZ, 0, P12_1_SCB6_SPI_MISO);
    Cy_GPIO_Pin_FastInit(DISP_SPI_PORT, DISP_SPI_MOSI_NUM, CY_GPIO_DM_STRONG_IN_OFF, 0, P12_0_SCB6_SPI_MOSI);
    Cy_GPIO_Pin_FastInit(DISP_SPI_PORT, DISP_SPI_SCLK_NUM, CY_GPIO_DM_STRONG_IN_OFF, 0, P12_2_SCB6_SPI_CLK);

    /* Assign divider type and number for SPI */
    cy_en_divider_types_t spi_clk_div_type = CY_SYSCLK_DIV_16_5_BIT;
    uint32_t              spi_clk_div_num  = 0;
    /* Connect assigned divider to be a clock source for SPI */
    Cy_SysClk_PeriphAssignDivider(PCLK_SCB6_CLOCK, spi_clk_div_type, spi_clk_div_num);

    /* The SPI master data rate = ((clk_peri/ divider) / Oversample)).
    * For clk_peri = 100 MHz, select divider value 1 and get SCB clock = (100 MHz / 1) = 100 MHz.
    * Select Oversample = 2. These setting results SPI data rate = 100 MHz / 2 = 50 MHz.
    */
	/* Fractional divider has 5 bits for fraction. Example: .25 = 8/32. So, we put 8 in the register
	* Int divider value should be actual value - 1.
	*/
    float div = (float)_CLK_PERI / (float)spi_freq;
    uint32_t div_int = div;
    uint32_t div_frac = (div - div_int) * 32;
    Cy_SysClk_PeriphSetFracDivider(spi_clk_div_type, spi_clk_div_num, div_int-1, div_frac);
    Cy_SysClk_PeriphEnableDivider(spi_clk_div_type, spi_clk_div_num);

    /* Enable SPI to operate */
    Cy_SCB_SPI_Enable(DISP_SPI_SCB);
}

void ili_platform_spi_deinit(void)
{
//	cyhal_spi_free(&mSPI);

    Cy_SCB_SPI_Disable(DISP_SPI_SCB, NULL);
    Cy_SCB_SPI_DeInit(DISP_SPI_SCB);

    /* Making pins normal GPIO else they're not pulling low when SPI is stopped */
	Cy_GPIO_Pin_FastInit(DISP_SPI_PORT, DISP_SPI_MOSI_NUM, CY_GPIO_DM_STRONG_IN_OFF, 0, P12_0_GPIO);
    Cy_GPIO_Pin_FastInit(DISP_SPI_PORT, DISP_SPI_MISO_NUM, CY_GPIO_DM_HIGHZ, 0, P12_1_GPIO);
	Cy_GPIO_Pin_FastInit(DISP_SPI_PORT, DISP_SPI_SCLK_NUM, CY_GPIO_DM_STRONG_IN_OFF, 0, P12_2_GPIO);
}


void ili_platform_spi_send8(uint8_t data)
{
	/*If Tx width is 16, set it to 16 before proceeding*/
	if ((SCB_TX_CTRL(DISP_SPI_SCB) & 0xFUL) == 16UL - 1UL)
	{
		_SPI_SET_TX_WIDTH(8, 1); // Width: 8, bytemode: Yes
	}
	SCB_TX_FIFO_WR(DISP_SPI_SCB) = data;
	while(!Cy_SCB_SPI_IsTxComplete(DISP_SPI_SCB));
}


void ili_platform_spi_send_buffer16(uint16_t *buf, uint32_t items_count)
{
	/*If Tx width is 8, set it to 16 before proceeding*/
	if ((SCB_TX_CTRL(DISP_SPI_SCB) & 0xFUL) == 8UL- 1UL)
	{
		_SPI_SET_TX_WIDTH(16, 0); // Width: 16, bytemode: No
	}
    Cy_SCB_SPI_WriteArrayBlocking(DISP_SPI_SCB, (void *)buf, items_count);
    while(!Cy_SCB_SPI_IsTxComplete(DISP_SPI_SCB));
}

void ili_platform_delay(uint64_t ms)
{
    cyhal_system_delay_ms(ms);
}
