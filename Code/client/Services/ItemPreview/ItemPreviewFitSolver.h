#pragma once

#include <cstdint>

struct ItemPreviewTransform
{
    float x{};
    float y{};
    float z{};
    float scale{1.0F};
};

struct ItemPreviewRasterBounds
{
    std::uint32_t targetHeight{};
    float modelCenterX{};
    float modelCenterY{};
    float targetCenterX{};
    float targetCenterY{};
    float containScale{1.0F};
    bool touchesScreenEdge{};
};

struct ItemPreviewFitCalibration
{
    float itemXPerScreenHeight{-0.0111472801F};
    float itemZPerScreenHeight{-0.0111111111F};
    float targetFill{0.94F};
    float edgeFallbackScale{0.60F};
    float minimumScale{0.15F};
    float maximumScale{8.0F};
    float minimumStepScale{0.20F};
    float maximumStepScale{6.0F};
    float minimumPosition{-256.0F};
    float maximumPosition{256.0F};
};

struct ItemPreviewFitResult
{
    bool valid{};
    ItemPreviewTransform transform{};
    float desiredPixelsX{};
    float desiredPixelsY{};
    float appliedScaleFactor{1.0F};
};

[[nodiscard]] ItemPreviewFitResult SolveItemPreviewFit(
    const ItemPreviewTransform& aBaseTransform,
    const ItemPreviewRasterBounds& aBounds,
    const ItemPreviewFitCalibration& aCalibration = {}) noexcept;
