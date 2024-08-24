#include "max30102.h"

MAX30102::MAX30102(const char *device,uint8_t tcaAddress,uint8_t maxAddress)
    :device(device),tcaAddress(tcaAddress),maxAddress(maxAddress),currentMode(PROXIMITY) {
        fd = init_i2c(tcaAddress);
    }

MAX30102::~MAX30102(){
    close(fd);
}

int MAX30102::init_i2c(int addr)
{
    int local_fd = open(device, O_RDWR);
    if (local_fd == -1)
    {
        perror("Failed to open I2C device");
        return -1;
    }

    if (ioctl(local_fd, I2C_SLAVE, addr) < 0)
    {
        perror("Failed to set I2C address");
        close(local_fd);
        return -1;
    }

    return local_fd;
}

int MAX30102::select_channel(uint8_t channel)
{
    if (channel > 7)
    {
        fprintf(stderr, "Invalid channel number\n");
        return -1;
    }

    uint8_t command = 1 << channel;

    if (write(fd, &command, 1) != 1)
    {
        perror("Failed to select channel");
        return -1;
    }

    return 0;
}

void MAX30102::max30102_init(enum Mode mode)
{
    max30102_fd=init_i2c(maxAddress);
    if(max30102_fd==-1) 
    {
        perror("Failed to open Max30102!");
        return ;
    }
    uint8_t config[2];
    // 根据模式进行复位设置
    if (mode == PROXIMITY)
    {
        printf("Initializing in PROXIMITY mode\n");

        config[0] = REG_LED1_PA;
        config[1] = 0x00; // RED LED: 0mA
        if (write(max30102_fd, config, 2) != 2)
            perror("Failed to write REG_LED1_PA");

        config[0] = REG_LED2_PA;
        config[1] = 0x19; // IR LED: 5mA
        if (write(max30102_fd, config, 2) != 2)
            perror("Failed to write REG_LED2_PA");

        config[0] = REG_SPO2_CONFIG;
        config[1] = 0x43; // SPO2 CONFIGURATION
        if (write(max30102_fd, config, 2) != 2)
            perror("Failed to write REG_SPO2_CONFIG");

        config[0] = REG_FIFO_CONFIG;
        config[1] = 0x00; // FIFO CONFIG
        if (write(max30102_fd, config, 2) != 2)
            perror("Failed to write REG_FIFO_CONFIG");

        config[0] = REG_MULTI_LED_CTRL1;
        config[1] = 0x12; // Multi-LED 1
        if (write(max30102_fd, config, 2) != 2)
            perror("Failed to write REG_MULTI_LED_CTRL1");

        config[0] = REG_MULTI_LED_CTRL2;
        config[1] = 0x00; // Multi-LED 2
        if (write(max30102_fd, config, 2) != 2)
            perror("Failed to write REG_MULTI_LED_CTRL2");

        config[0] = REG_FIFO_WR_PTR;
        config[1] = 0x00; // FIFO RESET
        if (write(max30102_fd, config, 2) != 2)
            perror("Failed to write REG_FIFO_WR_PTR");

        config[0] = REG_OVF_COUNTER;
        config[1] = 0x00; // FIFO RESET
        if (write(max30102_fd, config, 2) != 2)
            perror("Failed to write REG_OVF_COUNTER");

        config[0] = REG_FIFO_RD_PTR;
        config[1] = 0x00; // FIFO RESET
        if (write(max30102_fd, config, 2) != 2)
            perror("Failed to write REG_FIFO_RD_PTR");

        config[0] = REG_MODE_CONFIG;
        config[1] = 0x07; // Mode Configuration: Multi-LED Mode
        if (write(max30102_fd, config, 2) != 2)
            perror("Failed to write REG_MODE_CONFIG");

        config[0] = REG_INTR_ENABLE_1;
        config[1] = 0x40; // PPG RDY Interrupt Enable
        if (write(max30102_fd, config, 2) != 2)
            perror("Failed to write REG_INTR_ENABLE_1");
    }
    else
    {
        printf("Initializing in HRM_SPO2 mode\n");

        config[0] = REG_MODE_CONFIG;
        config[1] = 0x40;
        if (write(max30102_fd, config, 2) != 2)
        {
            perror("Failed to reset MAX30102");
            return;
        }
        usleep(50000); // 延时50ms等待复位完成

        // 清空指针
        config[0] = REG_FIFO_WR_PTR;
        config[1] = 0x00;
        if (write(max30102_fd, config, 2) != 2)
            perror("Failed to write REG_FIFO_WR_PTR");

        config[0] = REG_OVF_COUNTER;
        config[1] = 0x00;
        if (write(max30102_fd, config, 2) != 2)
            perror("Failed to write REG_OVF_COUNTER");

        config[0] = REG_FIFO_RD_PTR;
        config[1] = 0x00;
        if (write(max30102_fd, config, 2) != 2)
            perror("Failed to write REG_FIFO_RD_PTR");

        // 配置中断
        config[0] = REG_INTR_ENABLE_1;
        config[1] = 0xE0;
        if (write(max30102_fd, config, 2) != 2)
            perror("Failed to write REG_INTR_ENABLE_1");

        config[0] = REG_INTR_ENABLE_2;
        config[1] = 0x00;
        if (write(max30102_fd, config, 2) != 2)
            perror("Failed to write REG_INTR_ENABLE_2");

        // 配置FIFO
        config[0] = REG_FIFO_CONFIG;
        config[1] = 0x0F;
        if (write(max30102_fd, config, 2) != 2)
            perror("Failed to write REG_FIFO_CONFIG");

        // 配置模式
        config[0] = REG_MODE_CONFIG;
        config[1] = 0x03;
        if (write(max30102_fd, config, 2) != 2)
            perror("Failed to write REG_MODE_CONFIG");

        // 配置SPO2
        config[0] = REG_SPO2_CONFIG;
        config[1] = 0x27;
        if (write(max30102_fd, config, 2) != 2)
            perror("Failed to write REG_SPO2_CONFIG");

        // 配置LED
        config[0] = REG_LED1_PA;
        config[1] = 0x24;
        if (write(max30102_fd, config, 2) != 2)
            perror("Failed to write REG_LED1_PA");

        config[0] = REG_LED2_PA;
        config[1] = 0x24;
        if (write(max30102_fd, config, 2) != 2)
            perror("Failed to write REG_LED2_PA");

        config[0] = REG_PILOT_PA;
        config[1] = 0x7F;
        if (write(max30102_fd, config, 2) != 2)
            perror("Failed to write REG_PILOT_PA");
    }
}

void MAX30102::read_fifo(uint32_t *red_led, uint32_t *ir_led, int fd)
{
    
    uint8_t reg = REG_FIFO_DATA;
    uint8_t reg_data[6];

    if (write(max30102_fd, &reg, 1) != 1)
    {
        perror("Failed to write register address");
        return;
    }

    if (read(max30102_fd, reg_data, 6) != 6)
    {
        perror("Failed to read data");
        return;
    }

    *red_led = ((reg_data[0] & 0x03) << 16) | (reg_data[1] << 8) | reg_data[2];
    *ir_led = ((reg_data[3] & 0x03) << 16) | (reg_data[4] << 8) | reg_data[5];
}

void MAX30102::Near_read(int channel,int count,uint32_t *ir_data, uint32_t *red_data)
{
    
    uint32_t red_temp,ir_temp;
    read_fifo(&red_temp, &ir_temp, fd);

    if (currentMode == HRM_SPO2 && count >= 0)
    {
        *ir_data=ir_temp;
        *red_data=red_temp;
        printf("Channel %d - Red LED: %u, IR LED: %u\n", channel, red_temp, ir_temp);
    }
    if (ir_temp > 1500 && currentMode == PROXIMITY) // 检测到数据合理，即指尖靠近
    {
        printf("Finger detected, switching to HRM_SPO2 mode\n");
        max30102_init(HRM_SPO2);
        currentMode = HRM_SPO2;
    }
    else if (ir_temp < 1500 && currentMode == HRM_SPO2) //数据不合理，更换模式
    {
        printf("Finger removed, switching to PROXIMITY mode\n");
        max30102_init(PROXIMITY);
        currentMode = PROXIMITY;
    }
}


