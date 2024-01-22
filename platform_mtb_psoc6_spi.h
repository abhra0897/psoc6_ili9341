// NOTE: Only to be included by platform_select.h. User should NOT include it
// ModusToolbox PSoC6 platform

#ifndef _PLATFORM_MTB_PSOC6_SPI_
#define _PLATFORM_MTB_PSOC6_SPI_

#include <stdint.h>
#include <string.h>
#include "cybsp.h"
#include "cyhal.h"

/* Using SCB6 which has SPI3 because we want to use Arduino headers */
/* If user needs to use different SPI, find which SCB to use using device configurator */
#define DISP_SPI_SCB         SCB6

#define DISP_SPI_PORT        P12_0_PORT
#define DISP_SPI_MISO_NUM    P12_1_NUM	/* D12 */
#define DISP_SPI_MOSI_NUM    P12_0_NUM	/* D11 */
#define DISP_SPI_SCLK_NUM    P12_2_NUM	/* D13 */

#define DISP_CTL_PORT        P5_0_PORT
#define DISP_CS_NUM          P5_7_NUM	/* D7 */
#define DISP_DC_NUM          P5_4_NUM	/* D4 */
/*Optional*/
//#define DISP_RST_NUM         P5_5_NUM	/* D5 */
/* ------------------------------------- */
	/* Not needed for SPI. Only for parallel */
	//#define DISP_WR_NUM          P5_3_NUM	/* D3 */
	//#define DISP_RD_NUM          P5_2_NUM	/* D2 */
/* ------------------------------------- */


/* ====================================================== */
/*      Mandatory Config Macros needed by ili9341.c/h     */
/* ====================================================== */
#define ILI_BUS_TYPE_SPI
// #define ILI_BUS_TYPE_PARALLEL8

/* ============[ End: Mandatory Config Macros]=========== */


/* ====================================================== */
/*  Optional Config Macros (default values in ili9341.h)  */
/* ====================================================== */
#define ILI_SPI_FREQ    40000000UL    /* 40MHz  (Min: 10, Max: 50) */

/* ============[ End: Optional Config Macros]============ */


/* ====================================================== */
/*        Mandatory Macros needed by ili9341.c/h          */
/* ====================================================== */
#define ILI_PLATFORM_DC_HIGH()    {GPIO_PRT_OUT_SET(DISP_CTL_PORT) = (1 << DISP_DC_NUM);}
#define ILI_PLATFORM_DC_LOW()     {GPIO_PRT_OUT_CLR(DISP_CTL_PORT) = (1 << DISP_DC_NUM);}
/* ===============[ End: Mandatory Macros]=============== */


/* ====================================================== */
/*         Optional Macros needed by ili9341.c/h          */
/* ====================================================== */
#ifdef DISP_CS_NUM
	#define ILI_PLATFORM_CS_HIGH()    {GPIO_PRT_OUT_SET(DISP_CTL_PORT) = (1 << DISP_CS_NUM);}
	#define ILI_PLATFORM_CS_LOW()     {GPIO_PRT_OUT_CLR(DISP_CTL_PORT) = (1 << DISP_CS_NUM);}
#endif /*DISP_CS_NUM*/
#ifdef DISP_RST_NUM
	#define ILI_PLATFORM_RST_HIGH()   {GPIO_PRT_OUT_SET(DISP_CTL_PORT) = (1 << DISP_RST_NUM);}
	#define ILI_PLATFORM_RST_LOW()    {GPIO_PRT_OUT_CLR(DISP_CTL_PORT) = (1 << DISP_RST_NUM);}
#endif /*DISP_RST_NUM*/
/* ------------------------------------- */
	/* Not needed for SPI. Only for parallel */
	// #ifdef DISP_WR_NUM
	// 	#define ILI_PLATFORM_WR_HIGH()    {Cy_GPIO_Write(DISP_CTL_PORT, DISP_WR_NUM, 1);}
	// 	#define ILI_PLATFORM_WR_LOW()     {Cy_GPIO_Write(DISP_CTL_PORT, DISP_WR_NUM, 0);}
	// #endif /*DISP_WR_NUM*/
	// #ifdef DISP_RD_NUM
	// 	#define ILI_PLATFORM_RD_HIGH()    {Cy_GPIO_Write(DISP_CTL_PORT, DISP_RD_NUM, 1);}
	// 	#define ILI_PLATFORM_RD_LOW()     {Cy_GPIO_Write(DISP_CTL_PORT, DISP_RD_NUM, 0);}
	// #endif /*DISP_RD_NUM*/
/* ------------------------------------- */
/* ================[ End: Optional Macros]=============== */


/* ====================================================== */
/*        Mandatory functions needed by ili9341.c/h       */
/* ====================================================== */
void ili_platform_spi_init(uint64_t spi_freq, uint8_t cpol, uint8_t cpha, uint8_t is_lsbfirst);
void ili_platform_spi_deinit(void);
void ili_platform_spi_send8(uint8_t byte);
void ili_platform_spi_send_buffer16(uint16_t *buf, uint32_t items_count);
void ili_platform_delay(uint64_t ms);
/* ------------------------------------- */
	/* Not needed for SPI. Only for parallel */
	//void ili_platform_parallel_init(void);
	//void ili_platform_parallel_deinit(void);
	//void ili_platform_parallel_send8(uint8_t byte);
/* ------------------------------------- */
/* ==============[ End: Mandatory functions]============= */

#endif /*_PLATFORM_MTB_PSOC6_SPI_*/
