#ifndef MAX30102_H
#define MAX30102_H

#include <stdio.h>
#include <fcntl.h>
#include <unistd.h>
#include <linux/i2c-dev.h>
#include <sys/ioctl.h>
#include <stdint.h>
#include <errno.h>
#include <string.h>

#define MAX30102_ADDR 0x57
#define TCA9548A_ADDR 0x70

// MAX30102 Register
#define REG_INTR_STATUS_1 0x00
#define REG_INTR_STATUS_2 0x01
#define REG_INTR_ENABLE_1 0x02
#define REG_INTR_ENABLE_2 0x03
#define REG_FIFO_WR_PTR 0x04
#define REG_OVF_COUNTER 0x05
#define REG_FIFO_RD_PTR 0x06
#define REG_FIFO_DATA 0x07
#define REG_FIFO_CONFIG 0x08
#define REG_MODE_CONFIG 0x09
#define REG_SPO2_CONFIG 0x0A
#define REG_RED_LED 0x0C
#define REG_IR_LED 0x0D
#define REG_PILOT_PA 0x10
#define REG_MULTI_LED_CTRL1 0x11
#define REG_MULTI_LED_CTRL2 0x12

//PRO MODE
// #define PRO_MODE_TO_REG_RED_LED 0x00
// #define PRO_MODE_TO_REG_IR_LED 0x00
// #define PRO_MODE_TO_REG_SPO2_CONFIG 0x43
// #define PRO_MODE_TO_REG_FIFO_CONFIG 0x00
// #define PRO_MODE_TO_REG_MULTI_LED_CTRL1 0x12
// #define PRO_MODE_TO_REG_MULTI_LED_CTRL2 0x00
// #define PRO_MODE_TO_REG_FIFO_WR_PTR 0x00
// #define PRO_MODE_TO_REG_OVF_COUNTER 0x00
// #define PRO_MODE_TO_REG_FIFO_RD_PTR 0x00      // FIFO RESET
// #define PRO_MODE_TO_REG_MODE_CONFIG 0x07     // Mode Configuration: Multi-LED Mode
// #define PRO_MODE_TO_REG_INTR_ENABLE_1 0x40   // PPG RDY Interrupt Enable

//SPO2 MODE
// #define SPO2_MODE_TO_REG_FIFO_WR_PTR 0x00
// #define SPO2_MODE_TO_REG_OVF_COUNTER 0x00
// #define SPO2_MODE_TO_REG_FIFO_RD_PTR 0x00
// #define SPO2_MODE_TO_REG_INTR_ENABLE_1 0xE0
// #define SPO2_MODE_TO_REG_INTR_ENABLE_2 0x00
// #define SPO2_MODE_TO_REG_FIFO_CONFIG 0x0F
// #define SPO2_MODE_TO_REG_MODE_CONFIG 0x40
// #define SPO2_MODE_TO_REG_SPO2_CONFIG 0x27
// #define SPO2_MODE_TO_REG_RED_LED 0x24
// #define SPO2_MODE_TO_REG_IR_LED 0x24
// #define SPO2_MODE_TO_REG_PILOT_PA 0x7F


class MAX30102
{
public:
    MAX30102(const char *device, uint8_t tcaAddress = TCA9548A_ADDR, uint8_t maxAddress = MAX30102_ADDR);
    ~MAX30102();

    void max30102_init();
    void writeRegister(uint8_t reg,uint8_t add);

    void read_fifo(uint32_t *red_led, uint32_t *ir_led);
    bool get_data(uint32_t *red_led,uint32_t *ir_led);
    
private:
    const char *device;
    uint8_t tcaAddress;
    uint8_t maxAddress;
    int fd;
};

#endif