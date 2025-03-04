// Copyright (C) 2023, MLIT Japan. All rights reserved.

#pragma once

#include "PLATEAUCityObjectGroup.h"
#include "TwinLinkFacilityInfo.h"
#include "TwinLinkNavSystemBuildingInfo.generated.h"

USTRUCT(BlueprintType)
struct FTwinLinkNavSystemBuildingInfo {
    GENERATED_BODY()
public:
    FTwinLinkNavSystemBuildingInfo() {}
    FTwinLinkNavSystemBuildingInfo(TWeakObjectPtr<UTwinLinkFacilityInfo> Facility, TWeakObjectPtr<UPLATEAUCityObjectGroup> CityObject)
        : FacilityInfo(Facility)
        , CityObjectGroup(CityObject) {
    }
    FTwinLinkNavSystemBuildingInfo(TWeakObjectPtr<UPLATEAUCityObjectGroup> CityObject)
        : CityObjectGroup(CityObject) {
    }

    // 建物情報
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Base")
        TWeakObjectPtr<UTwinLinkFacilityInfo> FacilityInfo;
    // 建物のコンポーネント
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Base")
        TWeakObjectPtr<UPLATEAUCityObjectGroup> CityObjectGroup;
};