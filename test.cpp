#include <iostream>
#include <fcntl.h>
#include <sys/ioctl.h>
#include <linux/i2c-dev.h>
#include <unistd.h>
#include <cstring>

#define TCA9548A_ADDRESS 0x70 // TCA9548A 的 I2C 地址
#define I2C_BUS "/dev/i2c-4"  // I2C 总线设备文件

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

int channels[8], count_channel = 0, enable_channel, i2c_fd;

bool writeRegister(uint8_t reg, uint8_t add)
{
    uint8_t config[2] = {reg, add};
    if (write(i2c_fd, config, 2) != 2)
        return false;
    return true;
}

void read_fifo(uint32_t *red_led, uint32_t *ir_led)
{

    uint8_t reg = REG_FIFO_DATA;
    uint8_t reg_data[6];

    if (write(i2c_fd, &reg, 1) != 1)
    {
        perror("Failed to write register address");
        return;
    }

    if (read(i2c_fd, reg_data, 6) != 6)
    {
        perror("Failed to read data");
        return;
    }

    *red_led = ((reg_data[0] & 0x03) << 16) | (reg_data[1] << 8) | reg_data[2];
    *ir_led = ((reg_data[3] & 0x03) << 16) | (reg_data[4] << 8) | reg_data[5];
}

void max30102_init()
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
    writeRegister(REG_RED_LED, 0x24);
    writeRegister(REG_IR_LED, 0x24);
    writeRegister(REG_PILOT_PA, 0x7F);
}

int main()
{
    i2c_fd = open(I2C_BUS, O_RDWR);
    uint32_t red_led;
    uint32_t ir_led;
    if (i2c_fd < 0)
    {
        std::cerr << "Failed to open the I2C bus" << std::endl;
        return -1;
    }
    if (ioctl(i2c_fd, I2C_SLAVE, TCA9548A_ADDRESS) < 0)
    {
        std::cerr << "Failed to acquire bus access to the TCA9548A" << std::endl;
        close(i2c_fd);
        return -1;
    }
    for (int channel = 0; channel < 8; ++channel)
    {
        std::cout << "Scanning channel " << channel << "..." << std::endl;
        uint8_t control = 1 << channel;
        if (write(i2c_fd, &control, 1) != 1)
        {
            close(i2c_fd);
            i2c_fd = open(I2C_BUS, O_RDWR);
            ioctl(i2c_fd, I2C_SLAVE, TCA9548A_ADDRESS);
            continue;
        }
        if (ioctl(i2c_fd, I2C_SLAVE, 0x57) < 0)
        {
            std::cerr << "Failed to access device at address 0x57" << std::hex << std::endl;
            continue;
        }
        uint8_t buf;
        if (read(i2c_fd, &buf, 1) >= 0)
        {
            std::cout << "Device found at address 0x57" << std::hex << " on channel " << channel << std::endl;
            channels[count_channel++] = channel;
            max30102_init();
            
        }
    }
    for (int i = 0; i < count_channel; i++)
    {
        printf("enable_channel is : %d\n", channels[i]);

        uint8_t control = 1 << 1;
        if (write(i2c_fd, &control, 1) != 1)
        {
            close(i2c_fd);
            i2c_fd = open(I2C_BUS, O_RDWR);
            ioctl(i2c_fd, I2C_SLAVE, TCA9548A_ADDRESS);
            continue;
        }
        if (ioctl(i2c_fd, I2C_SLAVE, 0x57) < 0)
        {
            std::cerr << "Failed to access device at address 0x57" << std::hex << std::endl;
            continue;
        }

        for (int j = 0; j < 20; j++)
        {
            read_fifo(&red_led, &ir_led);
            printf("Channel : %d  %d  %d \n", channels[i] ,red_led, ir_led);
            usleep(500);
        }
    }

    return 0;
}
