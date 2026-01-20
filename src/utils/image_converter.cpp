#include "image_converter.h"
#include <algorithm>

namespace SFinGe {

QImage ImageConverter::floatArrayToGrayscale(const std::vector<float>& data, int width, int height) {
    QImage image(width, height, QImage::Format_Grayscale8);
    
    for (int j = 0; j < height; ++j) {
        for (int i = 0; i < width; ++i) {
            int gray = static_cast<int>(data[j * width + i] * 255.0f);
            gray = std::clamp(gray, 0, 255);
            image.setPixel(i, j, qRgb(gray, gray, gray));
        }
    }
    
    return image;
}

QImage ImageConverter::doubleArrayToGrayscale(const std::vector<double>& data, int width, int height) {
    QImage image(width, height, QImage::Format_Grayscale8);
    
    for (int j = 0; j < height; ++j) {
        for (int i = 0; i < width; ++i) {
            int gray = static_cast<int>(data[j * width + i] * 255.0);
            gray = std::clamp(gray, 0, 255);
            image.setPixel(i, j, qRgb(gray, gray, gray));
        }
    }
    
    return image;
}

std::vector<float> ImageConverter::grayscaleToFloatArray(const QImage& image) {
    int width = image.width();
    int height = image.height();
    std::vector<float> data(width * height);
    
    for (int j = 0; j < height; ++j) {
        for (int i = 0; i < width; ++i) {
            QRgb pixel = image.pixel(i, j);
            data[j * width + i] = qGray(pixel) / 255.0f;
        }
    }
    
    return data;
}

QImage ImageConverter::normalizeImage(const QImage& image) {
    if (image.isNull()) {
        return image;
    }
    
    QImage normalized = image.convertToFormat(QImage::Format_RGB888);
    return normalized;
}

}
