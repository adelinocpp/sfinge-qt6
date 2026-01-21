#ifndef IMAGE_H
#define IMAGE_H

#include <vector>
#include <string>
#include <cstdint>
#include <algorithm>
#include <cmath>

namespace SFinGe {

class Image {
public:
    Image();
    Image(int width, int height);
    Image(const Image& other);
    Image& operator=(const Image& other);
    
    int width() const { return m_width; }
    int height() const { return m_height; }
    bool isNull() const { return m_data.empty(); }
    
    void fill(uint8_t gray);
    void setPixel(int x, int y, uint8_t gray);
    uint8_t pixel(int x, int y) const;
    
    void setPixelRGB(int x, int y, uint8_t r, uint8_t g, uint8_t b);
    void getPixelRGB(int x, int y, uint8_t& r, uint8_t& g, uint8_t& b) const;
    
    Image copy() const;
    Image scaled(int newWidth, int newHeight) const;
    
    bool save(const std::string& filename) const;
    
    uint8_t* data() { return m_data.data(); }
    const uint8_t* data() const { return m_data.data(); }
    size_t dataSize() const { return m_data.size(); }
    
    void setDPI(int dpi) { m_dpi = dpi; }
    int getDPI() const { return m_dpi; }

private:
    int m_width;
    int m_height;
    int m_channels; // 1 for grayscale, 3 for RGB
    int m_dpi;
    std::vector<uint8_t> m_data;
};

} // namespace SFinGe

#endif // IMAGE_H
