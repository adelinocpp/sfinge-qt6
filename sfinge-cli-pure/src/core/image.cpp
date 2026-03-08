#include "image.h"
#include <cstdio>

#define STB_IMAGE_WRITE_IMPLEMENTATION
#include "stb/stb_image_write.h"

namespace SFinGe {

// Injeta o chunk pHYs (DPI) num PNG escrito pelo stbi_write_png.
// O PNG signature + IHDR ocupam os primeiros 33 bytes; o pHYs vai logo a seguir.
static void write_u32be(uint8_t* p, uint32_t v) {
    p[0] = (v >> 24) & 0xFF; p[1] = (v >> 16) & 0xFF;
    p[2] = (v >>  8) & 0xFF; p[3] =  v        & 0xFF;
}
static uint32_t png_crc32(const uint8_t* data, size_t len) {
    uint32_t crc = 0xFFFFFFFFu;
    for (size_t i = 0; i < len; ++i) {
        crc ^= data[i];
        for (int j = 0; j < 8; ++j)
            crc = (crc >> 1) ^ (0xEDB88320u & (uint32_t)(-(int32_t)(crc & 1)));
    }
    return crc ^ 0xFFFFFFFFu;
}
static bool inject_phys_chunk(const std::string& filename, int dpi) {
    FILE* f = fopen(filename.c_str(), "rb");
    if (!f) return false;
    fseek(f, 0, SEEK_END);
    long fsize = ftell(f);
    fseek(f, 0, SEEK_SET);
    std::vector<uint8_t> buf(static_cast<size_t>(fsize));
    if (fread(buf.data(), 1, buf.size(), f) != buf.size()) { fclose(f); return false; }
    fclose(f);

    // pHYs chunk: 4 (length) + 4 (type) + 4 (ppuX) + 4 (ppuY) + 1 (unit) + 4 (CRC) = 21 bytes
    uint32_t ppu = static_cast<uint32_t>(dpi / 0.0254 + 0.5);  // pixels per metre
    uint8_t chunk[21];
    write_u32be(chunk + 0, 9);          // data length = 9
    chunk[4]='p'; chunk[5]='H'; chunk[6]='Y'; chunk[7]='s';
    write_u32be(chunk +  8, ppu);       // ppuX
    write_u32be(chunk + 12, ppu);       // ppuY
    chunk[16] = 1;                      // unit: metre
    write_u32be(chunk + 17, png_crc32(chunk + 4, 13));  // CRC over type+data

    // Inserir após IHDR (offset 33 = 8-byte sig + 25-byte IHDR chunk)
    const size_t pos = 33;
    if (buf.size() < pos) return false;
    std::vector<uint8_t> out;
    out.reserve(buf.size() + 21);
    out.insert(out.end(), buf.begin(), buf.begin() + pos);
    out.insert(out.end(), chunk, chunk + 21);
    out.insert(out.end(), buf.begin() + pos, buf.end());

    f = fopen(filename.c_str(), "wb");
    if (!f) return false;
    bool ok = fwrite(out.data(), 1, out.size(), f) == out.size();
    fclose(f);
    return ok;
}

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
        if (result && m_dpi > 0)
            inject_phys_chunk(filename, m_dpi);
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
