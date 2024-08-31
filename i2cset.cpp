#include <iostream>
#include <fcntl.h>
#include <unistd.h>
#include <cstring>
#include <cstdio>
#include <cstdlib>

#define I2C_BUS "/dev/i2c-4"  // I2C 总线设备文件
#define TCA9548A_ADDRESS 0x70 // TCA9548A 的 I2C 地址
#define DEVICE_ADDRESS 0x57   // MAX30102 的 I2C 地址

int channels[8], count_channel = 0;

bool checkDeviceOnChannel(int channel)
{
    // 使用 i2cset 命令设置 TCA9548A 的通道
    char command[128];
    snprintf(command, sizeof(command), "i2cset -y 4 0x70 0x00 0x%02x", 1 << channel);

    if (system(command) != 0)
    {
        std::cerr << "Failed to set channel " << channel << std::endl;
        return false;
    }

    // 打开 I2C 总线
    int i2c_fd = open(I2C_BUS, O_RDWR);
    if (i2c_fd < 0)
    {
        std::cerr << "Failed to open the I2C bus" << std::endl;
        return false;
    }

    // 设置目标设备地址
    if (ioctl(i2c_fd, I2C_SLAVE, DEVICE_ADDRESS) < 0)
    {
        std::cerr << "Failed to access device at address 0x57 on channel " << channel << std::endl;
        close(i2c_fd);
        return false;
    }

    // 尝试读取设备数据来检测是否存在
    uint8_t buf;
    if (read(i2c_fd, &buf, 1) >= 0)
    {
        std::cout << "Device found at address 0x57 on channel " << channel << std::endl;
        close(i2c_fd);
        return true;
    }

    close(i2c_fd);
    return false;
}

int main()
{
    for (int channel = 0; channel < 8; ++channel)
    {
        std::cout << "Scanning channel " << channel << "..." << std::endl;
        if (checkDeviceOnChannel(channel))
        {
            channels[count_channel++] = channel;
        }
    }

    std::cout << "Detected channels: ";
    for (int i = 0; i < count_channel; ++i)
    {
        std::cout << channels[i] << " ";
    }
    std::cout << std::endl;

    return 0;
}
