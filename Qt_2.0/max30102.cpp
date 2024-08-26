#include "max30102.h"

MAX30102::MAX30102(const char *device, uint8_t tcaAddress, uint8_t maxAddress)
    : device(device), tcaAddress(tcaAddress), maxAddress(maxAddress)
{
    fd = open(device, O_RDWR);
    if (fd == -1)
        perror("Failed to open i2c");
    else if (ioctl(fd, I2C_SLAVE, tcaAddress) < 0)
    {
        close(fd);
        fd = -1;
    }
}

MAX30102::~MAX30102()
{
    close(fd);
}

void MAX30102::max30102_init()
{
    writeRegister(REG_MODE_CONFIG, 0x40);
    usleep(50000);

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

bool MAX30102::get_data(uint32_t *red_led, uint32_t *ir_led)
{
    uint32_t red_temp, ir_temp;

    // scanf channel
    for (int i = 0; i < 8; i++)
    {
        int channel = 1 << i;
        // int check_channel = write(fd, &channel, 1);`
        // printf("check is %d\n",check_channel);
        // if (check_channel != 1)
        if (write(fd, &channel, 1) != 1)
        {
            close(fd);
            fd = open(device, O_RDWR);
            if (fd == -1)
                perror("Failed to open i2c");
            else if (ioctl(fd, I2C_SLAVE, tcaAddress) < 0)
            {
                close(fd);
            }
            continue;
        }
        if (ioctl(fd, I2C_SLAVE, maxAddress) < 0)
        {
            continue;
        }
        //usleep(500000);
        max30102_init();
        for (int j = 0; j < 20; j++)
        {
            read_fifo(&red_temp, &ir_temp);
            usleep(5000);
            printf("RED : %d - IR : %d \n", red_temp, ir_temp);
            *red_led = red_temp;
            *ir_led = ir_temp;
        }
    }
}
