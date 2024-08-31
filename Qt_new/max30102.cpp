#include "max30102.h"
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

MAX30102::MAX30102(const char *device, uint8_t tcaAddress, uint8_t maxAddress)
    : device(device), tcaAddress(tcaAddress), maxAddress(maxAddress)
{
    for (int i = 0; i <= 7; i++)
    {
        // 构建 i2cset 命令以设置通道
        char command[128];
        snprintf(command, sizeof(command), "i2cset -y 4 0x%02x 0x00 0x%02x", tcaAddress, 1 << i);

        // 执行 i2cset 命令
        if (system(command) != 0)
        {
            std::cerr << "Failed to set channel " << i << " using i2cset." << std::endl;
            continue;
        }

        // 打开 I2C 总线
        fd = open(device, O_RDWR);
        if (fd < 0)
        {
            perror("Failed to open the I2C bus");
            continue;
        }

        // 设置 MAX30102 的 I2C 地址
        if (ioctl(fd, I2C_SLAVE, maxAddress) < 0)
        {
            std::cerr << "Failed to access device at address 0x" << std::hex << (int)maxAddress << " on channel " << i << std::endl;
            close(fd);
            continue;
        }

        // 检测设备是否存在
        uint8_t buf;
        if (read(fd, &buf, 1) >= 0)
        {
            std::cout << "Device found at address 0x" << std::hex << (int)maxAddress << " on channel " << i << std::endl;
            enable_channels[count_channel++] = i;
            max30102_init(); // 初始化设备
        }

        close(fd); 
    }

    for (int i = 0; i < count_channel; i++)
    {
        std::cout << "channel : " << enable_channels[i] << std::endl;
    }

    fd = open(device, O_RDWR);
    ioctl(fd, I2C_SLAVE, tcaAddress);
}

MAX30102::~MAX30102()
{
    close(fd);
}

void MAX30102::max30102_init()
{
    writeRegister(REG_MODE_CONFIG, 0x40);
    writeRegister(REG_FIFO_WR_PTR, 0x00);
    writeRegister(REG_OVF_COUNTER, 0x00);

    writeRegister(REG_FIFO_RD_PTR, 0x00);
    writeRegister(REG_INTR_ENABLE_1, 0xE0);
    writeRegister(REG_INTR_ENABLE_2, 0x00);

    writeRegister(REG_FIFO_CONFIG, 0x0F);
    writeRegister(REG_MODE_CONFIG, 0x03);
    writeRegister(REG_SPO2_CONFIG, 0x27);
    writeRegister(REG_RED_LED, 0x27);
    writeRegister(REG_IR_LED, 0x27);
    writeRegister(REG_PILOT_PA, 0x7F);
}

void MAX30102::read_fifo(uint32_t *red_led, uint32_t *ir_led, int fd)
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

    *red_led = (((reg_data[0]) << 16) | (reg_data[1] << 8) | reg_data[2]) & 0x03FFFF;
    *ir_led = (((reg_data[3]) << 16) | (reg_data[4] << 8) | reg_data[5]) & 0x03FFFF;
}

void MAX30102::writeRegister(uint8_t reg, uint8_t add)
{
    uint8_t config[2] = {reg, add};
    write(fd, config, 2);
}

void MAX30102::PreJob()
{
    now_use_channel = enable_channels[count_channel++];
    if (count_channel == enable_channel_num)
        count_channel = 0;

    int mask_channel = 1 << now_use_channel;
    printf("channel : %d\n", now_use_channel);

    write(fd, &mask_channel, 1);
    ioctl(fd, I2C_SLAVE, maxAddress);
}

void MAX30102::DoJob(uint32_t *ir_data, uint32_t *red_data)
{

    uint32_t red_temp, ir_temp;
    read_fifo(&red_temp, &ir_temp, fd);

    *ir_data = ir_temp;
    *red_data = red_temp;
    printf("Red LED: %u, IR LED: %u\n", red_temp, ir_temp);
}
