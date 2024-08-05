#include <stdio.h>
#include <stdint.h>
#include <string.h>

#include "pico/stdlib.h"
#include "pico/time.h"
#include "pico/multicore.h"
#include "hardware/uart.h"
// #include "pico/cyw43_arch.h"

#define START_FRAME 0xABCD
#define BAUDRATE 115200
#define UART uart0
#define UART_TX_PIN 0
#define UART_RX_PIN 1
#define BUTTON_PIN 3

#define UART_PC_TX_PIN 4
#define UART_PC_RX_PIN 5

#define UART_PC uart1

const int INTERVAL = 1000 / 40;
bool button_state;

typedef enum
{
    IDLE,
    RECEIVING,
    READY
} STATE;

struct FDB
{
    uint16_t start;
    int16_t cmd1;
    int16_t cmd2;
    int16_t speedR_meas;
    int16_t speedL_meas;
    int16_t wheelR_cnt;
    int16_t wheelL_cnt;
    int16_t batVoltage;
    int16_t boardTemp;
    uint16_t cmdLed;
    uint16_t checksum;
} feedback;

struct FDBWB
{
    FDB data;
    uint8_t button_state;
} feedback_with_button;

struct CMD
{
    uint16_t start;
    int16_t steer;
    int16_t speed;
    uint16_t checksum;
} command, send, zeroSpeed;

void pc2hover();
void hover2pc();

int main()
{

    stdio_init_all();
    uart_init(UART, BAUDRATE);
    uart_init(UART_PC, BAUDRATE);

    gpio_set_function(UART_TX_PIN, GPIO_FUNC_UART);
    gpio_set_function(UART_RX_PIN, GPIO_FUNC_UART);

    gpio_set_function(UART_PC_TX_PIN, GPIO_FUNC_UART);
    gpio_set_function(UART_PC_RX_PIN, GPIO_FUNC_UART);

    gpio_init(BUTTON_PIN);
    gpio_set_dir(BUTTON_PIN, GPIO_OUT);
    gpio_pull_down(BUTTON_PIN);

    // COMMAND CONFIG
    int time = to_ms_since_boot(get_absolute_time());
    int now = 0;
    command.start = (uint16_t)START_FRAME;

    zeroSpeed.start = (uint16_t)START_FRAME;
    zeroSpeed.speed = (uint16_t)0;
    zeroSpeed.steer = (uint16_t)0;
    zeroSpeed.checksum = (uint16_t)((send.start ^ send.steer ^ send.speed));
    multicore_launch_core1(pc2hover);

    hover2pc();


}

void pc2hover()
{
    int lastUpdate = to_ms_since_boot(get_absolute_time());

    STATE usb_state = IDLE;
    uint16_t usb_start = 0x0;
    int usb_idx = 0;
    uint8_t *usb_p = (uint8_t *)&command;
    uint16_t usb_checksum = 0x0;
    bool new_message = false;
    uint8_t byte;
    int now = 0;
    int time = to_ms_since_boot(get_absolute_time());

    while (true)
    {
        button_state = gpio_get(BUTTON_PIN);
        now = to_ms_since_boot(get_absolute_time());
        switch (usb_state)
        {
        case IDLE:
            if (!uart_is_readable(UART_PC))
                break;
            byte = (uint8_t)uart_getc(UART_PC);
            usb_start = ((uint16_t)(byte) << 8) | (usb_start >> 8);
            if (usb_start == START_FRAME)
            {
                // printf("Start %x\n", usb_start);
                command.start = usb_start;
                usb_p += 2;
                usb_idx = 2;
                usb_state = RECEIVING;
            }
            break;
        case RECEIVING:
            if (!uart_is_readable(UART_PC))
                break;
            byte = (uint8_t)uart_getc(UART_PC);
            *usb_p = byte;
            usb_p++;
            usb_idx++;
            // p++;
            if (sizeof(CMD) == usb_idx)
                usb_state = READY;
            break;
        case READY:
            usb_checksum = (uint16_t)((command.start ^ command.steer ^ command.speed));
            if (usb_checksum == command.checksum)
            {
                send = command;
                lastUpdate = to_ms_since_boot(get_absolute_time());
            }
            usb_start = 0x0;
            usb_p = (uint8_t *)&command;
            usb_idx = 0;
            usb_state = IDLE;
            new_message = true;
            break;
        }
        if (!button_state || (now - lastUpdate > 500))
        {
            uart_write_blocking(UART, (uint8_t *)&zeroSpeed, sizeof(CMD));
        }
        else if (new_message)
        {
            uart_write_blocking(UART, (uint8_t *)&send, sizeof(CMD));
            time = now;
            new_message = false;
        }
    }
}

void hover2pc()
{
    // FEEDBACK CONFIG
    STATE state = IDLE;
    uint16_t start = 0x0;
    int idx = 0;
    uint8_t *p = (uint8_t *)&feedback;
    uint16_t checksum = 0x0;

    while (1)
    {
        switch (state)
        {
        case IDLE:
            if (!uart_is_readable(UART))
                break;
            start = ((uint16_t)(uart_getc(UART)) << 8) | (start >> 8);
            if (start == START_FRAME)
            {
                // printf("Start %x\n", start);
                feedback.start = start;
                p += 2;
                idx = 2;
                state = RECEIVING;
            }
            break;
        case RECEIVING:
            if (!uart_is_readable(UART))
                break;
            *p = uart_getc(UART);
            p++;
            idx++;
            // p++;
            if (sizeof(FDB) == idx)
                state = READY;
            break;
        case READY:
            checksum = (uint16_t)(feedback.start ^ feedback.cmd1 ^ feedback.cmd2 ^ feedback.speedR_meas ^ feedback.speedL_meas ^ feedback.wheelR_cnt ^ feedback.wheelL_cnt ^ feedback.batVoltage ^ feedback.boardTemp ^ feedback.cmdLed);
            if (checksum == feedback.checksum)
            {
                memcpy((uint8_t *)&feedback_with_button, (const uint8_t *)&feedback, sizeof(FDB));
                feedback_with_button.button_state = !button_state;
                feedback_with_button.data.checksum = (uint16_t)(checksum ^ feedback_with_button.button_state);
                uart_write_blocking(UART_PC, (uint8_t *)&feedback_with_button, sizeof(FDBWB));
            }
            start = 0x0;
            p = (uint8_t *)&feedback;
            idx = 0;
            state = IDLE;
            break;
        }
    }
}