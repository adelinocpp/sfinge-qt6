#ifndef MATH_UTILS_H
#define MATH_UTILS_H

#include <cmath>
#include <vector>

namespace SFinGe {

inline bool insideEllipse(int cx, int cy, int a, int b, int x, int y) {
    int t = (x - cx) * (x - cx) * b * b + (y - cy) * (y - cy) * a * a - a * a * b * b;
    return t < 0;
}

inline double findNoise2(double x, double y) {
    int n = static_cast<int>(x) + static_cast<int>(y) * 57;
    n = (n << 13) ^ n;
    int nn = (n * (n * n * 60493 + 19990303) + 1376312589) & 0x7fffffff;
    return 1.0 - (static_cast<double>(nn) / 1073741824.0);
}

inline double interpolate(double a, double b, double x) {
    double ft = x * M_PI;
    double f = (1.0 - std::cos(ft)) * 0.5;
    return a * (1.0 - f) + b * f;
}

inline double noise(double x, double y) {
    double floorx = static_cast<double>(static_cast<int>(x));
    double floory = static_cast<double>(static_cast<int>(y));
    
    double s = findNoise2(floorx, floory);
    double t = findNoise2(floorx + 1, floory);
    double u = findNoise2(floorx, floory + 1);
    double v = findNoise2(floorx + 1, floory + 1);
    
    double int1 = interpolate(s, t, x - floorx);
    double int2 = interpolate(u, v, x - floorx);
    
    return interpolate(int1, int2, y - floory);
}

inline std::vector<float> renderClouds(int width, int height, double zoom, double persistence) {
    const int octaves = 2;
    std::vector<float> cloud(width * height);
    
    for (int y = 0; y < height; ++y) {
        for (int x = 0; x < width; ++x) {
            double getNoise = 0.0;
            
            for (int a = 0; a < octaves - 1; ++a) {
                double frequency = std::pow(2.0, a);
                double amplitude = std::pow(persistence, a);
                getNoise += noise(static_cast<double>(x) * frequency / zoom,
                                static_cast<double>(y) * frequency / zoom) * amplitude;
            }
            
            getNoise = (getNoise + 1.0) / 2.0;
            if (getNoise > 1.0) getNoise = 1.0;
            if (getNoise < 0.0) getNoise = 0.0;
            
            cloud[y * width + x] = static_cast<float>(getNoise);
        }
    }
    
    return cloud;
}

}

#endif
