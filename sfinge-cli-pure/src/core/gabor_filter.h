#ifndef GABOR_FILTER_H
#define GABOR_FILTER_H

#include <vector>
#include <cmath>

namespace SFinGe {

class GaborFilter {
public:
    GaborFilter(int size, double sigma, double theta, double lambda, double gamma = 1.0, double psi = 0.0);
    
    const std::vector<double>& getKernel() const { return m_kernel; }
    int getSize() const { return m_size; }
    
    static std::vector<double> createKernel(int size, double sigma, double theta, double lambda, 
                                           double gamma = 1.0, double psi = 0.0);
    
private:
    std::vector<double> m_kernel;
    int m_size;
};

class GaborFilterCache {
public:
    GaborFilterCache(int cacheDegrees, int cacheFrequencies, 
                     double minFreq, double maxFreq, int filterSize);
    
    const GaborFilter& getFilter(int degreeIndex, int freqIndex) const;
    
    int getCacheDegrees() const { return m_cacheDegrees; }
    int getCacheFrequencies() const { return m_cacheFrequencies; }
    
private:
    int valueToIndex(double val, double min, double max, int n) const;
    double indexToValue(int index, double min, double max, int n) const;
    
    std::vector<GaborFilter> m_filters;
    int m_cacheDegrees;
    int m_cacheFrequencies;
    double m_minFreq;
    double m_maxFreq;
};

}

#endif
