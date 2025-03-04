﻿// Copyright (C) 2023, MLIT Japan. All rights reserved.

#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"

#include "TwinLinkWidgetBase.generated.h"

/**
 * 全てのWidgetのベースクラス
 */
UCLASS(Abstract, Blueprintable, BlueprintType, ClassGroup = TwinLink)
class TWINLINKWIDGETS_API UTwinLinkWidgetBase : public UUserWidget {
    GENERATED_BODY()

public:
    /**
     * @brief コンストラクト
    */
    virtual void NativeConstruct() override;

    /**
     * @brief デストラクト
    */
    virtual void NativeDestruct() override;

    /**
     * @brief ウィジェットを管理者モードに同期する
     * @param bIsActiveAdminMode 現在の管理者モード
    */
    UFUNCTION(BlueprintImplementableEvent, Category = "TwinLink")
    void OnChangedAdminMode(const bool bIsActiveAdminMode);

    /**
     * @brief 管理者モードの状態を取得する
     * 　その時の管理者モードによって処理を分岐したい時に使用する
     * @return
    */
    UFUNCTION(BlueprintCallable, Category = "TwinLink")
    bool IsActiveAdminMode() const;

private:
    // 登録デリゲードの管理Handle

    FDelegateHandle OnActiveAdminModeHnd;
    FDelegateHandle OnInactiveAdminModeHnd;

private:

    /**
     * @brief 管理者モード関係の初期設定
    */
    void SetupLinkageWithAdminMode();

    /**
     * @brief 管理者モード関係の終了時処理
    */
    void FinalizeLinkageWithAdminMode();

};
