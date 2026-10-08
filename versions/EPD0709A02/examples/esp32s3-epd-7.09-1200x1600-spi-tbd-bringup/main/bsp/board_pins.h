#pragma once

#include "driver/gpio.h"
#include "hal/spi_types.h"

/*
 * 7.09" Spectra 6 (1200x1600)，适配板 J2：
 *   1 GND  2 RST_N  3 BUSY_N  4 D_CX  5 CSB_M  6 CSB_S
 *   7 SCL  8 SI0    9 SI1    10 SI2  11 SI3   12 +3V3
 *
 * BS0/BS1 均 0Ω 接地 → 4-wire SPI，要 DC，数据只走 SI0。
 * 板上无 LOAD_SW；高压由 DDIC 在 PON 后自举（VDDP/VDDN/VGH/VGL/VCOM）。
 */
#define EPD_SPI_HOST            SPI2_HOST
#define EPD_SPI_FREQ_HZ         (5 * 1000 * 1000)

#define PIN_EPD_RST             (GPIO_NUM_47)   /* J2-2  RST_N */
#define PIN_EPD_BUSY            (GPIO_NUM_38)   /* J2-3  BUSY_N，高=空闲 */
#define PIN_EPD_DC              (GPIO_NUM_39)   /* J2-4  D_CX */
#define PIN_EPD_CS0             (GPIO_NUM_40)   /* J2-5  CSB_M */
#define PIN_EPD_CS1             (GPIO_NUM_41)   /* J2-6  CSB_S */
#define PIN_EPD_SCK             (GPIO_NUM_42)   /* J2-7  SCL */
#define PIN_EPD_D0              (GPIO_NUM_12)   /* J2-8  SI0 / MOSI */
#define PIN_EPD_D1              (GPIO_NUM_11)   /* J2-9  SI1，4 线不用 */
#define PIN_EPD_D2              (GPIO_NUM_10)   /* J2-10 SI2，4 线不用 */
#define PIN_EPD_D3              (GPIO_NUM_9)    /* J2-11 SI3，4 线不用 */

#define PIN_EPD_LOAD_SW         (GPIO_NUM_NC)

#define EPD_CS_MAIN             0
#define EPD_CS_SUB              1
