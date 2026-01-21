#ifndef IMAGE_BUFFER_H
#define IMAGE_BUFFER_H

#include <vector>
#include <cstdint>

class ImageBuffer {
public:
    ImageBuffer(int width, int height);
    ~ImageBuffer();
    
    int getWidth() const { return m_width; }
    int getHeight() const { return m_height; }
    uint8_t* getData() { return m_data.data(); }
    const uint8_t* getData() const { return m_data.data(); }
    
    void setPixel(int x, int y, uint8_t value);
    uint8_t getPixel(int x, int y) const;
    
    bool saveToFile(const char* filename) const;
    
private:
    int m_width;
    int m_height;
    std::vector<uint8_t> m_data;
};

#endif
