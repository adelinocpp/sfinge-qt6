#include "gabor_filter.h"
#include <cmath>
#include <algorithm>

// SIMD intrinsics support
#if defined(__x86_64__) || defined(_M_X64) || defined(__i386__) || defined(_M_IX86)
    #include <immintrin.h>
    #define SFINGE_SIMD_AVAILABLE 1

    // Check for SSE2 (required)
    #if defined(__SSE2__) || defined(_M_X64) || (defined(_M_IX86_FP) && _M_IX86_FP >= 2)
        #define SFINGE_SSE2_AVAILABLE 1
    #endif

    // Check for AVX (optional, for better performance)
    #if defined(__AVX__)
        #define SFINGE_AVX_AVAILABLE 1
    #endif
#else
    #define SFINGE_SIMD_AVAILABLE 0
#endif

namespace SFinGe {

GaborFilter::GaborFilter(int size, double sigma, double theta, double lambda, double gamma, double psi) {
    // Garantir tamanho ímpar
    if (size % 2 == 0) {
        size += 1;
    }
    m_size = size;
    m_kernel = createKernel(size, sigma, theta, lambda, gamma, psi);
}

std::vector<double> GaborFilter::createKernel(int size, double sigma, double theta, 
                                             double lambda, double gamma, double psi) {
    // Garantir que o tamanho seja ímpar para que halfSize funcione corretamente
    if (size % 2 == 0) {
        size += 1;
    }
    
    std::vector<double> kernel(size * size);
    
    double sigma_x = sigma;
    double sigma_y = sigma / gamma;
    
    int halfSize = size / 2;
    
    for (int y = -halfSize; y <= halfSize; ++y) {
        for (int x = -halfSize; x <= halfSize; ++x) {
            double x_theta = x * std::cos(theta) + y * std::sin(theta);
            double y_theta = -x * std::sin(theta) + y * std::cos(theta);
            
            double gaussian = std::exp(-0.5 * (x_theta * x_theta / (sigma_x * sigma_x) + 
                                               y_theta * y_theta / (sigma_y * sigma_y)));
            
            double sinusoid = std::cos(2.0 * M_PI * x_theta / lambda + psi);
            
            int idx = (y + halfSize) * size + (x + halfSize);
            kernel[idx] = gaussian * sinusoid;
        }
    }
    
    return kernel;
}

GaborFilterCache::GaborFilterCache(int cacheDegrees, int cacheFrequencies, 
                                   double minFreq, double maxFreq, int filterSize)
    : m_cacheDegrees(cacheDegrees)
    , m_cacheFrequencies(cacheFrequencies)
    , m_minFreq(minFreq)
    , m_maxFreq(maxFreq) {
    
    m_filters.reserve(cacheDegrees * cacheFrequencies);
    
    for (int i = 0; i < cacheDegrees; ++i) {
        for (int j = 0; j < cacheFrequencies; ++j) {
            // Convenção v4.0: theta do orientationMap já é perpendicular às cristas
            // NÃO adicionar +π/2 aqui
            double theta = indexToValue(i, 0, 2.0 * M_PI, cacheDegrees);
            double freq = indexToValue(j, minFreq, maxFreq, cacheFrequencies);
            double sigma = std::sqrt(-9.0 / (8.0 * freq * freq * std::log(0.001)));
            
            m_filters.emplace_back(filterSize, sigma, theta, 1.0 / freq, 1.0, 0.0);
        }
    }
}

const GaborFilter& GaborFilterCache::getFilter(int degreeIndex, int freqIndex) const {
    int index = degreeIndex * m_cacheFrequencies + freqIndex;
    return m_filters[index];
}

int GaborFilterCache::valueToIndex(double val, double min, double max, int n) const {
    double gap = (max - min) / n;
    return static_cast<int>(std::floor((val - min) / gap));
}

double GaborFilterCache::indexToValue(int index, double min, double max, int n) const {
    double gap = (max - min) / n;
    return gap * (static_cast<double>(index) + 0.5) + min;
}

// ============================================================================
// SIMD-Optimized Functions
// ============================================================================

bool GaborFilter::isSIMDAvailable() {
#ifdef SFINGE_SIMD_AVAILABLE
    return true;
#else
    return false;
#endif
}

double GaborFilter::applyFilterSIMD(const std::vector<double>& kernel, int kernelSize,
                                   const float* imageData, int imageWidth, int imageHeight,
                                   int centerX, int centerY) {
    int halfSize = kernelSize / 2;
    double sum = 0.0;

#ifdef SFINGE_AVX_AVAILABLE
    __m256d sum_vec = _mm256_setzero_pd();

    for (int ky = -halfSize; ky <= halfSize; ++ky) {
        int iy = centerY + ky;
        if (iy < 0 || iy >= imageHeight) continue;

        int kRowOffset = (ky + halfSize) * kernelSize;
        int iRowOffset = iy * imageWidth;

        int kx = -halfSize;
        // Process 4 elements at once with AVX
        for (; kx + 3 <= halfSize; kx += 4) {
            int ix_base = centerX + kx;

            // Check bounds for all 4 elements
            bool allValid = true;
            for (int i = 0; i < 4; ++i) {
                if (ix_base + i < 0 || ix_base + i >= imageWidth) {
                    allValid = false;
                    break;
                }
            }

            if (allValid) {
                // Load 4 kernel values
                int k_idx = kRowOffset + kx + halfSize;
                __m256d k_vec = _mm256_loadu_pd(&kernel[k_idx]);

                // Load 4 image values (convert float to double)
                int i_idx = iRowOffset + ix_base;
                __m128 img_f = _mm_loadu_ps(&imageData[i_idx]);
                __m256d img_d = _mm256_cvtps_pd(img_f);

                // Multiply and accumulate
                __m256d prod = _mm256_mul_pd(k_vec, img_d);
                sum_vec = _mm256_add_pd(sum_vec, prod);
            } else {
                // Process individually if bounds check fails
                for (int i = 0; i < 4; ++i) {
                    int ix = ix_base + i;
                    if (ix >= 0 && ix < imageWidth) {
                        int k_idx = kRowOffset + kx + i + halfSize;
                        int i_idx = iRowOffset + ix;
                        sum += kernel[k_idx] * imageData[i_idx];
                    }
                }
            }
        }

        // Process remaining elements
        for (; kx <= halfSize; ++kx) {
            int ix = centerX + kx;
            if (ix >= 0 && ix < imageWidth) {
                int k_idx = kRowOffset + kx + halfSize;
                int i_idx = iRowOffset + ix;
                sum += kernel[k_idx] * imageData[i_idx];
            }
        }
    }

    // Horizontal sum of AVX vector
    alignas(32) double sum_array[4];
    _mm256_store_pd(sum_array, sum_vec);
    sum += sum_array[0] + sum_array[1] + sum_array[2] + sum_array[3];

#elif defined(SFINGE_SSE2_AVAILABLE)
    __m128d sum_vec = _mm_setzero_pd();

    for (int ky = -halfSize; ky <= halfSize; ++ky) {
        int iy = centerY + ky;
        if (iy < 0 || iy >= imageHeight) continue;

        int kRowOffset = (ky + halfSize) * kernelSize;
        int iRowOffset = iy * imageWidth;

        int kx = -halfSize;
        // Process 2 elements at once with SSE2
        for (; kx + 1 <= halfSize; kx += 2) {
            int ix_base = centerX + kx;

            if (ix_base >= 0 && ix_base + 1 < imageWidth) {
                // Load 2 kernel values
                int k_idx = kRowOffset + kx + halfSize;
                __m128d k_vec = _mm_loadu_pd(&kernel[k_idx]);

                // Load 2 image values (convert float to double)
                int i_idx = iRowOffset + ix_base;
                __m128 img_f = _mm_loadl_pi(_mm_setzero_ps(), (__m64*)&imageData[i_idx]);
                __m128d img_d = _mm_cvtps_pd(img_f);

                // Multiply and accumulate
                __m128d prod = _mm_mul_pd(k_vec, img_d);
                sum_vec = _mm_add_pd(sum_vec, prod);
            } else {
                // Process individually if bounds check fails
                for (int i = 0; i < 2; ++i) {
                    int ix = ix_base + i;
                    if (ix >= 0 && ix < imageWidth) {
                        int k_idx = kRowOffset + kx + i + halfSize;
                        int i_idx = iRowOffset + ix;
                        sum += kernel[k_idx] * imageData[i_idx];
                    }
                }
            }
        }

        // Process remaining elements
        for (; kx <= halfSize; ++kx) {
            int ix = centerX + kx;
            if (ix >= 0 && ix < imageWidth) {
                int k_idx = kRowOffset + kx + halfSize;
                int i_idx = iRowOffset + ix;
                sum += kernel[k_idx] * imageData[i_idx];
            }
        }
    }

    // Horizontal sum of SSE vector
    alignas(16) double sum_array[2];
    _mm_store_pd(sum_array, sum_vec);
    sum += sum_array[0] + sum_array[1];

#else
    // Fallback scalar implementation
    for (int ky = -halfSize; ky <= halfSize; ++ky) {
        int iy = centerY + ky;
        if (iy < 0 || iy >= imageHeight) continue;

        for (int kx = -halfSize; kx <= halfSize; ++kx) {
            int ix = centerX + kx;
            if (ix < 0 || ix >= imageWidth) continue;

            int k_idx = (ky + halfSize) * kernelSize + (kx + halfSize);
            int i_idx = iy * imageWidth + ix;
            sum += kernel[k_idx] * imageData[i_idx];
        }
    }
#endif

    return sum;
}

}
