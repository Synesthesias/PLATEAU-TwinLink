#include "TwinLinkNavSystem.h"
#include "NavigationSystem.h"
#include "Kismet/KismetSystemLibrary.h"
#include "Kismet/GameplayStatics.h"
#include "Components/InputComponent.h" 

namespace {
    template<class T>
    auto CreateChildActor(ATwinLinkNavSystem* self, TSubclassOf<T> Class, const TCHAR* Name) -> T* {
        FActorSpawnParameters Params;
        Params.Owner = self;
        Params.Name = FName(Name);
        const auto Ret = self->GetWorld()->SpawnActor<T>(Class, Params);
        if (!Ret)
            return Ret;
        Ret->SetActorLabel(FString(Name));
        Ret->AttachToActor(self, FAttachmentTransformRules::KeepWorldTransform, Name);
        return Ret;
    };

    template<class T, class F>
    void ForeachChildActor(AActor* Self, F&& Func) {
        if (!Self)
            return;
        for (auto& Child : Self->Children) {
            if (auto Target = Cast<T>(Child))
                Func(Target);

            ForeachChildActor<T>(Child, std::forward<F>(Func));
        }
    }

    std::optional<FVector2D> GetMousePosition(const APlayerController* PlayerController) {
        if (!PlayerController)
            return std::nullopt;
        float MouseX, MouseY;
        if (PlayerController->GetMousePosition(MouseX, MouseY)) {
            return FVector2D(MouseX, MouseY);
        }
        return std::nullopt;
    }

    // マウスカーソル位置におけるDemCollisionを包括するような線分を取得
    // Min/Maxが始点/終点を表す
    std::optional<FBox> GetMouseCursorWorldVector(const APlayerController* PlayerController, const FVector2D& MousePos, const FBox& RangeAabb) {
        if (!PlayerController)
            return std::nullopt;
        FVector WorldPosition, WorldDirection;
        if (UGameplayStatics::DeprojectScreenToWorld(PlayerController, MousePos, WorldPosition, WorldDirection)) {
            // 始点->道モデルのAABBの中心点までの距離 + Boxの対角線の長さがあれば道モデルとの
            const auto ToAabbVec = RangeAabb.GetCenter() - WorldPosition;
            const auto Length = ToAabbVec.Length() + (RangeAabb.Max - RangeAabb.Min).Length();
            return FBox(WorldPosition + WorldDirection * 100, WorldPosition + WorldDirection * Length);
        }
        return std::nullopt;

    }

}


ATwinLinkNavSystem::ATwinLinkNavSystem() {
    PrimaryActorTick.bCanEverTick = true;
    RootComponent = CreateDefaultSubobject<USceneComponent>(TEXT("Root"));
}

bool ATwinLinkNavSystem::CreateFindPathRequest(const FVector& Start, const FVector& End, FPathFindingQuery& OutRequest) const {
    const UNavigationSystemV1* NavSys = FNavigationSystem::GetCurrent<UNavigationSystemV1>(GetWorld());
    if (!NavSys)
        return false;
    OutRequest = FPathFindingQuery();
    OutRequest.StartLocation = Start;
    OutRequest.EndLocation = End;
    // #NOTE : ナビメッシュは車と人で区別しないということなのでエージェントは設定しない
    //query.NavAgentProperties.AgentHeight = 100;
    //query.NavAgentProperties.AgentRadius = 10;
    OutRequest.NavData = NavSys->GetDefaultNavDataInstance();
    OutRequest.Owner = this;
    return true;
}

TwinLinkNavSystemFindPathInfo ATwinLinkNavSystem::RequestFindPath(const FVector& Start, const FVector& End) const {

    UNavigationSystemV1* NavSys = FNavigationSystem::GetCurrent<UNavigationSystemV1>(GetWorld());
    FPathFindingQuery Query;
    CreateFindPathRequest(Start, End, Query);
    TwinLinkNavSystemFindPathInfo Ret = TwinLinkNavSystemFindPathInfo(NavSys->FindPathSync(Query, EPathFindingMode::Regular));
    if (Ret.PathFindResult.IsSuccessful()) {
        auto GetHeightCheckedPos = [this](FVector Pos) {
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
                auto CheckNum = static_cast<int32>((Length - ModLength) / FindPathHeightCheckInterval);
                for (auto I = 1; I <= CheckNum; ++I) {
                    auto P = GetHeightCheckedPos(LastPos + Dir * FindPathHeightCheckInterval);
                    Ret.HeightCheckedPoints.Add(P);
                    LastPos = P;
                }
            }
            Ret.HeightCheckedPoints.Add(Pos);
            LastPos = Pos;
        }
    }
    return Ret;
}

void ATwinLinkNavSystem::RequestFindPath() {
    const UNavigationSystemV1* NavSys = FNavigationSystem::GetCurrent<UNavigationSystemV1>(GetWorld());
    if (!NavSys)
        return;
    // まだ両地点生成済みじゃない場合は無視する
    if (GetNowSelectedPointType().IsValid())
        return;

    auto Start = PathLocatorActors[NavSystemPathPointType::Start]->GetActorLocation();
    Start.Z = 0;
    FNavLocation OutStart;
    if (NavSys->ProjectPointToNavigation(Start, OutStart, FVector::One() * 100))
        Start = OutStart;

    auto Dest = PathLocatorActors[NavSystemPathPointType::Dest]->GetActorLocation();
    Dest.Z = 0;
    FNavLocation OutDest;
    if (NavSys->ProjectPointToNavigation(Dest, OutDest, FVector::One() * 100))
        Dest = OutDest;
    PathFindInfo = RequestFindPath(Start, Dest);
}

void ATwinLinkNavSystem::Tick(float DeltaSeconds) {
    Super::Tick(DeltaSeconds);
    if (PathFindInfo.has_value() && PathFindInfo->PathFindResult.IsSuccessful()) {
        auto bBeforeSuccess = PathFindInfo->IsSuccess();
        PathFindInfo->Update(GetWorld(), DemCollisionChannel, DemCollisionAabb.Min.Z, DemCollisionAabb.Max.Z, 10);
        auto bAfterSuccess = PathFindInfo->IsSuccess();
        if (bBeforeSuccess == false && bAfterSuccess) {
            for (auto Child : PathDrawers) {
                if (Child)
                    Child->DrawPath(PathFindInfo->HeightCheckedPoints);
            }
        }
    }

    // 
    const APlayerController* PlayerController = Cast<APlayerController>(UGameplayStatics::GetPlayerController(GetWorld(), 0));
    if (!PlayerController)
        return;
    const UNavigationSystemV1* NavSys = FNavigationSystem::GetCurrent<UNavigationSystemV1>(GetWorld());
    if (!NavSys)
        return;
    // 押したとき
    auto MousePos = ::GetMousePosition(PlayerController);
    if (MousePos.has_value() == false)
        return;
    if (PlayerController->WasInputKeyJustPressed(EKeys::LeftMouseButton)) {
        UKismetSystemLibrary::PrintString(this, TEXT("KeyDown"), true, true, FColor::Cyan, 10.0f, TEXT("None"));

        auto Ray = ::GetMouseCursorWorldVector(PlayerController, *MousePos, DemCollisionAabb);
        if (Ray.has_value()) {
            TArray<FHitResult> HitResults;
            if (GetWorld()->LineTraceMultiByChannel(HitResults, Ray->Min, Ray->Max, ECC_WorldStatic)) {
                auto PointType = GetNowSelectedPointType();
                FHitResult* BlockHitResult = nullptr;
                NowSelectedPathLocatorActor = nullptr;
                for (auto& HitResult : HitResults) {

                    UKismetSystemLibrary::PrintString(this, HitResult.GetActor()->GetActorLabel(), true, true, FColor::Cyan, 10.0f, TEXT("None"));
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
                    FString Name = TEXT("PathLocator");
                    Name.AppendInt(PointType.GetValue());
                    NowSelectedPathLocatorActor = ::CreateChildActor(this, PathLocatorBps[PointType.GetEnumValue()], ToCStr(Name));
                    NowSelectedPathLocatorActor->SetActorLocation(BlockHitResult->Location);
                    NowSelectedPathLocatorActorLastValidLocation = BlockHitResult->Location;
                    PathLocatorActors.Add(PointType.GetEnumValue(), NowSelectedPathLocatorActor);
                    SetNowSelectedPointType(static_cast<NavSystemPathPointType>(PointType.GetValue() + 1));

                }
            }
        }
    }
    // 離した時
    else if (PlayerController->WasInputKeyJustReleased(EKeys::LeftMouseButton)) {
        UKismetSystemLibrary::PrintString(this, TEXT("KeyUp"), true, true, FColor::Cyan, 1.0f, TEXT("None"));
        if (NowSelectedPathLocatorActor)
            NowSelectedPathLocatorActor->SetActorLocation(NowSelectedPathLocatorActorLastValidLocation);
        NowSelectedPathLocatorActor = nullptr;
    }
    else if (PlayerController->IsInputKeyDown(EKeys::LeftMouseButton)) {

        UKismetSystemLibrary::PrintString(this, TEXT("Drag"), true, true, FColor::Cyan, 0.f, TEXT("None"));
        if (NowSelectedPathLocatorActor) {
            UKismetSystemLibrary::PrintString(this, TEXT("MoveActor"), true, true, FColor::Cyan, 0.f, TEXT("None"));
            auto ScreenPos = *MousePos + NowSelectedPathLocatorActorScreenOffset;
            auto Ray = ::GetMouseCursorWorldVector(PlayerController, ScreenPos, DemCollisionAabb);
            if (Ray.has_value()) {
                TArray<FHitResult> HitResults;
                if (GetWorld()->LineTraceMultiByChannel(HitResults, Ray->Min, Ray->Max, ECollisionChannel::ECC_WorldStatic)) {
                    for (auto& HitResult : HitResults) {

                        UKismetSystemLibrary::PrintString(this, TEXT("MoveHit"), true, true, FColor::Cyan, 0.f, TEXT("None"));
                        UKismetSystemLibrary::PrintString(this, HitResult.GetActor()->GetActorLabel(), true, true, FColor::Cyan, 1.0f, TEXT("None"));
                        if (HitResult.IsValidBlockingHit() == false)
                            continue;

                        NowSelectedPathLocatorActor->SetActorLocation(HitResult.Location);
                        auto IsGround = FVector::DotProduct(HitResult.Normal, FVector::UpVector) > FMath::Cos(FMath::DegreesToRadians(70));
                        auto Pos = HitResult.Location;
                        Pos.Z = 0.f;
                        FNavLocation OutStart;
                        auto IsInNavMesh = NavSys->ProjectPointToNavigation(Pos, OutStart, FVector::One() * 100);
                        auto IsValid = IsGround && IsInNavMesh;

                        NowSelectedPathLocatorActor->GetRootComponent()->GetChildComponent(1)->SetVisibility(IsValid == false);
                        if (IsValid) {
                            NowSelectedPathLocatorActorLastValidLocation = HitResult.Location;
                        }
                        break;
                    }
                }
            }
        }
    }
#ifdef WITH_EDITOR
    DebugDraw();
#endif
}

void ATwinLinkNavSystem::DebugDraw() {
#ifdef WITH_EDITOR
    UNavigationSystemV1* NavSys = FNavigationSystem::GetCurrent<UNavigationSystemV1>(GetWorld());
    if (!NavSys)
        return;
    if (DebugCallFindPath) {
        DebugCallFindPath = false;
        RequestFindPath();
    }

    // 道路モデルのAABB表示
    DrawDebugBox(GetWorld(), DemCollisionAabb.GetCenter(), DemCollisionAabb.GetExtent(), FColor::Red);

#endif
}


void ATwinLinkNavSystem::BeginPlay() {
    Super::BeginPlay();

    CreateChildActor(this, PathDrawerBp, TEXT("PathDrawerActor"));
    ForeachChildActor<AUTwinLinkNavSystemPathDrawer>(this, [&](AUTwinLinkNavSystemPathDrawer* Child) {
        PathDrawers.Add(Child);
        });
    // Input設定を行う
    SetupInput();
}

void ATwinLinkNavSystem::SetupInput() {
    // PlayerControllerを取得する
    APlayerController* controller = UGameplayStatics::GetPlayerController(GetWorld(), 0);

    // 入力を有効にする
    EnableInput(controller);
}

void ATwinLinkNavSystem::PressedAction() {
    // Pressed
    UKismetSystemLibrary::PrintString(this, TEXT("Pressed"), true, true, FColor::Cyan, 10.0f, TEXT("None"));
}

void ATwinLinkNavSystem::ReleasedAction() {
    // Released
    UKismetSystemLibrary::PrintString(this, TEXT("Released"), true, true, FColor::Cyan, 10.0f, TEXT("None"));
}

void ATwinLinkNavSystem::PressedAxis(const FInputActionValue& Value) {
    // input is a Vector2D
    FVector2D v = Value.Get<FVector2D>();

    // Axis Input Value
    UKismetSystemLibrary::PrintString(this, FString::Printf(TEXT("X:%f Y:%f"), v.X, v.Y), true, true, FColor::Cyan, 10.0f, TEXT("None"));
}