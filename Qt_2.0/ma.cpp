#include <iostream>
#include <ctime>
#include <qrencode.h>
#include <png.h>

void save_png(QRcode *qrcode, const char *filename) {
    int size = qrcode->width;
    int margin = 4; // Margin around the QR code
    int dpi = 72;
    int real_size = size + margin * 2;

    FILE *fp = fopen(filename, "wb");
    if (!fp) {
        std::cerr << "Failed to open file for writing: " << filename << std::endl;
        return;
    }

    png_structp png = png_create_write_struct(PNG_LIBPNG_VER_STRING, nullptr, nullptr, nullptr);
    if (!png) {
        std::cerr << "Failed to create PNG write struct" << std::endl;
        fclose(fp);
        return;
    }

    png_infop info = png_create_info_struct(png);
    if (!info) {
        std::cerr << "Failed to create PNG info struct" << std::endl;
        png_destroy_write_struct(&png, nullptr);
        fclose(fp);
        return;
    }

    if (setjmp(png_jmpbuf(png))) {
        std::cerr << "Failed during PNG creation" << std::endl;
        png_destroy_write_struct(&png, &info);
        fclose(fp);
        return;
    }

    png_init_io(png, fp);
    png_set_IHDR(png, info, real_size, real_size, 1, PNG_COLOR_TYPE_GRAY, PNG_INTERLACE_NONE,
                 PNG_COMPRESSION_TYPE_DEFAULT, PNG_FILTER_TYPE_DEFAULT);

    png_write_info(png, info);

    // Write the image
    for (int y = 0; y < real_size; y++) {
        png_bytep row = (png_bytep)malloc(real_size);
        for (int x = 0; x < real_size; x++) {
            if (x < margin || y < margin || x >= real_size - margin || y >= real_size - margin) {
                row[x] = 0xFF; // White margin
            } else {
                row[x] = (qrcode->data[(y - margin) * size + (x - margin)] & 1) ? 0 : 0xFF; // QR code data
            }
        }
        png_write_row(png, row);
        free(row);
    }

    png_write_end(png, nullptr);
    png_destroy_write_struct(&png, &info);
    fclose(fp);

    std::cout << "QR code saved as " << filename << std::endl;
}

int main() {
    // 获取当前时间并格式化为字符串
    std::time_t t = std::time(nullptr);
    char current_time[100];
    std::strftime(current_time, sizeof(current_time), "%Y-%m-%d %H:%M:%S", std::localtime(&t));

    // 生成二维码
    QRcode *qrcode = QRcode_encodeString(current_time, 1, QR_ECLEVEL_L, QR_MODE_8, 1);
    if (!qrcode) {
        std::cerr << "Failed to generate QR code" << std::endl;
        return 1;
    }

    // 保存二维码为PNG文件
    save_png(qrcode, "qrcode_time.png");

    // 释放内存
    QRcode_free(qrcode);

    return 0;
}
