#pragma once

#define CPP_SHARED
#include "shared.h"
#include "utils.h"

class HeuristicsBase
{
public:
    HeuristicsBase()
    {
        // For 8bit per component, there are 256 values(0-255)
        m_colorValuesPerBin = 256 / NUM_BINS_PER_HISTOGRAM;
    }

    virtual ~HeuristicsBase() { }

    void CalculateStatistics(uint32_t* histograms, float pixelCount);
    void ResetStats();

    virtual DirectX::SimpleMath::Vector3 GetColor(ColorSpace colorSpace,
        uint32_t width, uint32_t height) = 0;

    DirectX::SimpleMath::Vector3 CalculateColorFromTopBins(std::vector<uint32_t>* rawHistoData,
        size_t rawHistoDataSize, uint32_t width, uint32_t height);

protected:
    uint32_t m_colorValuesPerBin;
    float m_pixelCount;
    uint32_t const numberOfBinsToSumX = 4;

    float m_maxBin[NUMBER_OF_HISTOGRAMS];
    float m_maxPixels[NUMBER_OF_HISTOGRAMS];
    float m_avgBins[NUMBER_OF_HISTOGRAMS];

    // Histogram data stored (for when heuristics need raw data instead of statistics)
    std::vector<uint32_t> m_histogramsRawData[NUMBER_OF_HISTOGRAMS];
};

class Heuristic_Avg : public HeuristicsBase
{
public:
    Heuristic_Avg() {}
    ~Heuristic_Avg() {}
    DirectX::SimpleMath::Vector3 GetColor(ColorSpace colorSpace,
        uint32_t width, uint32_t height) override;
};


class Heuristic_TopXBuckets : public HeuristicsBase
{
public:
    Heuristic_TopXBuckets() {}
    ~Heuristic_TopXBuckets() {}
    DirectX::SimpleMath::Vector3 GetColor(ColorSpace colorSpace,
        uint32_t width, uint32_t height) override;
};


class Heuristic_BucketsXAverage : public HeuristicsBase
{
public:
    Heuristic_BucketsXAverage() {}
    ~Heuristic_BucketsXAverage() {}
    DirectX::SimpleMath::Vector3 GetColor(ColorSpace colorSpace,
        uint32_t width, uint32_t height) override;
};

