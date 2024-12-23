#include <iostream>
#include <fcntl.h>
#include <unistd.h>
#include <cstring>
#include <cstdio>
#include <cstdlib>
#include <sys/ioctl.h>
#include <linux/i2c-dev.h>

#define I2C_BUS "/dev/i2c-4"  // I2C 总线设备文件
#define TCA9548A_ADDRESS 0x70 // TCA9548A 的 I2C 地址
#define DEVICE_ADDRESS 0x57   // MAX30102 的 I2C 地址

int channels[8], count_channel = 0;

// 检查指定通道上是否有设备存在
bool checkDeviceOnChannel(int channel)
{
    // 使用 i2cset 命令设置 TCA9548A 的通道
    char command[128];
    snprintf(command, sizeof(command), "i2cset -y 4 0x70 0x00 0x%02x", 1 << channel);

    // 执行 i2cset 命令
    if (system(command) != 0)
    {
        std::cerr << "Failed to set channel " << channel << std::endl;
        return false;
    }

    // 使用 i2cdetect 命令检测设备是否存在
    char detect_command[128];
    snprintf(detect_command, sizeof(detect_command), "i2cdetect -y 4 | grep -q '57'");

    // 执行 i2cdetect 命令并检查结果
    if (system(detect_command) == 0)
    {
        std::cout << "Device found at address 0x57 on channel " << channel << std::endl;
        return true;
    }
    else
    {
        std::cerr << "No device found at address 0x57 on channel " << channel << std::endl;
        return false;
    }
}

int main()
{
    // 扫描所有通道
    for (int channel = 0; channel < 8; ++channel)
    {
        std::cout << "Scanning channel " << channel << "..." << std::endl;
        if (checkDeviceOnChannel(channel))
        {
            channels[count_channel++] = channel; // 存储检测到的通道
        }
    }

    // 打印检测到的通道
    std::cout << "Detected channels: ";
    for (int i = 0; i < count_channel; ++i)
    {
        std::cout << channels[i] << " ";
    }
    std::cout << std::endl;

    return 0;
}
