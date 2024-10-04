#ifndef QRCODE_H
#define QRCODE_H


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

using namespace std;

// 生成随机数字
string generate_random_numbers(int count)
{
    random_device rd;
    mt19937 gen(rd());
    uniform_int_distribution<> dis(0, 9);
    string numbers;
    for (int i = 0; i < count; ++i)
    {
        numbers += to_string(dis(gen));
    }
    return numbers;
}

// 生成随机字母
string generate_random_letters(int count)
{
    random_device rd;
    mt19937 gen(rd());
    uniform_int_distribution<> dis(0, 51);
    string letters;
    const string alphabet = "ABCDEFGHIJKLMNOPQRSTUVWXYZ";
    for (int i = 0; i < count; ++i)
    {
        letters += alphabet[dis(gen)];
    }
    return letters;
}

// 组合数字和字母并打乱顺序
string combine_num_letters(int num_count, int letter_count)
{
    string numbers = generate_random_numbers(num_count);
    string letters = generate_random_letters(letter_count);
    string combined = numbers + letters;
    // 打乱顺序
    vector<char> chars(combined.begin(), combined.end());
    shuffle(chars.begin(), chars.end(), mt19937(random_device()()));
    return string(chars.begin(), chars.end());
}

// 获取设备序列号
string get_device_serial()
{
    ifstream cpuinfo("/proc/cpuinfo");
    if (!cpuinfo.is_open())
    {
        return "000000000";
    }
    string line;
    while (getline(cpuinfo, line))
    {
        if (line.find("Serial") == 0)
        {
            size_t pos = line.find(":");
            if (pos != string::npos)
            {
                string serial = line.substr(pos + 1);
                // 去除空格和换行符
                serial.erase(remove_if(serial.begin(), serial.end(), ::isspace), serial.end());
                return serial;
            }
        }
    }
    return "000000000";
}

// 获取当前时间，格式为 YYYY-MM-DD-HH-MM-SS
std::string get_current_time() {
    time_t now = time(0);
    tm* ltm = localtime(&now);
    char time_str[50];
    // 使用 snprintf 替代 sprintf，确保不会超过缓冲区大小
    snprintf(time_str, sizeof(time_str), "%04d-%02d-%02d-%02d-%02d",
             1900 + ltm->tm_year,
             1 + ltm->tm_mon,
             ltm->tm_mday,
             ltm->tm_hour,
             ltm->tm_min,
             ltm->tm_sec);
    return string(time_str);
}

// 保存二维码为PNG
bool save_qr_png(const char *filename, QRcode *qrcode)
{
    if (!qrcode)
        return false;

    int size = qrcode->width;
    int border = 4; // 边框大小
    int scale = 10; // 每个模块的像素大小
    int img_size = (size + 2 * border) * scale;

    // 创建白色背景
    vector<unsigned char> image(img_size * img_size, 255);

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
                        if (px < img_size && py < img_size)
                        {
                            image[px * img_size + py] = 0; // 黑色
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
        cerr << "无法打开文件 " << filename << " 进行写入。" << endl;
        return false;
    }

    png_structp png_ptr = png_create_write_struct(PNG_LIBPNG_VER_STRING, NULL, NULL, NULL);
    if (!png_ptr)
    {
        cerr << "创建 PNG 写入结构失败。" << endl;
        fclose(fp);
        return false;
    }

    png_infop info_ptr = png_create_info_struct(png_ptr);
    if (!info_ptr)
    {
        cerr << "创建 PNG 信息结构失败。" << endl;
        png_destroy_write_struct(&png_ptr, NULL);
        fclose(fp);
        return false;
    }

    if (setjmp(png_jmpbuf(png_ptr)))
    {
        cerr << "PNG 写入过程中出现错误。" << endl;
        png_destroy_write_struct(&png_ptr, &info_ptr);
        fclose(fp);
        return false;
    }

    png_init_io(png_ptr, fp);

    // 设置头信息
    png_set_IHDR(
        png_ptr,
        info_ptr,
        img_size,
        img_size,
        8, // 位深度
        PNG_COLOR_TYPE_GRAY,
        PNG_INTERLACE_NONE,
        PNG_COMPRESSION_TYPE_DEFAULT,
        PNG_FILTER_TYPE_DEFAULT);

    png_write_info(png_ptr, info_ptr);

    // 写入图像数据
    for (int y = 0; y < img_size; y++)
    {
        png_bytep row = (png_bytep)&image[y * img_size];
        png_write_row(png_ptr, row);
    }

    png_write_end(png_ptr, info_ptr);
    png_destroy_write_struct(&png_ptr, &info_ptr);
    fclose(fp);

    return true;
}

// size_t Send_Writebacked(void *contents, size_t size, size_t nmemb, void *userp)
// {
//     size_t totalSize = size * nmemb;
//     std::string *str = static_cast<std::string *>(userp);
//     if (str)
//     {
//         str->append(static_cast<char *>(contents), totalSize);
//         return totalSize;
//     }
//     return 0;
// }
size_t WriteCallback(void *contents, size_t size, size_t nmemb, void *userp)
{
    ((std::string *)userp)->append((char *)contents, size * nmemb);
    return size * nmemb;
}

string Generate_and_send_user_message()
{

    string current_time = get_current_time();

    string user_message = "";

    string device_serial = get_device_serial();

    string numbers_and_letters = combine_num_letters(3, 3);
    string sample_id = device_serial + "-" + current_time + "-" + numbers_and_letters;

    cout << "Generated sample_id: " << sample_id << endl;

    // send sample_id
    CURL *curl;
    CURLcode res;
    std::string readBuffer;

    curl = curl_easy_init();
    if (curl)
    {
        std::string base_url = "http://sp.grifcc.top:8080/collect/get_user";

        char *encoded_data = curl_easy_escape(curl, sample_id.c_str(), sample_id.length());
        if (encoded_data)
        {
            std::string url = base_url + "?data=" + encoded_data;

            curl_free(encoded_data);

            curl_easy_setopt(curl, CURLOPT_URL, url.c_str());

            curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, WriteCallback);
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

    // create QRcode
    QRcode *qrcode = QRcode_encodeString(sample_id.c_str(), 0, QR_ECLEVEL_L, QR_MODE_8, 1);
    if (!qrcode)
    {
        cerr << "二维码生成失败。" << endl;
    }

    const char *qr_filename = "QRcode.png";
    if (!save_qr_png(qr_filename, qrcode))
    {
        cerr << "二维码 PNG 保存失败。" << endl;
        QRcode_free(qrcode);
    }
    QRcode_free(qrcode);

    string open_cmd = "feh " + string(qr_filename) + " &";
    int ret = system(open_cmd.c_str());
    if (ret != 0)
    {
        cerr << "无法打开二维码图片。请确保已安装 feh。" << endl;
    }

    user_message = readBuffer + "-" + sample_id;

    return user_message;
}

#endif
