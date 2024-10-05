#ifndef QRCODEGENERATOR_H
#define QRCODEGENERATOR_H

#include <iostream>
#include <fstream>
#include <sstream>
#include <string>
#include <ctime>
#include <random>
#include <algorithm>
#include <vector>
#include <cstdlib>
#include <cstring>
#include <qrencode.h>
#include <png.h>
#include <curl/curl.h>

class QRCodeGenerator {
public:
    QRCodeGenerator();
    ~QRCodeGenerator();

    // 生成并发送用户消息，返回用户消息字符串
    std::string generateAndSendUserMessage();

private:
    // 生成随机数字
    std::string generateRandomNumbers(int count);

    // 生成随机字母
    std::string generateRandomLetters(int count);

    // 组合数字和字母并打乱顺序
    std::string combineNumLetters(int numCount, int letterCount);

    // 获取设备序列号
    std::string getDeviceSerial();

    // 获取当前时间，格式为 YYYY-MM-DD-HH-MM-SS
    std::string getCurrentTime();

    // 保存二维码为PNG
    bool saveQrPng(const char* filename, QRcode* qrcode);

    // 回调函数，用于处理curl写入的数据
    static size_t writeCallbacks(void* contents, size_t size, size_t nmemb, void* userp);

    // 打开二维码图片的命令
    void openQrImage(const std::string& filename);
};

#endif // QRCODEGENERATOR_H
