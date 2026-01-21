#include "image.h"

#define STB_IMAGE_WRITE_IMPLEMENTATION
#include "stb/stb_image_write.h"

namespace SFinGe {

Image::Image() : m_width(0), m_height(0), m_channels(1), m_dpi(500) {}

Image::Image(int width, int height) 
    : m_width(width), m_height(height), m_channels(1), m_dpi(500) {
    m_data.resize(width * height, 255);
}

Image::Image(const Image& other) 
    : m_width(other.m_width), m_height(other.m_height), 
      m_channels(other.m_channels), m_dpi(other.m_dpi), m_data(other.m_data) {}

Image& Image::operator=(const Image& other) {
    if (this != &other) {
        m_width = other.m_width;
        m_height = other.m_height;
        m_channels = other.m_channels;
        m_dpi = other.m_dpi;
        m_data = other.m_data;
    }
    return *this;
}

void Image::fill(uint8_t gray) {
    std::fill(m_data.begin(), m_data.end(), gray);
}

void Image::setPixel(int x, int y, uint8_t gray) {
    if (x >= 0 && x < m_width && y >= 0 && y < m_height) {
        m_data[y * m_width + x] = gray;
    }
}

uint8_t Image::pixel(int x, int y) const {
    if (x >= 0 && x < m_width && y >= 0 && y < m_height) {
        return m_data[y * m_width + x];
    }
    return 255;
}

void Image::setPixelRGB(int x, int y, uint8_t r, uint8_t g, uint8_t b) {
    // For grayscale, convert to luminance
    uint8_t gray = static_cast<uint8_t>(0.299 * r + 0.587 * g + 0.114 * b);
    setPixel(x, y, gray);
}

void Image::getPixelRGB(int x, int y, uint8_t& r, uint8_t& g, uint8_t& b) const {
    uint8_t gray = pixel(x, y);
    r = g = b = gray;
}

Image Image::copy() const {
    return Image(*this);
}

Image Image::scaled(int newWidth, int newHeight) const {
    Image result(newWidth, newHeight);
    
    double scaleX = static_cast<double>(m_width) / newWidth;
    double scaleY = static_cast<double>(m_height) / newHeight;
    
    for (int y = 0; y < newHeight; ++y) {
        for (int x = 0; x < newWidth; ++x) {
            int srcX = static_cast<int>(x * scaleX);
            int srcY = static_cast<int>(y * scaleY);
            srcX = std::min(srcX, m_width - 1);
            srcY = std::min(srcY, m_height - 1);
            result.setPixel(x, y, pixel(srcX, srcY));
        }
    }
    
    return result;
}

bool Image::save(const std::string& filename) const {
    if (m_data.empty()) return false;
    
    // Determine format from extension
    std::string ext = filename.substr(filename.find_last_of('.') + 1);
    
    int result = 0;
    if (ext == "png" || ext == "PNG") {
        result = stbi_write_png(filename.c_str(), m_width, m_height, 1, m_data.data(), m_width);
    } else if (ext == "bmp" || ext == "BMP") {
        result = stbi_write_bmp(filename.c_str(), m_width, m_height, 1, m_data.data());
    } else if (ext == "jpg" || ext == "jpeg" || ext == "JPG" || ext == "JPEG") {
        result = stbi_write_jpg(filename.c_str(), m_width, m_height, 1, m_data.data(), 95);
    } else {
        // Default to PNG
        result = stbi_write_png(filename.c_str(), m_width, m_height, 1, m_data.data(), m_width);
    }
    
    return result != 0;
}

} // namespace SFinGe
