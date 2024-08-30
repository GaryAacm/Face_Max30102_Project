#include "max30102.h"

MAX30102::MAX30102(const char *device, uint8_t tcaAddress, uint8_t maxAddress)
    : device(device), tcaAddress(tcaAddress), maxAddress(maxAddress)
{

    for (int i = 0; i <= 7; i += 1)
    {
        int ues_channel = 1 << i;
        fd = open(device, O_RDWR);

        ioctl(fd, I2C_SLAVE, tcaAddress);
        if (write(fd, &ues_channel, 1) != 1)
        {
            close(fd);
            fd = open(device, O_RDWR);
            if (fd == -1)
                perror("Failed to open i2c");
            else if (ioctl(fd, I2C_SLAVE, tcaAddress) < 0)
            {
                close(fd);
                fd = -1;
            }
            continue;
        }
        if (ioctl(fd, I2C_SLAVE, maxAddress) < 0)
        {
            continue;
        }
        else
        {
            max30102_init();
            enable_channels[count_channel++] = i;
        }
    }
    printf("%d\n", count_channel);
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
    writeRegister(REG_RED_LED, 0x24);
    writeRegister(REG_IR_LED, 0x24);
    writeRegister(REG_PILOT_PA, 0x7F);
}

void MAX30102::writeRegister(uint8_t reg, uint8_t add)
{
    uint8_t config[2] = {reg, add};
    write(fd, config, 2);
}

void MAX30102::read_fifo(uint32_t *red_led, uint32_t *ir_led)
{

    uint8_t reg = REG_FIFO_DATA;
    uint8_t reg_data[6];

    if (write(fd, &reg, 1) != 1)
    {
        perror("Failed to write register address");
        return;
    }

    if (read(fd, reg_data, 6) != 6)
    {
        perror("Failed to read data");
        return;
    }

    *red_led = ((reg_data[0] & 0x03) << 16) | (reg_data[1] << 8) | reg_data[2];
    *ir_led = ((reg_data[3] & 0x03) << 16) | (reg_data[4] << 8) | reg_data[5];
}

bool MAX30102::get_data(uint32_t *red_data, uint32_t *ir_data)
{
    uint32_t red_temp, ir_temp;
    fd = open(device, O_RDWR);

    ioctl(fd, I2C_SLAVE, tcaAddress);

    // scanf channel
    while (1)
    {
        for (int i = 1; i < 4; i += 1)
        {
            // int use_channel = 1 << enable_channels[i];
            int use_channel = 1 << i;
            if (write(fd, &use_channel, 1) != 1)
            {
                perror("Failed to select channel");
                continue;
            }

            if (ioctl(fd, I2C_SLAVE, maxAddress) < 0)
            {
                perror("Failed to set I2C address");
                close(fd);
                continue;
            }
            for (int j = 0; j < 20; j++)
            {
                read_fifo(&red_temp, &ir_temp);
                printf("channel %d - RED : %d - IR : %d \n", enable_channels[i], red_temp, ir_temp);
                *red_data = red_temp;
                *ir_data = ir_temp;
            }
        }
    }
    close(fd);
}
