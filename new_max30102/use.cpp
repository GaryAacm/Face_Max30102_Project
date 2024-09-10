#include <iostream>
#include <fcntl.h>
#include <unistd.h>
#include <cstring>
#include <cstdio>
#include <cstdlib>
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
#define REG_LED1_PA 0x0C
#define REG_LED2_PA 0x0D
#define REG_PILOT_PA 0x10
#define REG_MULTI_LED_CTRL1 0x11
#define REG_MULTI_LED_CTRL2 0x12

enum Mode
{
    PROXIMITY,
    HRM_SPO2
};
enum Mode init_mode = PROXIMITY;   // 设置初始化为接近模式
enum Mode current_mode = HRM_SPO2; // 定义并初始化current_mode

int enable_channels[8];
int count_channel = 0;

void scanf_channel()
{
    for (int i = 0; i < 8; i++)
    {
        char command[128];
        snprintf(command, sizeof(command), "i2cset -y 4 0x%02x 0x00 0x%02x", TCA9548A_ADDR, 1 << i);

        if (system(command) != 0)
        {
            continue;
        }

        char detect_command[128];
        snprintf(detect_command, sizeof(detect_command), "i2cdetect -y 4 | grep -q '57'");

        if (system(detect_command) == 0)
        {
            enable_channels[count_channel++] = i;
        }
        else
        {
            std::cerr << "No device found at address 0x57 on channel " << std::endl;
            continue;
        }
    }
    for (int i = 0; i < count_channel; i++)
    {
        std::cout << "channel : " << enable_channels[i] << std::endl;
    }
}

// 初始化I2C设备
int init_i2c(const char *device, int addr)
{
    int fd = open(device, O_RDWR);
    if (fd == -1)
    {
        perror("Failed to open I2C device");
        return -1;
    }

    if (ioctl(fd, I2C_SLAVE, addr) < 0)
    {
        perror("Failed to set I2C address");
        close(fd);
        return -1;
    }

    return fd;
}

void writeRegister(uint8_t fd, uint8_t reg, uint8_t add)
{
    uint8_t config[2] = {reg, add};
    write(fd, config, 2);
}

void max30102_init(int fd, enum Mode mode)
{

    writeRegister(fd, REG_MODE_CONFIG, 0x40);

    writeRegister(fd, REG_FIFO_WR_PTR, 0x00);
    writeRegister(fd, REG_OVF_COUNTER, 0x00);

    writeRegister(fd, REG_FIFO_RD_PTR, 0x00);
    writeRegister(fd, REG_INTR_ENABLE_1, 0xE0);
    writeRegister(fd, REG_INTR_ENABLE_2, 0x00);

    writeRegister(fd, REG_FIFO_CONFIG, 0x0F);
    writeRegister(fd, REG_MODE_CONFIG, 0x03);
    writeRegister(fd, REG_SPO2_CONFIG, 0x27);
    writeRegister(fd, REG_LED1_PA, 0x24);
    writeRegister(fd, REG_LED2_PA, 0x24);
    writeRegister(fd, REG_PILOT_PA, 0x7F);
}

void read_fifo(uint32_t *red_led, uint32_t *ir_led, int fd)
{
    uint8_t reg = REG_FIFO_DATA;
    uint8_t reg_data[6];

    if (write(fd, &reg, 1) != 1)
    {

        return;
    }

    if (read(fd, reg_data, 6) != 6)
    {
        return;
    }

    *red_led = ((reg_data[0] & 0x03) << 16) | (reg_data[1] << 8) | reg_data[2];
    *ir_led = ((reg_data[3] & 0x03) << 16) | (reg_data[4] << 8) | reg_data[5];
}

void finger_near_read(int fd, int channel, int count)
{
    uint32_t ir_data, red_data;
    read_fifo(&red_data, &ir_data, fd);

    printf("Red LED: %u, IR LED: %u\n", red_data, ir_data);
}

int main()
{
    const char *device = "/dev/i2c-4"; // 指定I2C设备接口
    int i2c_fd = init_i2c(device, TCA9548A_ADDR);
    if (i2c_fd == -1)
    {
        return -1;
    }
    scanf_channel();
    for (int i = 0; i < count_channel; i++)
    {
        int channel = i;
        int use = 1 << enable_channels[i];
        int check = write(i2c_fd, &use, 1);
        if (check != 1)
            continue;

        int max30102_fd = init_i2c(device, MAX30102_ADDR);
        if (max30102_fd == -1)
        {
            perror("Failed to setup MAX30102");
            continue;
        }

        max30102_init(max30102_fd, current_mode);
    }
    while (1)
    {
        for (int i = 0; i < count_channel; i++)
        {
            int channel = i;
            int use = 1 << enable_channels[i];
            int check = write(i2c_fd, &use, 1);
            if (check != 1)
                continue;

            int max30102_fd = init_i2c(device, MAX30102_ADDR);
            if (max30102_fd == -1)
            {
                perror("Failed to setup MAX30102");
                continue;
            }

            for (int j = 0; j < 50; ++j)
            {
                finger_near_read(max30102_fd, channel, j);
                usleep(5000);
            }
        }
    }

    close(i2c_fd);
    return 0;
}