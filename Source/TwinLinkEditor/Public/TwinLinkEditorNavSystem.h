// Copyright (C) 2023, MLIT Japan. All rights reserved.

#pragma once

#include "CoreMinimal.h"
#include "Kismet/BlueprintFunctionLibrary.h"
#include "EditorLevelLibrary.h"

#include "TwinLinkEditorNavSystemParam.h"
#include "Subsystems/EditorActorSubsystem.h"
#include "TwinLinkEditorNavSystem.generated.h"
class APLATEAUInstancedCityModel;
class ANavMeshBoundsVolume;
/**
 * @brief : エディタ側のナビメッシュ関係を管理するクラス
 *        : `PLATEAUモデルからナビメッシュの生成等
 */
UCLASS()
class TWINLINKEDITOR_API UTwinLinkEditorNavSystem : public UEditorLevelLibrary {
    GENERATED_BODY()
public:
    /*
     * 道路モデルからナビメッシュを生成する
     * 道路モデル以外のメッシュはナビメッシュの影響フラグをオフ
     * 道路モデルを包括するようなBoundingBox生成
     * NavigationSystemにBuildを要求
     */
    UFUNCTION(BlueprintCallable, Category = "Nav")
        static void MakeNavMesh(UEditorActorSubsystem* Editor, UWorld* World, UTwinLinkEditorNavSystemParam* Param);

    /*
     * ワールドの全StaticMeshに対してbRelevantで指定した値でCanEverAffectNavigationを実行する
     */
    UFUNCTION(BlueprintCallable, Category = "Nav")
        static void SetCanEverAffectNavigationAllActors(UWorld* World, bool bRelevant);

    /*
     * ナビメッシュ生成済みかのエラーメッセージを返す
     */
    UFUNCTION(BlueprintCallable, Category = "Nav")
        static FString GetNavMeshBuildingMessage(UWorld* World);

    /*
     * ナビメッシュ生成済みかどうかを返す
     */
    UFUNCTION(BlueprintCallable, Category = "Nav")
        static bool IsNavMeshBuilt(UWorld* World);
};
