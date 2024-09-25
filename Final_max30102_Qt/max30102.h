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
#include <MQTTClient.h>


#define ADDRESS     "tcp://192.168.31.142:1883" 
#define CLIENTID    "OrangePiClient"
#define TOPIC       "sensor/data"
#define QOS         1
#define TIMEOUT     10000L

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


class MAX30102
{
public:
    MAX30102(const char *device, uint8_t tcaAddress = TCA9548A_ADDR, uint8_t maxAddress = MAX30102_ADDR);
    ~MAX30102();

    void max30102_init(int fd);
    void writeRegister(uint8_t fd, uint8_t reg, uint8_t add);

    void read_fifo(uint32_t *red_led, uint32_t *ir_led,int fd);
    int init_i2c(const char *device, int addr);
    void scanf_channel();
    void init_channel_sensor();
    void get_middle_data(uint32_t *red_data,uint32_t *ir_data,MQTTClient client);
    void get_branch_data(uint32_t red_data[],uint32_t ir_data[],MQTTClient client);
    void Quit();

private:
    const char *device;
    uint8_t tcaAddress;
    uint8_t maxAddress;
    int fd;
    int max30102_fd;
    int enable_channels[8];
    int count_channel=0;
    int middle_channels[4]={0,2,4,6};
};

#endif