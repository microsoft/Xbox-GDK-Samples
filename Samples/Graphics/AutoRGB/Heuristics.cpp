#include "pch.h"
#include "Heuristics.h"

using namespace DirectX::SimpleMath;

void HeuristicsBase::CalculateStatistics(uint32_t* histograms, float pixelCount)
{
    assert(histograms && "histograms argument cannot be null.");

    ResetStats();
    uint32_t const componentCount = NUMBER_OF_HISTOGRAMS;

    float pixelSumPerHistogram[componentCount] = {};
    m_pixelCount = pixelCount;

    std::vector<uint32_t> asd;
    for (size_t i = 0; i < componentCount * NUM_BINS_PER_HISTOGRAM; ++i)
    {
        uint32_t num = histograms[i];
        asd.push_back(num);
    }

    for (uint32_t comp = 0; comp < componentCount; ++comp)
    {
        auto& currentHistogram = m_histogramsRawData[comp];
        currentHistogram.resize(NUM_BINS_PER_HISTOGRAM);

        uint32_t const offset = comp * NUM_BINS_PER_HISTOGRAM;
        for (uint32_t bin = 0; bin < NUM_BINS_PER_HISTOGRAM; ++bin)
        {
            uint32_t pixelsInThisBin = histograms[offset + bin];
            currentHistogram[bin] = pixelsInThisBin;

            m_avgBins[comp] += bin * (pixelsInThisBin / m_pixelCount);
            pixelSumPerHistogram[comp] += pixelsInThisBin;

            if (pixelsInThisBin > m_maxPixels[comp])
            {
                m_maxPixels[comp] = static_cast<float>(pixelsInThisBin);
                m_maxBin[comp] = static_cast<float>(bin);
            }
        }
    }
}

void HeuristicsBase::ResetStats()
{
    for (uint32_t hist = 0; hist < NUMBER_OF_HISTOGRAMS; ++hist)
    {
        m_avgBins[hist] = 0.0f;
        m_maxPixels[hist] = 0.0f;
        m_maxBin[hist] = 0.0f;

        m_histogramsRawData[hist].clear();
    }
}


Vector3 HeuristicsBase::CalculateColorFromTopBins(std::vector<uint32_t>* rawHistoData,
    size_t rawHistoDataSize, uint32_t width, uint32_t height)
{
    std::ignore = rawHistoDataSize;
    float sumOfTopXBins[NUMBER_OF_HISTOGRAMS] = {};

    Vector3 finalColor;
    uint32_t pixelCoverage = 0;
    uint32_t numBinsUsed = 0;

    // Percentage of pixels required before we stop adding buckets
    float pixelsRequiredCoveragePcg = 0.3f;

    // This value is a hack to avoid one color overly dominating the rest
    // Tested with 0.01 and 0.05 - in between seems the sweet spot
    float binDisplacementValue = 0.01f;

    // Check the first X buckets (for all 3 histograms corresponding to the current color space)
    // until you have covered enough pixels (for now, a % of total pixels).
    for (int bin = NUM_BINS_PER_HISTOGRAM - 1; bin >= 0; --bin)
    {
        float threshold = pixelsRequiredCoveragePcg * (width * height);
        if (pixelCoverage > threshold)
        {
            break;
        }

        numBinsUsed++;
        for (uint32_t histogram = 0; histogram < 3; ++histogram)
        {
            uint32_t pixelsInThisBin = rawHistoData[histogram][static_cast<uint32_t>(bin)];

            sumOfTopXBins[histogram] += pixelsInThisBin * (bin + binDisplacementValue);
            pixelCoverage += pixelsInThisBin;
        }
    }

    // Even if the first set of bins covers enough pixels, we might want to look
    // at a minimum number of bins in order to capture more light information
    uint32_t minBins = 2;
    numBinsUsed = std::max(numBinsUsed, minBins);

    // Experiment 2 - Add luma info to the final result
    // TODO - For now we are not using luma since the supported device don't support modifying the lamps intensity

    // Experiment 3 - return black in case the intensity is way too low

    finalColor = Vector3(sumOfTopXBins[0], sumOfTopXBins[1], sumOfTopXBins[2]);
    finalColor.Normalize();
    return finalColor;
}


/// AVERAGE HEURISTIC
Vector3 Heuristic_Avg::GetColor(ColorSpace colorSpace, uint32_t width, uint32_t height)
{
    // Not supporting YCC for now
    if (colorSpace == ColorSpace::YCC)
        assert(false && L"Not supporting YCC for this Heuristic");

    Vector3 finalColor;
    std::ignore = width;
    std::ignore = height;

    uint8_t r = 0, g = 0, b = 0;
        
    r = static_cast<uint8_t>(std::min(m_avgBins[0] * m_colorValuesPerBin, 255.0f));
    g = static_cast<uint8_t>(std::min(m_avgBins[1] * m_colorValuesPerBin, 255.0f));
    b = static_cast<uint8_t>(std::min(m_avgBins[2] * m_colorValuesPerBin, 255.0f));

    finalColor = Vector3(r, g, b) / 255.0f;

    finalColor.Normalize();

    // In RGB color space, [0.0 - 1.0] floats
    return finalColor;
}

/// TOP_X_BUCKETS HEURISTIC
Vector3 Heuristic_TopXBuckets::GetColor(ColorSpace colorSpace, uint32_t width, uint32_t height)
{
    // Not supporting YCC for now
    if (colorSpace == ColorSpace::YCC)
        assert(false && L"Not supporting YCC for this Heuristic");

    Vector3 finalColor = CalculateColorFromTopBins(m_histogramsRawData, std::size(m_histogramsRawData), width, height);

    // In RGB color space, [0.0-1.0] floats
    return finalColor;
}

/// AVG_X_TOP_BUCKETS HEURISTIC
Vector3 Heuristic_BucketsXAverage::GetColor(ColorSpace colorSpace, uint32_t width, uint32_t height)
{
    // Not supporting YCC for now
    if (colorSpace == ColorSpace::YCC)
        assert(false && L"Not supporting YCC for this Heuristic");

    Vector3 colorFromTopBuckets = CalculateColorFromTopBins(m_histogramsRawData, std::size(m_histogramsRawData), width, height);

    Vector3 avgCompColor;
    {
        uint8_t c1 = 0, c2 = 0, c3 = 0;
        c1 = static_cast<uint8_t>(std::min(m_avgBins[0] * m_colorValuesPerBin, 255.0f));
        c2 = static_cast<uint8_t>(std::min(m_avgBins[1] * m_colorValuesPerBin, 255.0f));
        c3 = static_cast<uint8_t>(std::min(m_avgBins[2] * m_colorValuesPerBin, 255.0f));

        avgCompColor = Vector3(c1, c2, c3) / 255.0f;
        avgCompColor.Normalize();
    }

    // In RGB color space, [0.0 - 1.0] floats
    float alpha = 0.4f;
    return (1.0f - alpha) * colorFromTopBuckets + alpha * avgCompColor;
}
