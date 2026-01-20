#ifndef IMAGE_CONVERTER_H
#define IMAGE_CONVERTER_H

#include <QImage>
#include <vector>

namespace SFinGe {

class ImageConverter {
public:
    static QImage floatArrayToGrayscale(const std::vector<float>& data, int width, int height);
    static QImage doubleArrayToGrayscale(const std::vector<double>& data, int width, int height);
    static std::vector<float> grayscaleToFloatArray(const QImage& image);
    
    static QImage normalizeImage(const QImage& image);
};

}

#endif
