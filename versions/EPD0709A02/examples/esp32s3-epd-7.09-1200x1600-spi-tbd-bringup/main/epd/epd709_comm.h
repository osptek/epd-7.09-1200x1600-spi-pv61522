#pragma once

#include <stdint.h>

#define GPIO_LOW    0
#define GPIO_HIGH   1

#define EPD_SPI_CHUNK_SIZE  32768

uint8_t initialSpi(void);
void initialGpioDefault(void);
void initialGpio(void);
void delayms(unsigned int delay_time);

uint8_t spiTransmitCommand(uint8_t command);
uint8_t spiTransmitData(uint8_t *data, unsigned long length);
uint8_t spiTransmitLargeData(uint8_t command, uint8_t *data, unsigned long length);
uint8_t spiTransmit(uint8_t command, uint8_t *data, unsigned int length);
uint8_t spiReceive(uint8_t command, uint8_t *data, unsigned int length);
uint8_t spiReceiveData(uint8_t *data, unsigned int length);

void setGpioLevel(uint8_t pin, uint8_t level);
uint8_t getGpioLevel(uint8_t pin);
void diagnoseBusyPin(void);

uint8_t *epdChunkBuffer(void);
