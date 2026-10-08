#include <string.h>

#include "driver/gpio.h"
#include "driver/spi_master.h"
#include "esp_heap_caps.h"
#include "esp_log.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

#include "board_pins.h"
#include "epd709_comm.h"
#include "status.h"

static const char *TAG = "epd709_comm";

#define SPI_MAX_BUFFER_SIZE  32768

static spi_device_handle_t spi;
static uint8_t *s_chunk;

static void gpio_float(uint64_t pin_mask)
{
    gpio_config_t io = {
        .pin_bit_mask = pin_mask,
        .mode = GPIO_MODE_INPUT,
        .pull_up_en = GPIO_PULLUP_DISABLE,
        .pull_down_en = GPIO_PULLDOWN_DISABLE,
        .intr_type = GPIO_INTR_DISABLE,
    };
    ESP_ERROR_CHECK(gpio_config(&io));
}

uint8_t *epdChunkBuffer(void)
{
    return s_chunk;
}

void delayms(unsigned int delay_time)
{
    vTaskDelay(pdMS_TO_TICKS(delay_time));
}

void setGpioLevel(uint8_t pin, uint8_t level)
{
    gpio_set_level(pin, level);
}

uint8_t getGpioLevel(uint8_t pin)
{
    return (uint8_t)gpio_get_level(pin);
}

void initialGpioDefault(void)
{
    gpio_config_t dc = {
        .pin_bit_mask = (1ULL << PIN_EPD_DC),
        .mode = GPIO_MODE_OUTPUT,
    };
    ESP_ERROR_CHECK(gpio_config(&dc));
    gpio_set_level(PIN_EPD_DC, GPIO_LOW);

    gpio_config_t ctrl = {
        .pin_bit_mask = (1ULL << PIN_EPD_RST) | (1ULL << PIN_EPD_CS0) | (1ULL << PIN_EPD_CS1),
        .mode = GPIO_MODE_OUTPUT,
        .pull_up_en = GPIO_PULLUP_DISABLE,
        .pull_down_en = GPIO_PULLDOWN_DISABLE,
    };
    ESP_ERROR_CHECK(gpio_config(&ctrl));
    gpio_set_level(PIN_EPD_CS0, GPIO_HIGH);
    gpio_set_level(PIN_EPD_CS1, GPIO_HIGH);
    gpio_set_level(PIN_EPD_RST, GPIO_HIGH);

    gpio_float((1ULL << PIN_EPD_SCK) |
               (1ULL << PIN_EPD_D0) |
               (1ULL << PIN_EPD_D1) |
               (1ULL << PIN_EPD_D2) |
               (1ULL << PIN_EPD_D3));

    gpio_config_t busy = {
        .pin_bit_mask = (1ULL << PIN_EPD_BUSY),
        .mode = GPIO_MODE_INPUT,
        .pull_up_en = GPIO_PULLUP_DISABLE,
        .pull_down_en = GPIO_PULLDOWN_DISABLE,
        .intr_type = GPIO_INTR_DISABLE,
    };
    ESP_ERROR_CHECK(gpio_config(&busy));
    ESP_LOGI(TAG, "D/C GPIO=%d LOW, BUSY GPIO=%d initial level=%d, RST GPIO=%d HIGH",
             PIN_EPD_DC, PIN_EPD_BUSY, gpio_get_level(PIN_EPD_BUSY), PIN_EPD_RST);
}

void initialGpio(void)
{
    gpio_set_level(PIN_EPD_CS0, GPIO_HIGH);
    gpio_set_level(PIN_EPD_CS1, GPIO_HIGH);
    gpio_set_level(PIN_EPD_RST, GPIO_HIGH);
}

void diagnoseBusyPin(void)
{
    gpio_set_pull_mode(PIN_EPD_BUSY, GPIO_PULLUP_ONLY);
    vTaskDelay(pdMS_TO_TICKS(5));
    int with_up = gpio_get_level(PIN_EPD_BUSY);
    gpio_set_pull_mode(PIN_EPD_BUSY, GPIO_PULLDOWN_ONLY);
    vTaskDelay(pdMS_TO_TICKS(5));
    int with_down = gpio_get_level(PIN_EPD_BUSY);
    gpio_set_pull_mode(PIN_EPD_BUSY, GPIO_FLOATING);
    ESP_LOGE(TAG, "BUSY diagnostic: pull-up=%d pull-down=%d", with_up, with_down);
    if (with_up == 1 && with_down == 0) {
        ESP_LOGE(TAG, "BUSY follows internal pulls: GPIO%d is probably open/not connected", PIN_EPD_BUSY);
    } else if (with_up == 0 && with_down == 0) {
        ESP_LOGE(TAG, "BUSY is strongly LOW: DDIC is unpowered, held reset, busy, or pin mapping is wrong");
    }
}

uint8_t initialSpi(void)
{
    if (s_chunk == NULL) {
        s_chunk = heap_caps_aligned_alloc(32, SPI_MAX_BUFFER_SIZE, MALLOC_CAP_DMA);
        if (s_chunk == NULL) {
            ESP_LOGE(TAG, "DMA chunk alloc failed");
            return ERROR;
        }
    }

    spi_bus_config_t bus = {
        .mosi_io_num = PIN_EPD_D0,
        .miso_io_num = PIN_EPD_D1,
        .sclk_io_num = PIN_EPD_SCK,
        .data2_io_num = -1,
        .data3_io_num = -1,
        .flags = SPICOMMON_BUSFLAG_MASTER,
        .max_transfer_sz = SPI_MAX_BUFFER_SIZE,
    };
    esp_err_t err = spi_bus_initialize(EPD_SPI_HOST, &bus, SPI_DMA_CH_AUTO);
    if (err != ESP_OK) {
        ESP_LOGE(TAG, "spi_bus_initialize: %s", esp_err_to_name(err));
        return ERROR;
    }

    /* 4 线：8-bit 命令相位 + D/C，CS 用 GPIO。与 reference/7.09eink 一致。 */
    spi_device_interface_config_t dev = {
        .command_bits = 8,
        .clock_speed_hz = EPD_SPI_FREQ_HZ,
        .mode = 0,
        .spics_io_num = -1,
        .queue_size = 1,
        .cs_ena_posttrans = 3,
        .flags = SPI_DEVICE_HALFDUPLEX,
    };
    err = spi_bus_add_device(EPD_SPI_HOST, &dev, &spi);
    if (err != ESP_OK) {
        ESP_LOGE(TAG, "spi_bus_add_device: %s", esp_err_to_name(err));
        return ERROR;
    }

    ESP_LOGI(TAG, "initialSpi() 4-wire command_bits=8 half-duplex");
    return DONE;
}

static uint8_t spi_tx(const uint8_t *data, unsigned int length)
{
    if (length == 0) {
        return DONE;
    }
    gpio_set_level(PIN_EPD_DC, GPIO_HIGH);
    spi_transaction_ext_t trans = {
        .base = {
            .length = length * 8,
            .tx_buffer = data,
            .flags = SPI_TRANS_VARIABLE_CMD,
        },
        .command_bits = 0,
    };
    esp_err_t err = spi_device_polling_transmit(spi, &trans.base);
    if (err != ESP_OK) {
        ESP_LOGE(TAG, "SPI tx: %s", esp_err_to_name(err));
        return ERROR;
    }
    return DONE;
}

uint8_t spiTransmitCommand(uint8_t commandBuf)
{
    gpio_set_level(PIN_EPD_DC, GPIO_LOW);
    spi_transaction_t trans = {
        .cmd = commandBuf,
        .length = 0,
    };
    esp_err_t err = spi_device_polling_transmit(spi, &trans);
    if (err != ESP_OK) {
        ESP_LOGE(TAG, "SPI cmd: %s", esp_err_to_name(err));
        return ERROR;
    }
    return DONE;
}

uint8_t spiTransmit(uint8_t commandBuf, uint8_t *dataBuffer, unsigned int dataLength)
{
    uint8_t status = spiTransmitCommand(commandBuf);
    if (status != DONE || dataBuffer == NULL || dataLength == 0) {
        return status;
    }
    return spi_tx(dataBuffer, dataLength);
}

uint8_t spiTransmitData(uint8_t *dataBuffer, unsigned long dataLength)
{
    uint8_t status = DONE;
    while (dataLength > 0 && status == DONE) {
        unsigned long n = (dataLength > SPI_MAX_BUFFER_SIZE) ? SPI_MAX_BUFFER_SIZE : dataLength;
        status = spi_tx(dataBuffer, (unsigned int)n);
        dataBuffer += n;
        dataLength -= n;
    }
    return status;
}

uint8_t spiTransmitLargeData(uint8_t commandBuf, uint8_t *dataBuffer, unsigned long dataLength)
{
    uint8_t status = spiTransmitCommand(commandBuf);
    if (status != DONE) {
        return status;
    }
    return spiTransmitData(dataBuffer, dataLength);
}

uint8_t spiReceive(uint8_t commandBuf, uint8_t *dataBuffer, unsigned int dataLength)
{
    uint8_t status = spiTransmitCommand(commandBuf);
    if (status != DONE) {
        return status;
    }
    return spiReceiveData(dataBuffer, dataLength);
}

uint8_t spiReceiveData(uint8_t *dataBuffer, unsigned int dataLength)
{
    if (dataBuffer == NULL || dataLength == 0) {
        return DONE;
    }
    gpio_set_level(PIN_EPD_DC, GPIO_HIGH);
    spi_transaction_ext_t trans = {
        .base = {
            .rxlength = dataLength * 8,
            .flags = SPI_TRANS_VARIABLE_CMD,
        },
        .command_bits = 0,
    };
    if (dataLength <= 4) {
        trans.base.flags |= SPI_TRANS_USE_RXDATA;
    } else {
        trans.base.rx_buffer = dataBuffer;
    }
    esp_err_t err = spi_device_polling_transmit(spi, &trans.base);
    if (err != ESP_OK) {
        ESP_LOGE(TAG, "SPI rx: %s", esp_err_to_name(err));
        return ERROR;
    }
    if (dataLength <= 4) {
        memcpy(dataBuffer, trans.base.rx_data, dataLength);
    }
    return DONE;
}
