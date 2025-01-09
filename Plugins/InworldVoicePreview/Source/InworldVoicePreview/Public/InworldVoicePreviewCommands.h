// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Framework/Commands/Commands.h"
#include "InworldVoicePreviewStyle.h"

class FInworldVoicePreviewCommands : public TCommands<FInworldVoicePreviewCommands>
{
public:

	FInworldVoicePreviewCommands()
		: TCommands<FInworldVoicePreviewCommands>(TEXT("InworldVoicePreview"), NSLOCTEXT("Contexts", "InworldVoicePreview", "InworldVoicePreview Plugin"), NAME_None, FInworldVoicePreviewStyle::GetStyleSetName())
	{
	}

	// TCommands<> interface
	virtual void RegisterCommands() override;

public:
	TSharedPtr< FUICommandInfo > OpenPluginWindow;
};