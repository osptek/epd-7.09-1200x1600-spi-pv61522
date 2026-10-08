#include <string.h>

#include "esp_log.h"
#include "esp_timer.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

#include "board_pins.h"
#include "epd709.h"
#include "epd709_comm.h"
#include "status.h"

static const char *TAG = "epd709";

#define EPD_INIT_BUSY_TIMEOUT_MS        15000
#define EPD_REFRESH_BUSY_TIMEOUT_MS     60000

static const uint8_t spiCsPin[2] = {
    PIN_EPD_CS0, PIN_EPD_CS1
};

static const uint8_t PSR_V[2] = { 0xDF, 0x6B };
static const uint8_t PWR_V[6] = { 0x0F, 0x00, 0x28, 0x2C, 0x28, 0x38 };
static const uint8_t POF_V[1] = { 0x01 };
static const uint8_t POFS_MV[4] = { 0x00, 0xC0, 0x03, 0xA8 };
static const uint8_t POFS_SV[4] = { 0x00, 0xC0, 0x03, 0x9A };
static const uint8_t DRF_V[1] = { 0x00 };
static const uint8_t PLL_V[1] = { 0x08 };
static const uint8_t CDI_V[1] = { 0x37 };
static const uint8_t TCON_V[2] = { 0x03, 0x03 };
static const uint8_t TRES_V[4] = { 0x04, 0xB0, 0x03, 0x20 };
static const uint8_t CMD66_V[6] = { 0x49, 0x55, 0x13, 0x5D, 0x05, 0x10 };
static const uint8_t EN_BUF_V[1] = { 0x07 };
static const uint8_t CCSET_V[1] = { 0x01 };
static const uint8_t PWS_V[1] = { 0x22 };
static const uint8_t AN_TM_V[9] = { 0x00, 0x0C, 0x0C, 0xD9, 0xDD, 0xDD, 0x15, 0x15, 0x55 };
static const uint8_t AGID_V[1] = { 0x10 };
static const uint8_t CMDA4_V[9] = { 0x03, 0x00, 0x01, 0x03, 0x00, 0x03, 0x00, 0x00, 0x00 };
static const uint8_t DCDC_V[3] = { 0x44, 0x54, 0x00 };
static const uint8_t BTST_P_V[2] = { 0xE0, 0x20 };
static const uint8_t BOOST_VDDP_EN_V[1] = { 0x01 };
static const uint8_t BTST_N_V[2] = { 0xE0, 0x20 };
static const uint8_t BUCK_BOOST_VDDN_V[1] = { 0x01 };
static const uint8_t TFT_VCOM_POWER_V[1] = { 0x02 };

char partialWindowUpdateStatus = DONE;

void resetPin(unsigned int pinStatus)
{
    setGpioLevel(PIN_EPD_RST, pinStatus);
}

void setPinCsAll(unsigned int setLevel)
{
    unsigned char i;
    for (i = 0; i < 2; i++) {
        setGpioLevel(spiCsPin[i], setLevel);
    }
}

void setPinCs(uint8_t csNumber, unsigned int setLevel)
{
    setGpioLevel(spiCsPin[csNumber], setLevel);
}

uint8_t checkBusyHigh(const char *stage, uint32_t timeout_ms)
{
    const int64_t start = esp_timer_get_time();
    int64_t last_log = start;
    ESP_LOGI(TAG, "%s: wait BUSY high, now=%d", stage, getGpioLevel(PIN_EPD_BUSY));
    while (!getGpioLevel(PIN_EPD_BUSY)) {
        int64_t now = esp_timer_get_time();
        if ((now - start) / 1000 >= timeout_ms) {
            ESP_LOGE(TAG, "%s: BUSY LOW timeout; check VDD/RST/BUSY", stage);
            return ERROR;
        }
        if (now - last_log > 5 * 1000 * 1000) {
            ESP_LOGW(TAG, "%s: still waiting BUSY high, elapsed=%ds",
                     stage, (int)((now - start) / 1000000));
            last_log = now;
        }
        vTaskDelay(pdMS_TO_TICKS(10));
    }
    ESP_LOGI(TAG, "%s: BUSY HIGH (%lld ms)", stage, (long long)((esp_timer_get_time() - start) / 1000));
    return DONE;
}

void checkBusyLow(void)
{
    const int64_t start = esp_timer_get_time();
    while (getGpioLevel(PIN_EPD_BUSY)) {
        if ((esp_timer_get_time() - start) / 1000 >= EPD_REFRESH_BUSY_TIMEOUT_MS) {
            ESP_LOGE(TAG, "wait BUSY low timeout, BUSY=%d", getGpioLevel(PIN_EPD_BUSY));
            return;
        }
        vTaskDelay(pdMS_TO_TICKS(10));
    }
}

void epdHardwareReset(void)
{
    delayms(5);
    resetPin(GPIO_LOW);
    delayms(20);
    resetPin(GPIO_HIGH);
    delayms(50);
}

void writeEpd(uint8_t epdCommand, uint8_t *epdData, unsigned int epdDataLength)
{
    uint8_t *chunk = epdChunkBuffer();
    if (chunk != NULL && epdData != NULL && epdDataLength > 0 && epdDataLength <= EPD_SPI_CHUNK_SIZE) {
        memcpy(chunk, epdData, epdDataLength);
        spiTransmit(epdCommand, chunk, epdDataLength);
        return;
    }
    spiTransmit(epdCommand, epdData, epdDataLength);
}

void readEpd(uint8_t epdCommand, uint8_t *epdData, unsigned int epdDataLength)
{
    spiReceive(epdCommand, epdData, epdDataLength);
}

void writeEpdCommand(uint8_t epdCommand)
{
    spiTransmitCommand(epdCommand);
}

void writeEpdData(uint8_t *epdData, unsigned int epdDataLength)
{
    spiTransmitData(epdData, epdDataLength);
}

uint8_t initEPD(void)
{
    initialGpioDefault();
    epdHardwareReset();
    if (checkBusyHigh("hardware reset", EPD_INIT_BUSY_TIMEOUT_MS) != DONE) {
        diagnoseBusyPin();
        return ERROR;
    }
    if (initialSpi() != DONE) {
        return ERROR;
    }

    setPinCs(0, GPIO_LOW);
    writeEpd(AN_TM, (uint8_t *)AN_TM_V, sizeof(AN_TM_V));
    setPinCsAll(GPIO_HIGH);

    setPinCsAll(GPIO_LOW);
    writeEpd(CMD66, (uint8_t *)CMD66_V, sizeof(CMD66_V));
    setPinCsAll(GPIO_HIGH);

    setPinCsAll(GPIO_LOW);
    writeEpd(PSR, (uint8_t *)PSR_V, sizeof(PSR_V));
    setPinCsAll(GPIO_HIGH);

    setPinCs(0, GPIO_LOW);
    writeEpd(DCDC, (uint8_t *)DCDC_V, sizeof(DCDC_V));
    setPinCsAll(GPIO_HIGH);

    setPinCsAll(GPIO_LOW);
    writeEpd(PLL, (uint8_t *)PLL_V, sizeof(PLL_V));
    setPinCsAll(GPIO_HIGH);

    setPinCsAll(GPIO_LOW);
    writeEpd(CDI, (uint8_t *)CDI_V, sizeof(CDI_V));
    setPinCsAll(GPIO_HIGH);

    setPinCsAll(GPIO_LOW);
    writeEpd(TCON, (uint8_t *)TCON_V, sizeof(TCON_V));
    setPinCsAll(GPIO_HIGH);

    setPinCs(0, GPIO_LOW);
    writeEpd(POFS, (uint8_t *)POFS_MV, sizeof(POFS_MV));
    setPinCsAll(GPIO_HIGH);

    setPinCs(1, GPIO_LOW);
    writeEpd(POFS, (uint8_t *)POFS_SV, sizeof(POFS_SV));
    setPinCsAll(GPIO_HIGH);

    setPinCsAll(GPIO_LOW);
    writeEpd(AGID, (uint8_t *)AGID_V, sizeof(AGID_V));
    setPinCsAll(GPIO_HIGH);

    setPinCsAll(GPIO_LOW);
    writeEpd(PWS, (uint8_t *)PWS_V, sizeof(PWS_V));
    setPinCsAll(GPIO_HIGH);

    setPinCsAll(GPIO_LOW);
    writeEpd(CCSET, (uint8_t *)CCSET_V, sizeof(CCSET_V));
    setPinCsAll(GPIO_HIGH);

    setPinCsAll(GPIO_LOW);
    writeEpd(TRES, (uint8_t *)TRES_V, sizeof(TRES_V));
    setPinCsAll(GPIO_HIGH);

    setPinCs(0, GPIO_LOW);
    writeEpd(CMDA4, (uint8_t *)CMDA4_V, sizeof(CMDA4_V));
    setPinCsAll(GPIO_HIGH);

    setPinCs(0, GPIO_LOW);
    writeEpd(PWR, (uint8_t *)PWR_V, sizeof(PWR_V));
    setPinCsAll(GPIO_HIGH);

    setPinCs(0, GPIO_LOW);
    writeEpd(EN_BUF, (uint8_t *)EN_BUF_V, sizeof(EN_BUF_V));
    setPinCsAll(GPIO_HIGH);

    setPinCs(0, GPIO_LOW);
    writeEpd(BTST_P, (uint8_t *)BTST_P_V, sizeof(BTST_P_V));
    setPinCsAll(GPIO_HIGH);

    setPinCs(0, GPIO_LOW);
    writeEpd(BOOST_VDDP_EN, (uint8_t *)BOOST_VDDP_EN_V, sizeof(BOOST_VDDP_EN_V));
    setPinCsAll(GPIO_HIGH);

    setPinCs(0, GPIO_LOW);
    writeEpd(BTST_N, (uint8_t *)BTST_N_V, sizeof(BTST_N_V));
    setPinCsAll(GPIO_HIGH);

    setPinCs(0, GPIO_LOW);
    writeEpd(BUCK_BOOST_VDDN, (uint8_t *)BUCK_BOOST_VDDN_V, sizeof(BUCK_BOOST_VDDN_V));
    setPinCsAll(GPIO_HIGH);

    setPinCs(0, GPIO_LOW);
    writeEpd(TFT_VCOM_POWER, (uint8_t *)TFT_VCOM_POWER_V, sizeof(TFT_VCOM_POWER_V));
    setPinCsAll(GPIO_HIGH);

    ESP_LOGI(TAG, "initEPD() has been executed.");
    checkDriverICStatus();
    return DONE;
}

uint8_t checkDriverICStatus(void)
{
    uint8_t csx, status = DONE;
    uint8_t dataBuf[3];

    for (csx = 0; csx < 2; csx++) {
        memset(dataBuf, 0, sizeof(dataBuf));
        setPinCs(csx, GPIO_LOW);
        readEpd(0xF2, dataBuf, sizeof(dataBuf));
        setPinCs(csx, GPIO_HIGH);
        ESP_LOGI(TAG, "Driver IC [%d] = 0x%02X 0x%02X 0x%02X", csx, dataBuf[0], dataBuf[1], dataBuf[2]);
        if ((dataBuf[0] & 0x01) == 0x01) {
            ESP_LOGI(TAG, "Driver IC [%d] is ready.", csx);
        } else {
            ESP_LOGW(TAG, "Driver IC [%d] did not reply.", csx);
            status = ERROR;
        }
    }
    return status;
}

static uint8_t send_cmd_hold_cs_wait(const char *what, uint8_t cmd, uint8_t *data, unsigned int len)
{
    setPinCsAll(GPIO_LOW);
    if (data != NULL && len > 0) {
        writeEpd(cmd, data, len);
    } else {
        writeEpdCommand(cmd);
    }
    delayms(1);
    uint8_t status = checkBusyHigh(what, EPD_REFRESH_BUSY_TIMEOUT_MS);
    setPinCsAll(GPIO_HIGH);
    return status;
}

uint8_t epdDisplay(void)
{
    ESP_LOGI(TAG, "Write PON");
    if (send_cmd_hold_cs_wait("PON", PON, NULL, 0) != DONE) {
        return ERROR;
    }

    ESP_LOGI(TAG, "Write DRF");
    setPinCsAll(GPIO_LOW);
    delayms(30);
    writeEpd(DRF, (uint8_t *)DRF_V, sizeof(DRF_V));
    delayms(1);
    if (checkBusyHigh("DRF", EPD_REFRESH_BUSY_TIMEOUT_MS) != DONE) {
        setPinCsAll(GPIO_HIGH);
        return ERROR;
    }
    setPinCsAll(GPIO_HIGH);

    ESP_LOGI(TAG, "Write POF");
    if (send_cmd_hold_cs_wait("POF", POF, (uint8_t *)POF_V, sizeof(POF_V)) != DONE) {
        return ERROR;
    }
    ESP_LOGI(TAG, "Display Done!!");
    return DONE;
}

void executeTscCommand(void)
{
    uint8_t readTscBuf[2] = {0};

    setPinCs(0, GPIO_LOW);
    spiTransmitCommand(TSC);
    if (checkBusyHigh("TSC", EPD_INIT_BUSY_TIMEOUT_MS) != DONE) {
        setPinCs(0, GPIO_HIGH);
        return;
    }
    spiReceiveData(&readTscBuf[0], sizeof(readTscBuf));
    setPinCs(0, GPIO_HIGH);
    delayms(30);
    ESP_LOGI(TAG, "TSC Data = 0x%02X, 0x%02X", readTscBuf[0], readTscBuf[1]);
}

static void fill_color(uint8_t color, unsigned long bytes)
{
    uint8_t *chunk = epdChunkBuffer();
    memset(chunk, color, EPD_SPI_CHUNK_SIZE);
    while (bytes > 0) {
        unsigned long n = (bytes > EPD_SPI_CHUNK_SIZE) ? EPD_SPI_CHUNK_SIZE : bytes;
        writeEpdData(chunk, (unsigned int)n);
        bytes -= n;
    }
}

void epdDisplayColor(uint8_t colorSelect)
{
    for (uint8_t ic = 0; ic < 2; ic++) {
        setPinCs(ic, GPIO_LOW);
        writeEpdCommand(DTM);
        fill_color(colorSelect, EPD_IMAGE_BYTES_PER_IC);
        setPinCs(ic, GPIO_HIGH);
        ESP_LOGI(TAG, "IC%u framebuffer sent", ic);
    }
    epdDisplay();
    ESP_LOGI(TAG, "Display color complete.");
}

uint8_t epdDisplayColorBar(void)
{
    static const uint8_t colors[] = {
        BLACK, BLUE, GREEN, RED, YELLOW, WHITE,
    };
    for (uint8_t ic = 0; ic < 2; ic++) {
        setPinCs(ic, GPIO_LOW);
        writeEpdCommand(DTM);
        for (unsigned band = 0; band < sizeof(colors); band++) {
            fill_color(colors[band], EPD_COLOR_BAR_BYTES);
        }
        setPinCs(ic, GPIO_HIGH);
        ESP_LOGI(TAG, "IC%u six-stripe framebuffer sent", ic);
    }
    if (epdDisplay() != DONE) {
        return ERROR;
    }
    ESP_LOGI(TAG, "Display color bar complete.");
    return DONE;
}

void writeEpdImage(uint8_t csx, const uint8_t *imageData, unsigned long imageDataLength)
{
    if (csx == 0) {
        executeTscCommand();
    }
    setPinCs(csx, GPIO_LOW);
    spiTransmitLargeData(DTM, (uint8_t *)imageData, imageDataLength);
    setPinCs(csx, GPIO_HIGH);
    ESP_LOGI(TAG, "Writing data is completed.");
}

static char validate_partial_window(uint8_t csx,
                                    unsigned int xStart, unsigned int yStart,
                                    unsigned int xPixel, unsigned int yLine,
                                    unsigned int *HRST, unsigned int *HRED,
                                    unsigned int *VRST, unsigned int *VRED)
{
    *HRST = xStart * 2;
    *HRED = (xStart + xPixel) * 2 - 1;
    *VRST = yStart / 2;
    *VRED = (yStart + yLine) / 2 - 1;

    if (*HRST % 8 != 0) {
        return -1;
    }
    if ((*HRED - 7) % 8 != 0) {
        return -2;
    }
    if ((xStart > 584) || (xPixel > 600)) {
        return -3;
    }
    if ((*HRED - *HRST + 1 < 32) || (*HRED + 1 > 1200)) {
        return -4;
    }
    if ((yStart + yLine) % 2 != 0) {
        return -5;
    }
    if ((yStart > 1596) || (yLine > 1600)) {
        return -6;
    }
    if (((int)(*VRED - *VRST) + 1 <= 0) || (*VRED + 1 > 800)) {
        return -7;
    }
    if (csx > 1) {
        return -8;
    }
    return DONE;
}

char partialWindowUpdateWithImageData(uint8_t csx, const uint8_t *imageData, unsigned long imageDataLength,
                                      unsigned int xStart, unsigned int yStart,
                                      unsigned int xPixel, unsigned int yLine,
                                      uint8_t epdDisplayEnable)
{
    unsigned int HRST, HRED, VRST, VRED;
    uint8_t partialWindowData[9];
    char status = validate_partial_window(csx, xStart, yStart, xPixel, yLine,
                                          &HRST, &HRED, &VRST, &VRED);
    if (status == DONE) {
        memset(partialWindowData, 0, sizeof(partialWindowData));
        partialWindowData[0] = (uint8_t)(HRST >> 8);
        partialWindowData[1] = (uint8_t)HRST;
        partialWindowData[2] = (uint8_t)(HRED >> 8);
        partialWindowData[3] = (uint8_t)HRED;
        partialWindowData[4] = (uint8_t)(VRST >> 8);
        partialWindowData[5] = (uint8_t)VRST;
        partialWindowData[6] = (uint8_t)(VRED >> 8);
        partialWindowData[7] = (uint8_t)VRED;
        partialWindowData[8] = PTLW_ENABLE;

        if (csx == 0) {
            executeTscCommand();
        }
        setPinCs(csx, GPIO_LOW);
        writeEpd(PTLW, partialWindowData, sizeof(partialWindowData));
        setPinCs(csx, GPIO_HIGH);

        setPinCs(csx, GPIO_LOW);
        spiTransmitLargeData(DTM, (uint8_t *)imageData, imageDataLength);
        setPinCs(csx, GPIO_HIGH);
    } else {
        partialWindowUpdateStatus = ERROR;
        ESP_LOGW(TAG, "partialWindowUpdateStatus = ERROR %d", (int)status);
    }

    if (epdDisplayEnable) {
        if (partialWindowUpdateStatus == DONE) {
            epdDisplay();
        }
        delayms(300);
        memset(partialWindowData, 0, sizeof(partialWindowData));
        partialWindowData[8] = PTLW_DISABLE;
        partialWindowUpdateStatus = DONE;
        setPinCsAll(GPIO_LOW);
        writeEpd(PTLW, partialWindowData, sizeof(partialWindowData));
        setPinCsAll(GPIO_HIGH);
    }
    return status;
}

char partialWindowUpdateWithoutImageData(uint8_t csx, unsigned int xStart, unsigned int yStart,
                                         unsigned int xPixel, unsigned int yLine,
                                         uint8_t epdDisplayEnable)
{
    unsigned int HRST, HRED, VRST, VRED;
    uint8_t partialWindowData[9];
    char status = validate_partial_window(csx, xStart, yStart, xPixel, yLine,
                                          &HRST, &HRED, &VRST, &VRED);
    if (status == DONE) {
        memset(partialWindowData, 0, sizeof(partialWindowData));
        partialWindowData[0] = (uint8_t)(HRST >> 8);
        partialWindowData[1] = (uint8_t)HRST;
        partialWindowData[2] = (uint8_t)(HRED >> 8);
        partialWindowData[3] = (uint8_t)HRED;
        partialWindowData[4] = (uint8_t)(VRST >> 8);
        partialWindowData[5] = (uint8_t)VRST;
        partialWindowData[6] = (uint8_t)(VRED >> 8);
        partialWindowData[7] = (uint8_t)VRED;
        partialWindowData[8] = PTLW_ENABLE;

        if (csx == 0) {
            executeTscCommand();
        }
        setPinCs(csx, GPIO_LOW);
        writeEpd(PTLW, partialWindowData, sizeof(partialWindowData));
        setPinCs(csx, GPIO_HIGH);
    } else {
        partialWindowUpdateStatus = ERROR;
        ESP_LOGW(TAG, "partialWindowUpdateStatus = ERROR %d", (int)status);
    }

    if (epdDisplayEnable) {
        if (partialWindowUpdateStatus == DONE) {
            epdDisplay();
        }
        delayms(300);
        memset(partialWindowData, 0, sizeof(partialWindowData));
        partialWindowData[8] = PTLW_DISABLE;
        partialWindowUpdateStatus = DONE;
        setPinCsAll(GPIO_LOW);
        writeEpd(PTLW, partialWindowData, sizeof(partialWindowData));
        setPinCsAll(GPIO_HIGH);
    }
    return status;
}
