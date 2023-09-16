// Copyright (C) 2023, MLIT Japan. All rights reserved.

#include "TwinLink.h"

#include "Interfaces/IPluginManager.h"
void FTwinLinkModule::StartupModule() {
    bAdminMode = false;
}

void FTwinLinkModule::ShutdownModule() {}

FString FTwinLinkModule::GetContentDir() {
    return IPluginManager::Get()
        .FindPlugin(TEXT("PLATEAU-TwinLink"))
        ->GetContentDir();
}

void FTwinLinkModule::SetAdminMode(const bool bEnabled) {
    const auto IsSame = bAdminMode == bEnabled;
    bAdminMode = bEnabled;

    if (IsSame) {
        return;
    }

    if (bEnabled) {
        if (OnActiveAdminMode.IsBound()) {
            OnActiveAdminMode.Broadcast();
        }
    }
    else {
        if (OnInactiveAdminMode.IsBound()) {
            OnInactiveAdminMode.Broadcast();
        }
    }
}

bool FTwinLinkModule::IsAdminModeEnabled() const {
    return bAdminMode;
}

void UTwinLinkBlueprintLibrary::SetAdminMode(const bool bEnabled) {
    FTwinLinkModule::Get().SetAdminMode(bEnabled);
}

bool UTwinLinkBlueprintLibrary::IsAdminModeEnabled() {
    return FTwinLinkModule::Get().IsAdminModeEnabled();
}

void UTwinLinkBlueprintLibrary::TwinLinkSetFacilityModel(AActor* Model) {
    FTwinLinkModule::Get().SetFacilityModel(Model);
}

void UTwinLinkBlueprintLibrary::TwinLinkSetCityModel(AActor* Model) {
    FTwinLinkModule::Get().SetCityModel(Model);
}

ESlateVisibility UTwinLinkBlueprintLibrary::AsSlateVisibility(bool visibility)
{
    return visibility ? ESlateVisibility::Visible : ESlateVisibility::Hidden;
}

IMPLEMENT_MODULE(FTwinLinkModule, TwinLink)
