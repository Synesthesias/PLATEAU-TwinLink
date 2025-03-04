// Copyright (C) 2023, MLIT Japan. All rights reserved.

#pragma once

#include "CoreMinimal.h"
#include "TwinLinkWidgetBase.h"
#include "NavSystem/TwinLinkNavSystemEntranceLocator.h"
#include "TwinLinkAddFacilityDialogBase.generated.h"

class ATwinLinkNavSystemPathLocator;
class ATwinLinkNavSystemEntranceLocator;
class ATwinLinkWorldViewer;

/**
 * 施設情報追加ダイアログのcpp基底クラス
 */
UCLASS(Abstract, Blueprintable, BlueprintType, ClassGroup = TwinLink)
class TWINLINKWIDGETS_API UTwinLinkAddFacilityDialogBase
    : public UTwinLinkWidgetBase {
    GENERATED_BODY()
    GENERATED_BODY()

public:
    /**
     * @brief 設定
    */
    UFUNCTION(BlueprintCallable, Category = "TwinLink")
        void Setup();

    /**
     * @brief 施設情報を追加する
     * @param _Name
     * @param _Category
     * @param _FeatureID  使ってない削除予定
     * @param _ImageFileName
     * @param _Description
     * @param _SpotInfo
    */
    UFUNCTION(BlueprintCallable, Category = "TwinLink")
        void AddFacilityInfo(
            const FString& InName, const FString& InCategory,
            const FString& InImageFileName,
            const FString& InDescription, const FString& InSpotInfo);

    /**
     * @brief イメージファイルのパスを作成する
     * @param InFileName 拡張子付きファイル名
     * @return
    */
    UFUNCTION(BlueprintCallable, Category = "TwinLink")
        FString CreateImageFilePath(const FString& InFileName);

    /**
     * @brief 施設情報が追加されたときに呼び出される
    */
    UFUNCTION(BlueprintImplementableEvent, Category = "TwinLink")
        void OnAddedFacilityInfo();

    /**
     * @brief 施設情報の追加に失敗したときに呼び出される
    */
    UFUNCTION(BlueprintImplementableEvent, Category = "TwinLink")
        void OnFailedFacilityInfo();

    /**
     * @brief 施設情報のFeatureIDが指定された時に呼び出される
    */
    UFUNCTION(BlueprintImplementableEvent, Category = "TwinLink")
        void OnSelectFeatureID(const FString& _FeatureID);

    /**
     * @brief 施設カテゴリの種類を設定する
    */
    UFUNCTION(BlueprintImplementableEvent, Category = "TwinLink")
        void SyncCategoryCollectionWidget(const TArray<FString>& Categories);

    UFUNCTION(BlueprintCallable, Category = "TwinLink")
        const UPrimitiveComponent* GetTargetFeatureComponent() const;

    UFUNCTION(BlueprintCallable, Category = "TwinLink")
        void OnShowDialog();
    virtual void BeginDestroy() override;

    /*
     * @brief : 自身が表示されていたら, 現在見ている建物のポインタを返す
     */
    UFUNCTION(BlueprintCallable, Category = "TwinLink")
        const UObject* GetSelectedObjectIfVisible();
protected:
    FString FeatureID;
    FTwinLinkEntranceLocatorWidgetNode* EntranceLocatorNode = nullptr;
};
