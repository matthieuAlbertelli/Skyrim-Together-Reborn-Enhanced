#pragma once

#include <cstdint>

struct ItemPreviewRasterCaptureRequest
{
    bool valid{};
    bool captureRequested{};
    float left{};
    float top{};
    float width{};
    float height{};
    std::uint64_t selectionRevision{};
    std::uint64_t regionRevision{};
    std::uint64_t solverRevision{};
    std::uint64_t selectedItem{};
};

struct ItemPreviewRasterMeasurement
{
    bool valid{};
    std::uint64_t selectionRevision{};
    std::uint64_t regionRevision{};
    std::uint64_t solverRevision{};
    std::uint64_t selectedItem{};

    std::uint32_t targetWidth{};
    std::uint32_t targetHeight{};
    std::uint32_t modelLeft{};
    std::uint32_t modelTop{};
    std::uint32_t modelRight{};
    std::uint32_t modelBottom{};
    std::uint32_t safeLeft{};
    std::uint32_t safeTop{};
    std::uint32_t safeRight{};
    std::uint32_t safeBottom{};
    std::uint32_t safeOverflowLeft{};
    std::uint32_t safeOverflowTop{};
    std::uint32_t safeOverflowRight{};
    std::uint32_t safeOverflowBottom{};

    float modelCenterX{};
    float modelCenterY{};
    float safeCenterX{};
    float safeCenterY{};
    float containScale{};
    bool insideSafeTarget{};
    bool touchesScreenEdge{};
};
