#include "max30102.h"

MAX30102::MAX30102(const char *device, uint8_t tcaAddress, uint8_t maxAddress)
    : device(device), tcaAddress(tcaAddress), maxAddress(maxAddress), currentMode(HRM_SPO2)
{
    fd = open(device, O_RDWR);
    if (fd == -1)
        perror("Failed to open I2C device");

    if (ioctl(fd, I2C_SLAVE, tcaAddress) < 0)
    {
        perror("Failed to set I2C address");
        close(fd);
    }
    for (int i = 0; i < 8; i++)
    {
        int use_channel = 1 << i;
        if (write(fd, &use_channel, 1) != 1)
        {
            perror("Failed to select channel");
            close(fd);
            fd = open(device, O_RDWR);
            if (fd == -1)
                perror("Failed to open I2C device");
            ioctl(fd, I2C_SLAVE, tcaAddress);
            continue;
        }
        else if (ioctl(fd, I2C_SLAVE, maxAddress) < 0)
        {
            continue;
        }
        else
        {
            enable_channels[enable_channel_num++] = i;
            max30102_init(HRM_SPO2);
        }
    }
}

MAX30102::~MAX30102()
{
    close(fd);
}

void MAX30102::max30102_init(enum Mode mode)
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
    writeRegister(fd, REG_RED_LED, 0x24);
    writeRegister(fd, REG_IR_LED, 0x24);
    writeRegister(fd, REG_PILOT_PA, 0x7F);

    // if (mode == PROXIMITY)
    // {
    //     printf("Initializing in PROXIMITY mode\n");

    //     writeRegister(fd, REG_RED_LED, 0x00);HRM_SPO2
    //     writeRegister(fd, REG_IR_LED, 0x19);
    //     writeRegister(fd, REG_SPO2_CONFIG, 0x43);
    //     writeRegister(fd, REG_FIFO_CONFIG, 0x00);
    //     writeRegister(fd, REG_MULTI_LED_CTRL1, 0x12);
    //     writeRegister(fd, REG_MULTI_LED_CTRL2, 0x00);

    //     writeRegister(fd, REG_FIFO_WR_PTR, 0x00);
    //     writeRegister(fd, REG_OVF_COUNTER, 0x00);

    //     writeRegister(fd, REG_FIFO_RD_PTR, 0x00);
    //     writeRegister(fd, REG_MODE_CONFIG, 0x07);
    //     writeRegister(fd, REG_INTR_ENABLE_1, 0x40);
    // }
    // else
    {
        // printf("Initializing in HRM_SPO2 mode\n");

        // writeRegister(fd, REG_MODE_CONFIG, 0x40);
        // writeRegister(fd, REG_FIFO_WR_PTR, 0x00);
        // writeRegister(fd, REG_OVF_COUNTER, 0x00);

        // writeRegister(fd, REG_FIFO_RD_PTR, 0x00);
        // writeRegister(fd, REG_INTR_ENABLE_1, 0xE0);
        // writeRegister(fd, REG_INTR_ENABLE_2, 0x00);

        // writeRegister(fd, REG_FIFO_CONFIG, 0x0F);
        // writeRegister(fd, REG_MODE_CONFIG, 0x03);
        // writeRegister(fd, REG_SPO2_CONFIG, 0x27);
        // writeRegister(fd, REG_RED_LED, 0x24);
        // writeRegister(fd, REG_IR_LED, 0x24);
        // writeRegister(fd, REG_PILOT_PA, 0x7F);
    }
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

    *red_led = ((reg_data[0] & 0x03) << 16) | (reg_data[1] << 8) | reg_data[2];
    *ir_led = ((reg_data[3] & 0x03) << 16) | (reg_data[4] << 8) | reg_data[5];
}

void MAX30102::writeRegister(int max_fd, uint8_t reg, uint8_t add)
{
    uint8_t config[2] = {reg, add};
    write(max_fd, config, 2);
}

void MAX30102::PreJob()
{
    now_use_channel = enable_channels[count_channel++];
    if (count_channel == enable_channel_num)
        count_channel = 0;

    int mask_channel = 1 << now_use_channel;

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

    // if (currentMode == HRM_SPO2)
    {
        // *ir_data = ir_temp;
        // *red_data = red_temp;
        // printf("Red LED: %u, IR LED: %u\n", red_temp, ir_temp);
    }
    // if (ir_temp > 1500 && currentMode == PROXIMITY)
    // {
    //     max30102_init(HRM_SPO2);
    //     currentMode = HRM_SPO2;
    // }
    // else if (ir_temp < 1500 && currentMode == HRM_SPO2)
    // {
    //     max30102_init(PROXIMITY);
    //     currentMode = PROXIMITY;
    // }
    usleep(1000);
}
