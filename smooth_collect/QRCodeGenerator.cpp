#include "QRCodeGenerator.h"
#include <cstdio>
using namespace std;

QRCodeGenerator::QRCodeGenerator()
{
    // 初始化libcurl
    curl_global_init(CURL_GLOBAL_DEFAULT);
}

QRCodeGenerator::~QRCodeGenerator()
{
    // 清理libcurl
    curl_global_cleanup();
}

std::string QRCodeGenerator::generateRandomNumbers(int count)
{
    std::random_device rd;
    std::mt19937 gen(rd());
    std::uniform_int_distribution<> dis(0, 9);
    std::string numbers;
    numbers.reserve(count);
    for (int i = 0; i < count; ++i)
    {
        numbers += std::to_string(dis(gen));
    }
    return numbers;
}

std::string QRCodeGenerator::generateRandomLetters(int count)
{
    std::random_device rd;
    std::mt19937 gen(rd());
    std::uniform_int_distribution<> dis(0, 25); // 修改为0-25，匹配26个字母
    std::string letters;
    letters.reserve(count);
    const std::string alphabet = "ABCDEFGHIJKLMNOPQRSTUVWXYZ";
    for (int i = 0; i < count; ++i)
    {
        letters += alphabet[dis(gen)];
    }
    return letters;
}

std::string QRCodeGenerator::combineNumLetters(int numCount, int letterCount)
{
    std::string numbers = generateRandomNumbers(numCount);
    std::string letters = generateRandomLetters(letterCount);
    std::string combined = numbers + letters;

    // 打乱顺序
    std::vector<char> chars(combined.begin(), combined.end());
    std::mt19937 gen(std::random_device{}());
    std::shuffle(chars.begin(), chars.end(), gen);
    return std::string(chars.begin(), chars.end());
}

std::string QRCodeGenerator::getDeviceSerial()
{
    std::ifstream cpuinfo("/proc/cpuinfo");
    if (!cpuinfo.is_open())
    {
        return "000000000";
    }
    std::string line;
    while (std::getline(cpuinfo, line))
    {
        if (line.find("Serial") == 0)
        {
            size_t pos = line.find(":");
            if (pos != std::string::npos)
            {
                std::string serial = line.substr(pos + 1);
                // 去除空格和换行符
                serial.erase(std::remove_if(serial.begin(), serial.end(), ::isspace), serial.end());
                return serial;
            }
        }
    }
    return "000000000";
}

std::string QRCodeGenerator::getCurrentTime()
{
    std::time_t now = std::time(nullptr);
    std::tm *ltm = std::localtime(&now);
    char timeStr[100];
    std::snprintf(timeStr, sizeof(timeStr), "%04d-%02d-%02d-%02d-%02d-%02d",
                  1900 + ltm->tm_year,
                  1 + ltm->tm_mon,
                  ltm->tm_mday,
                  ltm->tm_hour,
                  ltm->tm_min,
                  ltm->tm_sec);
    return std::string(timeStr);
}

bool QRCodeGenerator::saveQrPng(const char *filename, QRcode *qrcode)
{
    if (!qrcode)
        return false;

    int size = qrcode->width;
    int border = 4; // 边框大小
    int scale = 10; // 每个模块的像素大小
    int imgSize = (size + 2 * border) * scale;

    // 创建白色背景
    std::vector<unsigned char> image(imgSize * imgSize, 255);

    for (int y = 0; y < size; y++)
    {
        for (int x = 0; x < size; x++)
        {
            if (qrcode->data[y * size + x] & 1)
            {
                for (int dy = 0; dy < scale; dy++)
                {
                    for (int dx = 0; dx < scale; dx++)
                    {
                        int px = (y + border) * scale + dy;
                        int py = (x + border) * scale + dx;
                        if (px < imgSize && py < imgSize)
                        {
                            image[px * imgSize + py] = 0; // 黑色
                        }
                    }
                }
            }
        }
    }

    // 使用 libpng 保存图片
    FILE *fp = fopen(filename, "wb");
    if (!fp)
    {
        std::cerr << "无法打开文件 " << filename << " 进行写入。" << std::endl;
        return false;
    }

    png_structp png_ptr = png_create_write_struct(PNG_LIBPNG_VER_STRING, NULL, NULL, NULL);
    if (!png_ptr)
    {
        std::cerr << "创建 PNG 写入结构失败。" << std::endl;
        fclose(fp);
        return false;
    }

    png_infop info_ptr = png_create_info_struct(png_ptr);
    if (!info_ptr)
    {
        std::cerr << "创建 PNG 信息结构失败。" << std::endl;
        png_destroy_write_struct(&png_ptr, NULL);
        fclose(fp);
        return false;
    }

    if (setjmp(png_jmpbuf(png_ptr)))
    {
        std::cerr << "PNG 写入过程中出现错误。" << std::endl;
        png_destroy_write_struct(&png_ptr, &info_ptr);
        fclose(fp);
        return false;
    }

    png_init_io(png_ptr, fp);

    // 设置头信息
    png_set_IHDR(
        png_ptr,
        info_ptr,
        imgSize,
        imgSize,
        8,
        PNG_COLOR_TYPE_GRAY,
        PNG_INTERLACE_NONE,
        PNG_COMPRESSION_TYPE_DEFAULT,
        PNG_FILTER_TYPE_DEFAULT);

    png_write_info(png_ptr, info_ptr);

    // 写入图像数据
    for (int y = 0; y < imgSize; y++)
    {
        png_bytep row = (png_bytep)&image[y * imgSize];
        png_write_row(png_ptr, row);
    }

    png_write_end(png_ptr, info_ptr);
    png_destroy_write_struct(&png_ptr, &info_ptr);
    fclose(fp);

    return true;
}

size_t QRCodeGenerator::writeCallbacks(void *contents, size_t size, size_t nmemb, void *userp)
{
    ((std::string *)userp)->append((char *)contents, size * nmemb);
    return size * nmemb;
}

void QRCodeGenerator::openQrImage(const std::string &filename)
{
    std::string openCmd = "feh " + filename + " &";
    int ret = std::system(openCmd.c_str());
    if (ret != 0)
    {
        std::cerr << "无法打开二维码图片。请确保已安装 feh。" << std::endl;
    }
}

std::string QRCodeGenerator::generateAndSendUserMessage()
{
    std::string currentTime = getCurrentTime();
    std::string deviceSerial = getDeviceSerial();
    std::string numbersAndLetters = combineNumLetters(3, 3);
    std::string sampleId = deviceSerial + "-" + currentTime + "-" + numbersAndLetters;

    std::cout << "Generated sample_id: " << sampleId << std::endl;

    // 发送 sample_id
    CURL *curl = curl_easy_init();
    CURLcode res;
    std::string readBuffer;

    if (curl)
    {
        const std::string baseUrl = "http://sp.grifcc.top:8080/collect/get_user";
        cout << "Success in sending message" << endl;

        char *encodedData = curl_easy_escape(curl, sampleId.c_str(), sampleId.length());
        if (encodedData)
        {
            std::string url = baseUrl + "?data=" + encodedData;
            curl_free(encodedData);

            curl_easy_setopt(curl, CURLOPT_URL, url.c_str());
            curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, QRCodeGenerator::writeCallbacks);
            curl_easy_setopt(curl, CURLOPT_WRITEDATA, &readBuffer);

            res = curl_easy_perform(curl);

            if (res != CURLE_OK)
            {
                std::cerr << "curl_easy_perform() 失败: " << curl_easy_strerror(res) << std::endl;
            }
            else
            {
                std::cout << "服务器返回的数据: " << readBuffer << std::endl;
            }
        }
        else
        {
            std::cerr << "URL 编码失败" << std::endl;
        }
        curl_easy_cleanup(curl);
    }
    else
    {
        std::cerr << "初始化 libcurl 失败" << std::endl;
    }

    // 生成二维码
    QRcode *qrcode = QRcode_encodeString(sampleId.c_str(), 0, QR_ECLEVEL_L, QR_MODE_8, 1);
    if (!qrcode)
    {
        std::cerr << "二维码生成失败。" << std::endl;
        return "";
    }

    const char *qrFilename = "QRCode.png";
    if (!saveQrPng(qrFilename, qrcode))
    {
        std::cerr << "二维码 PNG 保存失败。" << std::endl;
        QRcode_free(qrcode);
        return "";
    }
    QRcode_free(qrcode);

    // 打开二维码图片
    openQrImage(qrFilename);

    // 组合用户消息
    std::string userMessage = readBuffer + "-" + sampleId;
    cout << userMessage << endl;

    FILE *f = fopen("User_Message.txt", "w");
    fprintf(f, "%s", userMessage);
    fclose(f);

    return userMessage;
}
