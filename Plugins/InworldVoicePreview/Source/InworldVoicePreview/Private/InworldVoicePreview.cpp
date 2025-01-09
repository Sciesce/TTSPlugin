// Copyright Epic Games, Inc. All Rights Reserved.

#include "InworldVoicePreview.h"
#include "InworldVoicePreviewStyle.h"
#include "InworldVoicePreviewCommands.h"
#include "LevelEditor.h"
#include "Widgets/Docking/SDockTab.h"
#include "Widgets/Layout/SBox.h"
#include "Widgets/Text/STextBlock.h"
#include "ToolMenus.h"
#include "Interfaces/IPluginManager.h"
#include "Blueprint/UserWidget.h"

static const FName InworldVoicePreviewTabName("InworldVoicePreview");

#define LOCTEXT_NAMESPACE "FInworldVoicePreviewModule"

void FInworldVoicePreviewModule::StartupModule()
{
	// This code will execute after your module is loaded into memory; the exact timing is specified in the .uplugin file per-module

	// Register the plugin's content directory
	FString PluginDir = IPluginManager::Get().FindPlugin(TEXT("InworldVoicePreview"))->GetBaseDir();
	FString ContentDir = PluginDir / TEXT("Content");
	FPackageName::RegisterMountPoint(TEXT("/InworldVoicePreview/"), ContentDir);
	
	FInworldVoicePreviewStyle::Initialize();
	FInworldVoicePreviewStyle::ReloadTextures();

	FInworldVoicePreviewCommands::Register();
	
	PluginCommands = MakeShareable(new FUICommandList);

	PluginCommands->MapAction(
		FInworldVoicePreviewCommands::Get().OpenPluginWindow,
		FExecuteAction::CreateRaw(this, &FInworldVoicePreviewModule::PluginButtonClicked),
		FCanExecuteAction());

	UToolMenus::RegisterStartupCallback(FSimpleMulticastDelegate::FDelegate::CreateRaw(this, &FInworldVoicePreviewModule::RegisterMenus));
	
	FGlobalTabmanager::Get()->RegisterNomadTabSpawner(InworldVoicePreviewTabName, FOnSpawnTab::CreateRaw(this, &FInworldVoicePreviewModule::OnSpawnPluginTab))
		.SetDisplayName(LOCTEXT("FInworldVoicePreviewTabTitle", "InworldVoicePreview"))
		.SetMenuType(ETabSpawnerMenuType::Enabled);
}

void FInworldVoicePreviewModule::ShutdownModule()
{
	// This function may be called during shutdown to clean up your module.  For modules that support dynamic reloading,
	// we call this function before unloading the module.

	UToolMenus::UnRegisterStartupCallback(this);

	UToolMenus::UnregisterOwner(this);

	FInworldVoicePreviewStyle::Shutdown();

	FInworldVoicePreviewCommands::Unregister();

	FGlobalTabmanager::Get()->UnregisterNomadTabSpawner(InworldVoicePreviewTabName);
}

TSharedRef<SDockTab> FInworldVoicePreviewModule::OnSpawnPluginTab(const FSpawnTabArgs& SpawnTabArgs)
{
	// Set up the path and register mount point
	if (TSharedPtr<IPlugin> Plugin = IPluginManager::Get().FindPlugin(TEXT("InworldVoicePreview")))
	{
		FString PluginDir = Plugin->GetBaseDir();
		FString ContentDir = PluginDir / TEXT("Content");
		FPackageName::RegisterMountPoint(TEXT("/InworldVoicePreview/"), ContentDir);
	}
    
	const FString WidgetBPPath = TEXT("/InworldVoicePreview/Widgets/APIWidget.APIWidget_C");
    
	if (!WidgetClass)
	{
		FSoftClassPath ClassPath(WidgetBPPath);
		WidgetClass = ClassPath.TryLoadClass<UUserWidget>();
		if (!WidgetClass)
		{
			UE_LOG(LogTemp, Error, TEXT("Failed to load widget class from: %s"), *WidgetBPPath);
		}
	}

	UUserWidget* Widget = nullptr;
	if (WidgetClass && GEditor)
	{
		Widget = CreateWidget<UUserWidget>(GEditor->GetEditorWorldContext().World(), WidgetClass);
		if (!Widget)
		{
			UE_LOG(LogTemp, Error, TEXT("Failed to create widget instance"));
		}
	}

	return SNew(SDockTab)
	.TabRole(ETabRole::NomadTab)
	[
		Widget ? Widget->TakeWidget() : SNullWidget::NullWidget
	];
		
}

void FInworldVoicePreviewModule::PluginButtonClicked()
{
	FGlobalTabmanager::Get()->TryInvokeTab(InworldVoicePreviewTabName);
}

void FInworldVoicePreviewModule::RegisterMenus()
{
	// Owner will be used for cleanup in call to UToolMenus::UnregisterOwner
	FToolMenuOwnerScoped OwnerScoped(this);

	{
		UToolMenu* Menu = UToolMenus::Get()->ExtendMenu("LevelEditor.MainMenu.Window");
		{
			FToolMenuSection& Section = Menu->FindOrAddSection("WindowLayout");
			Section.AddMenuEntryWithCommandList(FInworldVoicePreviewCommands::Get().OpenPluginWindow, PluginCommands);
		}
	}

	{
		UToolMenu* ToolbarMenu = UToolMenus::Get()->ExtendMenu("LevelEditor.LevelEditorToolBar");
		{
			FToolMenuSection& Section = ToolbarMenu->FindOrAddSection("Settings");
			{
				FToolMenuEntry& Entry = Section.AddEntry(FToolMenuEntry::InitToolBarButton(FInworldVoicePreviewCommands::Get().OpenPluginWindow));
				Entry.SetCommandList(PluginCommands);
			}
		}
	}
}

#undef LOCTEXT_NAMESPACE
	
IMPLEMENT_MODULE(FInworldVoicePreviewModule, InworldVoicePreview)