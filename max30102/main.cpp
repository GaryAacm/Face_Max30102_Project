#include <iostream>
#include "max30102.h"

int main()
{
    const char *i2c_device = "/dev/i2c-4";
    MAX30102 max30102(i2c_device);
    max30102.get_data();
    return 0;
}
