#include <iostream>
#include <fcntl.h>
#include <sys/ioctl.h>
#include <linux/i2c-dev.h>
#include <unistd.h>
#include <cstring>

#define TCA9548A_ADDRESS 0x77  // TCA9548A 的 I2C 地址
#define I2C_BUS "/dev/i2c-4"    // I2C 总线设备文件

// 在特定的通道上扫描设备
void scanI2CDevices(int i2c_fd, int channel) {
    // 选择 TCA9548A 的通道
    uint8_t control = 1 << channel;
    if (write(i2c_fd, &control, 1) != 1) 
    {
        std::cerr << "Failed to switch to channel " << channel << std::endl;
        return;
    }

    for (int addr = 0x00; addr <= 0x77; ++addr) {
        if (ioctl(i2c_fd, I2C_SLAVE, addr) < 0) {
            std::cerr << "Failed to access device at address 0x" << std::hex << addr << std::endl;
            continue;
        }
        uint8_t buf;
        if (read(i2c_fd, &buf, 1) >= 0) {
            std::cout << "Device found at address 0x" << std::hex << addr << " on channel " << channel << std::endl;
        }
    }
}

int main() {
    // 打开 I2C 总线
    int i2c_fd = open(I2C_BUS, O_RDWR);
    if (i2c_fd < 0) {
        std::cerr << "Failed to open the I2C bus" << std::endl;
        return -1;
    }

    // 设置 TCA9548A 地址
    if (ioctl(i2c_fd, I2C_SLAVE, TCA9548A_ADDRESS) < 0) {
        std::cerr << "Failed to acquire bus access to the TCA9548A" << std::endl;
        close(i2c_fd);
        return -1;
    }

    for (int channel = 0; channel < 8; ++channel) {
        std::cout << "Scanning channel " << channel << "..." << std::endl;
        scanI2CDevices(i2c_fd, channel);
        usleep(100000);  
    }
    close(i2c_fd);
    return 0;
}
