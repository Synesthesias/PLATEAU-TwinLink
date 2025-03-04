// Copyright (C) 2023, MLIT Japan. All rights reserved.


#include "NavSystem/TwinLinkNavSystemPathFinder.h"

#include <optional>

#include "NavigationSystem.h"
#include "Kismet/GameplayStatics.h"
#include <NavSystem/TwinLinkNavSystemPathLocator.h>

#include "Misc/TwinLinkActorEx.h"
#include "Misc/TwinLinkMathEx.h"
#include "TwinLinkPlayerController.h"
#include "TwinLinkWorldViewer.h"
#include "NavSystem/TwinLinkNavSystem.h"
namespace {
    bool CreateFindPathRequest(const UObject* Self, const FVector& Start, const FVector& End, FPathFindingQuery& OutRequest) {
        const UNavigationSystemV1* NavSys = FNavigationSystem::GetCurrent<UNavigationSystemV1>(Self->GetWorld());
        if (!NavSys)
            return false;
        OutRequest = FPathFindingQuery();
        OutRequest.StartLocation = Start;
        OutRequest.EndLocation = End;
        // #NOTE : ナビメッシュは車と人で区別しないということなのでエージェントは設定しない
        //query.NavAgentProperties.AgentHeight = 100;
        //query.NavAgentProperties.AgentRadius = 10;
        OutRequest.NavData = NavSys->GetDefaultNavDataInstance();
        OutRequest.Owner = Self;
        return true;
    }
}
// Sets default values
ATwinLinkNavSystemPathFinder::ATwinLinkNavSystemPathFinder() {
    // Set this actor to call Tick() every frame.  You can turn this off to improve performance if you don't need it.
    PrimaryActorTick.bCanEverTick = true;
}

bool ATwinLinkNavSystemPathFinder::RequestStartPathFinding(FTwinLinkNavSystemFindPathInfo& Out) {
    const UNavigationSystemV1* NavSys = FNavigationSystem::GetCurrent<UNavigationSystemV1>(GetWorld());
    if (!NavSys)
        return false;
    // まだ両地点生成済みじゃない場合は無視する
    if (IsReadyPathFinding() == false)
        return false;
    auto Start = *GetPathLocation(TwinLinkNavSystemPathPointType::Start);
    auto Dest = *GetPathLocation(TwinLinkNavSystemPathPointType::Dest);
    Start.Z = Dest.Z = 0;
    Out = RequestPathFinding(Start, Dest);
    return true;
}

bool ATwinLinkNavSystemPathFinder::TryGetPathLocation(TwinLinkNavSystemPathPointType Type, FVector& Out) const {
    auto Ret = GetPathLocation(Type);
    if (Ret.has_value() == false)
        return false;
    Out = *Ret;
    return true;
}

bool ATwinLinkNavSystemPathFinder::TryGetCameraLocationAndLookAt(const FVector& NowCameraLocation, FVector& OutLocation, FVector& OutLookAt) const {
    FVector Start;
    const auto HasStart = TryGetPathLocation(TwinLinkNavSystemPathPointType::Start, Start);
    FVector Dest;
    const auto HasDest = TryGetPathLocation(TwinLinkNavSystemPathPointType::Dest, Dest);
    const auto NavSystem = ATwinLinkNavSystem::GetInstance(GetWorld());
    if (!NavSystem)
        return false;
    const auto Param = NavSystem->GetRuntimeParam();
    if (!Param)
        return false;
    if (HasStart && HasDest) {
        const auto Center = (Start + Dest) * 0.5f;

        // Start/Destが対象になるような位置/方向にカメラを置いたときに
        // その二つがスクリーン上のRangeの位置に来るような距離Depthを求める
        const auto PlayerController = GetWorld()->GetFirstPlayerController();
        FMinimalViewInfo ViewInfo;
        PlayerController->GetViewTarget()->CalcCamera(0.f, ViewInfo);
        const auto Near = FMath::Tan(FMath::DegreesToRadians(ViewInfo.FOV) * 0.5f);
        const auto Range = FMath::Max(1e-4f, Param->RouteGuideStartCameraScreenRange);
        // Start -> Destの法線を求める (#NOTE : Start/Destの高さにあまり差がない前提)
        auto Dir = (Dest - Start);
        const auto Len = Dir.Length() * 0.5f;
        const auto Depth = FMath::Max(Param->RouteGuideStartCameraMinDist, Len / Range * Near);

        Dir = FVector(Dir.Y, -Dir.X, 0).GetSafeNormal();
        // 現在のカメラ位置に近いほうにする
        if (Dir.Dot(NowCameraLocation - Center) < 0)
            Dir = -Dir;
        double PhiSi, PhiCo;
        FMath::SinCos(&PhiSi, &PhiCo, FMath::DegreesToRadians(Param->RouteGuideStartCameraRotDist.Y));
        OutLocation = Center + Depth * (PhiCo * Dir + PhiSi * FVector::UpVector);
        OutLookAt = Center;
        return true;
    }

    if (HasStart) {
        OutLocation = Start + TwinLinkMathEx::PolarDegree2Cartesian(Param->RouteGuideStartCameraRotDist);
        OutLookAt = Start;
        return true;
    }

    if (HasDest) {
        OutLocation = Dest + TwinLinkMathEx::PolarDegree2Cartesian(Param->RouteGuideStartCameraRotDist);
        OutLookAt = Dest;
        return true;
    }
    return false;
}

void ATwinLinkNavSystemPathFinder::ChangeCameraLocation(float MoveSec) const {

    if (const auto Viewer = ATwinLinkNavSystem::GetWorldViewer(GetWorld())) {
        FVector Location, LookAt;
        if (TryGetCameraLocationAndLookAt(Viewer->GetNowCameraLocationOrZero(), Location, LookAt)) {
            Viewer->SetLocationLookAt(Location, LookAt, MoveSec);
        }
    }
}

// Called when the game starts or when spawned
void ATwinLinkNavSystemPathFinder::BeginPlay() {
    Super::BeginPlay();
    if (const auto System = ATwinLinkNavSystem::GetInstance(GetWorld())) {
        System->OnFacilityClicked.AddUObject(this, &ATwinLinkNavSystemPathFinder::OnFacilityClick);
    }
}

void ATwinLinkNavSystemPathFinder::Tick(float DeltaTime) {
    Super::Tick(DeltaTime);

}

void ATwinLinkNavSystemPathFinder::OnFacilityClick(const FHitResult& Info, const FTwinLinkNavSystemBuildingInfo& Facility) {
    OnFacilityClicked.Broadcast(Info, Facility);
}

const ATwinLinkNavSystem* ATwinLinkNavSystemPathFinder::GetTwinLinkNavSystem() const {
    return TwinLinkActorEx::FindActorInOwner<ATwinLinkNavSystem>(this);
}

FTwinLinkNavSystemFindPathInfo ATwinLinkNavSystemPathFinder::RequestPathFinding(const FVector& Start,
    const FVector& End) const {
    UNavigationSystemV1* NavSys = FNavigationSystem::GetCurrent<UNavigationSystemV1>(GetWorld());
    FPathFindingQuery Query;
    CreateFindPathRequest(this, Start, End, Query);
    FTwinLinkNavSystemFindPathInfo Ret = FTwinLinkNavSystemFindPathInfo(NavSys->FindPathSync(Query, EPathFindingMode::Regular));
    const auto TwinLinkNavSystem = GetTwinLinkNavSystem();
    const auto DemCollisionAabb = TwinLinkNavSystem->GetDemCollisionAabb();
    const auto DemCollisionChannel = TwinLinkNavSystem->GetDemCollisionChannel();
    const auto HeightCheckInterval = TwinLinkNavSystem->GetPathFindingHeightCheckInterval();
    if (Ret.PathFindResult.IsSuccessful()) {
        auto GetHeightCheckedPos = [this, &DemCollisionAabb, &DemCollisionChannel](FVector Pos) {
            auto HeightMax = Pos;
            HeightMax.Z = DemCollisionAabb.Max.Z + 1;
            auto HeightMin = Pos;
            HeightMin.Z = DemCollisionAabb.Min.Z - 1;
            FHitResult HeightResult;

            if (GetWorld()->LineTraceSingleByChannel(HeightResult, HeightMax, HeightMin, DemCollisionChannel))
                Pos.Z = HeightResult.Location.Z + 1;
            return Pos;
        };

        auto& points = Ret.PathFindResult.Path->GetPathPoints();
        FVector LastPos = FVector::Zero();
        float ModLength = 0.f;
        for (auto i = 0; i < points.Num(); ++i) {
            auto& p = points[i];
            auto Pos = GetHeightCheckedPos(p.Location);
            if (i > 0) {
                auto Dir = (Pos - LastPos);
                auto Length = ModLength + Dir.Length();
                Dir.Normalize();
                auto CheckNum = static_cast<int32>((Length - ModLength) / HeightCheckInterval);
                for (auto I = 1; I <= CheckNum; ++I) {
                    auto P = GetHeightCheckedPos(LastPos + Dir * HeightCheckInterval);
                    Ret.HeightCheckedPoints.Add(P);
                    LastPos = P;
                }
            }
            Ret.PathPointsIndices.Add(Ret.HeightCheckedPoints.Num());
            Ret.HeightCheckedPoints.Add(Pos);

            LastPos = Pos;
        }
    }
    return Ret;
}

ATwinLinkNavSystemPathFinderAnyLocation::ATwinLinkNavSystemPathFinderAnyLocation() {
}

void ATwinLinkNavSystemPathFinderAnyLocation::BeginPlay() {
    Super::BeginPlay();

    // PlayerControllerを取得する
    APlayerController* controller = UGameplayStatics::GetPlayerController(GetWorld(), 0);

    // 入力を有効にする
    EnableInput(controller);
    // マウスカーソルオンにする
    controller->SetShowMouseCursor(true);
}

// Called every frame
void ATwinLinkNavSystemPathFinderAnyLocation::Tick(float DeltaTime) {
    Super::Tick(DeltaTime);

    const auto TwinLinkNavSystem = GetTwinLinkNavSystem();
    const auto DemCollisionAabb = TwinLinkNavSystem->GetDemCollisionAabb();

    // 
    const APlayerController* PlayerController = Cast<APlayerController>(UGameplayStatics::GetPlayerController(GetWorld(), 0));
    if (!PlayerController)
        return;
    const UNavigationSystemV1* NavSys = FNavigationSystem::GetCurrent<UNavigationSystemV1>(GetWorld());
    if (!NavSys)
        return;
    // 押したとき
    auto MousePos = ATwinLinkPlayerController::GetMousePosition(PlayerController);
    if (MousePos.has_value() == false)
        return;
    if (PlayerController->WasInputKeyJustPressed(EKeys::LeftMouseButton)) {
        auto Ray = ::ATwinLinkPlayerController::ScreenToWorldRayThroughBoundingBox(PlayerController, *MousePos, DemCollisionAabb);
        if (Ray.has_value()) {
            TArray<FHitResult> HitResults;
            FCollisionQueryParams Params;
            if (GetWorld()->LineTraceMultiByChannel(HitResults, Ray->Min, Ray->Max, ECC_WorldStatic)) {
                auto PointType = GetNextCreatePointType();
                FHitResult* BlockHitResult = nullptr;
                NowSelectedPathLocatorActor = nullptr;
                for (auto& HitResult : HitResults) {

                    if (HitResult.IsValidBlockingHit())
                        BlockHitResult = &HitResult;
                    // Start/Destが重なっていた場合最初にヒットしたほうを選択する
                    NowSelectedPathLocatorActor = Cast<ATwinLinkNavSystemPathLocator>(HitResult.GetActor());
                    if (NowSelectedPathLocatorActor) {
                        NowSelectedPathLocatorActorScreenOffset = FVector2D::Zero();
                        FVector2D ScreenPos = FVector2D::Zero();
                        if (UGameplayStatics::ProjectWorldToScreen(PlayerController, NowSelectedPathLocatorActor->GetActorLocation(), ScreenPos))
                            NowSelectedPathLocatorActorScreenOffset = ScreenPos - *MousePos;
                        break;
                    }
                }

                if (BlockHitResult != nullptr && NowSelectedPathLocatorActor == nullptr && PointType.IsValid()) {
                    NowSelectedPathLocatorActor = GetOrSpawnActor(PointType.GetEnumValue());
                    NowSelectedPathLocatorActor->UpdateLocation(NavSys, *BlockHitResult);
                    SetNowSelectedPointType(static_cast<TwinLinkNavSystemPathPointType>(PointType.GetValue() + 1));
                }
                if (NowSelectedPathLocatorActor) {
                    NowSelectedPathLocatorActor->Select();
                }
            }
        }
    }
    // 離した時
    else if (PlayerController->WasInputKeyJustReleased(EKeys::LeftMouseButton)) {
        if (NowSelectedPathLocatorActor) {
            NowSelectedPathLocatorActor->UnSelect();

            if (IsReadyPathFinding())
                OnReadyPathFinding.Broadcast();
        }
        NowSelectedPathLocatorActor = nullptr;
    }
    // ドラッグ中
    else if (PlayerController->IsInputKeyDown(EKeys::LeftMouseButton)) {
        if (NowSelectedPathLocatorActor) {
            const auto ScreenPos = *MousePos + NowSelectedPathLocatorActorScreenOffset;
            const auto Ray = ::ATwinLinkPlayerController::ScreenToWorldRayThroughBoundingBox(PlayerController, ScreenPos, DemCollisionAabb);
            if (Ray.has_value()) {
                TArray<FHitResult> HitResults;
                if (GetWorld()->LineTraceMultiByChannel(HitResults, Ray->Min, Ray->Max, ECollisionChannel::ECC_WorldStatic)) {
                    for (auto& HitResult : HitResults) {
                        if (HitResult.IsValidBlockingHit() == false)
                            continue;

                        NowSelectedPathLocatorActor->UpdateLocation(NavSys, HitResult);
                        break;
                    }
                }
            }
        }
    }
}

bool ATwinLinkNavSystemPathFinderAnyLocation::IsReadyPathFinding() const {
    return GetPathLocation(TwinLinkNavSystemPathPointType::Start).has_value() && GetPathLocation(TwinLinkNavSystemPathPointType::Dest).has_value();
}

std::optional<FVector> ATwinLinkNavSystemPathFinderAnyLocation::GetPathLocation(TwinLinkNavSystemPathPointType Type) const {
    if (PathLocatorActors.Contains(Type)) {
        if (const auto Actor = PathLocatorActors[Type])
            return Actor->GetLastValidLocation();
    }
    return std::nullopt;
}

void ATwinLinkNavSystemPathFinderAnyLocation::SetPathLocation(TwinLinkNavSystemPathPointType Type, FVector Location) {
    if (const auto Actor = GetOrSpawnActor(Type)) {
        const auto bIsChanged = Actor->UpdateLocation(FNavigationSystem::GetCurrent<UNavigationSystemV1>(GetWorld()), Location);
        if (bIsChanged && IsReadyPathFinding())
            OnReadyPathFinding.Broadcast();
    }
}

void ATwinLinkNavSystemPathFinderAnyLocation::Clear() {
    for (auto& Item : PathLocatorActors)
        Item.Value->Destroy(true);
}

std::optional<FBox> ATwinLinkNavSystemPathFinderAnyLocation::GetPathLocationBox(
    TwinLinkNavSystemPathPointType Type) const {
    if (!PathLocatorActors.Contains(Type))
        return std::nullopt;
    if (const auto Locator = PathLocatorActors[Type]) {
        return TwinLinkActorEx::GetActorBounds(Locator, false, true);
    }
    return std::nullopt;
}

ATwinLinkNavSystemPathLocator* ATwinLinkNavSystemPathFinderAnyLocation::GetOrSpawnActor(TwinLinkNavSystemPathPointType Type) {
    if (PathLocatorActors.Contains(Type))
        return PathLocatorActors[Type];

    FString Name = TEXT("PathLocator");
    Name.AppendInt(static_cast<int>(Type));
    auto Ret = TwinLinkActorEx::SpawnChildActor(this, PathLocatorBps[Type], Name);
    PathLocatorActors.Add(Type, Ret);
    return Ret;
}

ATwinLinkNavSystemPathFinderListSelect::ATwinLinkNavSystemPathFinderListSelect() {
}

void ATwinLinkNavSystemPathFinderListSelect::Tick(float DeltaTime) {
    if (bDebugIsLocationMovable == false)
        return;
    Super::Tick(DeltaTime);
}

void ATwinLinkNavSystemPathFinderListSelect::SetPathLocation(TwinLinkNavSystemPathPointType Type, FVector Location) {
    Super::SetPathLocation(Type, Location);
}

