#include <TiltedOnlinePCH.h>

#include <Services/ItemPreview/ItemPreviewFitSolver.h>

#include <algorithm>
#include <cmath>

ItemPreviewFitResult SolveItemPreviewFit(
    const ItemPreviewTransform& aBaseTransform,
    const ItemPreviewRasterBounds& aBounds,
    const ItemPreviewFitCalibration& aCalibration) noexcept
{
    ItemPreviewFitResult result{};

    const float renderHeight = static_cast<float>(aBounds.targetHeight);
    const float pixelsPerItemX =
        aCalibration.itemXPerScreenHeight * renderHeight;
    const float pixelsPerItemZ =
        aCalibration.itemZPerScreenHeight * renderHeight;

    if (!std::isfinite(renderHeight) || renderHeight <= 1.0F ||
        !std::isfinite(pixelsPerItemX) ||
        !std::isfinite(pixelsPerItemZ) ||
        std::abs(pixelsPerItemX) <= 0.0001F ||
        std::abs(pixelsPerItemZ) <= 0.0001F ||
        !std::isfinite(aBounds.containScale) ||
        aBounds.containScale <= 0.0F)
    {
        return result;
    }

    result.desiredPixelsX =
        aBounds.targetCenterX - aBounds.modelCenterX;
    result.desiredPixelsY =
        aBounds.targetCenterY - aBounds.modelCenterY;

    float scaleFactor =
        aBounds.containScale * aCalibration.targetFill;
    if (aBounds.touchesScreenEdge)
    {
        scaleFactor = std::min(
            scaleFactor,
            aCalibration.edgeFallbackScale);
    }

    result.appliedScaleFactor = std::clamp(
        scaleFactor,
        aCalibration.minimumStepScale,
        aCalibration.maximumStepScale);

    result.transform.x = std::clamp(
        aBaseTransform.x + result.desiredPixelsX / pixelsPerItemX,
        aCalibration.minimumPosition,
        aCalibration.maximumPosition);
    result.transform.y = aBaseTransform.y;
    result.transform.z = std::clamp(
        aBaseTransform.z + result.desiredPixelsY / pixelsPerItemZ,
        aCalibration.minimumPosition,
        aCalibration.maximumPosition);
    result.transform.scale = std::clamp(
        aBaseTransform.scale * result.appliedScaleFactor,
        aCalibration.minimumScale,
        aCalibration.maximumScale);
    result.valid =
        std::isfinite(result.transform.x) &&
        std::isfinite(result.transform.y) &&
        std::isfinite(result.transform.z) &&
        std::isfinite(result.transform.scale);
    return result;
}
