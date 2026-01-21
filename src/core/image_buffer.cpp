#include "image_buffer.h"
#include <fstream>
#include <iostream>

ImageBuffer::ImageBuffer(int width, int height)
    : m_width(width), m_height(height) {
    m_data.resize(width * height, 0);
}

ImageBuffer::~ImageBuffer() {
}

void ImageBuffer::setPixel(int x, int y, uint8_t value) {
    if (x >= 0 && x < m_width && y >= 0 && y < m_height) {
        m_data[y * m_width + x] = value;
    }
}

uint8_t ImageBuffer::getPixel(int x, int y) const {
    if (x >= 0 && x < m_width && y >= 0 && y < m_height) {
        return m_data[y * m_width + x];
    }
    return 0;
}

bool ImageBuffer::saveToFile(const char* filename) const {
    // Simple PGM format (grayscale)
    std::ofstream file(filename, std::ios::binary);
    if (!file.is_open()) {
        return false;
    }
    
    // Write PGM header
    file << "P5\n";
    file << m_width << " " << m_height << "\n";
    file << "255\n";
    
    // Write pixel data
    file.write(reinterpret_cast<const char*>(m_data.data()), m_data.size());
    
    return file.good();
}
