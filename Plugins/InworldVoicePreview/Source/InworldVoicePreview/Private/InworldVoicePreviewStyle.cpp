// Copyright Epic Games, Inc. All Rights Reserved.

#include "InworldVoicePreviewStyle.h"
#include "Styling/SlateStyleRegistry.h"
#include "Framework/Application/SlateApplication.h"
#include "Slate/SlateGameResources.h"
#include "Interfaces/IPluginManager.h"
#include "Styling/SlateStyleMacros.h"

#define RootToContentDir Style->RootToContentDir

TSharedPtr<FSlateStyleSet> FInworldVoicePreviewStyle::StyleInstance = nullptr;

void FInworldVoicePreviewStyle::Initialize()
{
	if (!StyleInstance.IsValid())
	{
		StyleInstance = Create();
		FSlateStyleRegistry::RegisterSlateStyle(*StyleInstance);
	}
}

void FInworldVoicePreviewStyle::Shutdown()
{
	FSlateStyleRegistry::UnRegisterSlateStyle(*StyleInstance);
	ensure(StyleInstance.IsUnique());
	StyleInstance.Reset();
}

FName FInworldVoicePreviewStyle::GetStyleSetName()
{
	static FName StyleSetName(TEXT("InworldVoicePreviewStyle"));
	return StyleSetName;
}

const FVector2D Icon16x16(16.0f, 16.0f);
const FVector2D Icon20x20(20.0f, 20.0f);

TSharedRef< FSlateStyleSet > FInworldVoicePreviewStyle::Create()
{
	TSharedRef< FSlateStyleSet > Style = MakeShareable(new FSlateStyleSet("InworldVoicePreviewStyle"));
	Style->SetContentRoot(IPluginManager::Get().FindPlugin("InworldVoicePreview")->GetBaseDir() / TEXT("Resources"));

	Style->Set("InworldVoicePreview.OpenPluginWindow", new IMAGE_BRUSH_SVG(TEXT("PlaceholderButtonIcon"), Icon20x20));

	return Style;
}

void FInworldVoicePreviewStyle::ReloadTextures()
{
	if (FSlateApplication::IsInitialized())
	{
		FSlateApplication::Get().GetRenderer()->ReloadTextureResources();
	}
}

const ISlateStyle& FInworldVoicePreviewStyle::Get()
{
	return *StyleInstance;
}
