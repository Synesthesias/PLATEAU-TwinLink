﻿// Copyright (C) 2023, MLIT Japan. All rights reserved.

#pragma once

#include "CoreMinimal.h"
#include "TwinLinkWidgetBase.h"
#include "TwinLinkTimeControlPanelBase.generated.h"

/**
 * 時間設定パネルウィジェットの基底クラス
 */
UCLASS(Abstract, Blueprintable, BlueprintType, ClassGroup = TwinLink)
class TWINLINKWIDGETS_API UTwinLinkTimeControlPanelBase : public UTwinLinkWidgetBase
{
	GENERATED_BODY()
	
public:
    /**
     * @brief 
    */
    virtual void NativeConstruct() override;
    /**
     * @brief 
    */
    virtual void NativeDestruct() override;

public:
    /**
     * @brief 初期化
     * クラスが想定する動作を行う状態にする
     * もう一度読んだらリセットして初期化し直す
    */
    UFUNCTION(BlueprintCallable, Category = "TwinLink")
    void Setup();

    /**
     * @brief BP側の初期化
    */
    UFUNCTION(BlueprintImplementableEvent, Category = "TwinLink")
    void SetupWBP();

    /**
     * @brief 日時のオフセットの設定
     * スライドバーで表現する期日の開始地点
     * @param Offset 
    */
    UFUNCTION(BlueprintCallable, Category = "TwinLink")
    void SetOffsetDateTime(const FDateTime& Offset);

    /**
     * @brief ステップ値によって日時のオフセットを設定する
     * @param Step 
    */
    UFUNCTION(BlueprintCallable, Category = "TwinLink")
    void SetOffsetDateTimeFromStep(const float& Step);

    /**
     * @brief スライドバーで時間を操作する　24時間の範囲で
     * 24時間の開始地点はSetOffsetDateTime()で決める
     * @param Step
    */
    UFUNCTION(BlueprintCallable, Category = "TwinLink")
    void SetSlideTimespan(float Step);

    /**
     * @brief リアルタイムモードを有効にする
     * @param bIsActive 
    */
    UFUNCTION(BlueprintCallable, Category = "TwinLink")
    void ActivateRealtimeMode(bool bIsActive);

    /**
     * @brief 現在の設定時刻を取得する
     * @return 
    */
    UFUNCTION(BlueprintCallable, Category = "TwinLink")
    FDateTime GetSelectedDateTime() const;

    /**
     * @brief 現在の設定時刻を取得する
     * @return
    */
    UFUNCTION(BlueprintCallable, Category = "TwinLink")
    float GetOffsetDateAsNormalized() const;

    /**
     * @brief 現在の設定時刻を取得する
     * @return
    */
    UFUNCTION(BlueprintCallable, Category = "TwinLink")
    float GetOffsetTimeAsNormalized() const;

    /**
     * @brief 設定時刻が変更された時に呼び出される
     * @param InDateTime 
     * @param bIsRealTime
    */
    UFUNCTION(BlueprintImplementableEvent, Category = "TwinLink")
    void OnChangedSelectDateTime(const FDateTime& InDateTime, bool bIsRealTime = false);

    /**
     * @brief リアルタイムモードが変更された
     * @param bIsActive 
    */
    UFUNCTION(BlueprintImplementableEvent, Category = "TwinLink")
    void OnChangedRealtimeMode(bool bIsActive);

    /**
     * @brief スライダーの値を変更する
     * @param bIsActive
    */
    UFUNCTION(BlueprintImplementableEvent, Category = "TwinLink")
    void ChangedSlideValue(float DayStep, float TimeStep);

    /**
     * @brief 基準の日時からを1週間の範囲を取得する
     * 年、月、日のみ設定してある
     * @return 
    */
    UFUNCTION(BlueprintCallable, Category = "TwinLink")
    const TArray<FDateTime> GetWeekCollection() const;

private:
    /**
     * @brief 24時間の範囲を0-1で表現する
     * 年、月、日は利用していなので注意
     * @param Date 
     * @return 
    */
    double NormalizeDateTimeAsDay(const FTimespan& Date) const;

    /**
     * @brief 指定時間の設定
     * @param Offset 
     * @param Step 
    */
    void SetRealTime();

    /**
     * @brief OffsetDateTime, SlideTimespanを指定した時間から計算する
     * @param InTime 
    */
    void CalcTimeParamter(const FDateTime& InTime);

private:    
    /** リアルタイムの時間による更新頻度 **/
    const float RealtimeUpdateRate = 60.0f;

    /** 表示される期間の最初の地点 **/
    FDateTime StartDateTime;

    /** 表示される期間の範囲 **/
    const int SelectableDaySpan = 7;

    /** スライドバーで操作を開始する基準時間 **/
    FDateTime OffsetDateTime;
    /** スライドバーによって操作された時間 **/
    FTimespan SlideTimespan;

    /** リアルタイムモードか **/
    bool bIsRealtimeMode;

    /** リアルタイムモードの更新タイマー **/
    FTimerHandle RealTimeModeUpdateTimerHnd;

};
