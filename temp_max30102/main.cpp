#include <iostream>
#include "max30102.h"

int main()
{
    const char *i2c_device = "/dev/i2c-4";
    MAX30102 max30102(i2c_device);
    uint32_t red,ir;
    max30102.get_data(&red,&ir);
    return 0;
}
