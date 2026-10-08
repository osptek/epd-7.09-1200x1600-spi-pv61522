#pragma once

#include <stdint.h>

#define BLACK           0x00
#define WHITE           0x11
#define YELLOW          0x22
#define RED             0x33
#define BLUE            0x55
#define GREEN           0x66

#define PSR             0x00
#define PWR             0x01
#define POF             0x02
#define POFS            0x03
#define PON             0x04
#define BTST_N          0x05
#define BTST_P          0x06
#define DTM             0x10
#define DRF             0x12
#define PLL             0x30
#define TSC             0x40
#define CDI             0x50
#define TCON            0x60
#define TRES            0x61
#define PTLW            0x83
#define AN_TM           0x74
#define AGID            0x86
#define CMDA4           0xA4
#define DCDC            0xA5
#define BUCK_BOOST_VDDN 0xB0
#define TFT_VCOM_POWER  0xB1
#define EN_BUF          0xB6
#define BOOST_VDDP_EN   0xB7
#define CCSET           0xE0
#define PWS             0xE3
#define CMD66           0xF0

#define PTLW_ENABLE     0x01
#define PTLW_DISABLE    0x00

/* 单颗 IC 的帧缓冲大小：800 x 1200 / 2 */
#define EPD_IMAGE_BYTES_PER_IC  480000
#define EPD_COLOR_BAR_BYTES     80000

void epdHardwareReset(void);
void setPinCsAll(unsigned int level);
void setPinCs(uint8_t cs_index, unsigned int level);
uint8_t checkBusyHigh(const char *stage, uint32_t timeout_ms);
void checkBusyLow(void);
uint8_t initEPD(void);
void writeEpd(uint8_t command, uint8_t *data, unsigned int length);
void readEpd(uint8_t command, uint8_t *data, unsigned int length);
void writeEpdCommand(uint8_t command);
void writeEpdData(uint8_t *data, unsigned int length);
uint8_t epdDisplay(void);
void epdDisplayColor(uint8_t color);
void writeEpdImage(uint8_t cs_index, const uint8_t *image, unsigned long length);
uint8_t epdDisplayColorBar(void);
uint8_t checkDriverICStatus(void);
void executeTscCommand(void);
char partialWindowUpdateWithImageData(uint8_t cs_index, const uint8_t *image, unsigned long length,
                                      unsigned int x_start, unsigned int y_start,
                                      unsigned int x_pixel, unsigned int y_line,
                                      uint8_t display_enable);
char partialWindowUpdateWithoutImageData(uint8_t cs_index, unsigned int x_start, unsigned int y_start,
                                         unsigned int x_pixel, unsigned int y_line,
                                         uint8_t display_enable);
