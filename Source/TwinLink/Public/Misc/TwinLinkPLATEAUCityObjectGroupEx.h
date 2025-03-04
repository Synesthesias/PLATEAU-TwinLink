#pragma once
#include "PLATEAUInstancedCityModel.h"

class TWINLINK_API FTwinLinkPLATEAUCityObjectGroupEx
{
public:
    /*
     * @brief : BoundingBox取得
     */
    static bool TryBoundingBox(const UPLATEAUCityObjectGroup* Self, FBox& Out);

    /*
     * @brief : BoundingBox取得
     */
    static std::optional<FBox> GetBoundingBox(const UPLATEAUCityObjectGroup* Self);

    /*
     * @brief : StaticMeshのBoundingBox取得
     */
    static bool TryGetStaticMeshBoundingBox(const UPLATEAUCityObjectGroup* Self, FBox& Out);

    /*
     * @brief : StaticMeshのBoundingBox取得
     */
    static std::optional<FBox> GetStaticMeshBoundingBox(const UPLATEAUCityObjectGroup* Self);
};
