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

#define MAX30102_ADDR 0x57 // MAX30102的I2C地址
#define TCA9548A_ADDR 0x70 // TCA9548A的I2C地址

// MAX30102 寄存器地址
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

enum Mode
{
    PROXIMITY,
    HRM_SPO2
};

class MAX30102{
public:
    MAX30102(const char *device,uint8_t tcaAddress=TCA9548A_ADDR,uint8_t maxAddress=MAX30102_ADDR);
    ~MAX30102();

    void max30102_init(enum Mode mode);
    void writeRegister(int max_fd,uint8_t reg,uint8_t add);
    
    void read_fifo(uint32_t *red_led, uint32_t *ir_led, int fd);
    void DoJob(uint32_t *ir_data, uint32_t *red_data);
    void PreJob();

    

private:
    const char *device;
    uint8_t tcaAddress;
    uint8_t maxAddress;
    int fd;
    //int max30102_fd;
    enum Mode currentMode;
    int enable_channels[8];
    int enable_channel_num = 0;
    int count_channel = 0;
    int now_use_channel = 0;

};

#endif