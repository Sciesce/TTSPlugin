// Copyright Epic Games, Inc. All Rights Reserved.

#include "InworldVoicePreviewCommands.h"

#define LOCTEXT_NAMESPACE "FInworldVoicePreviewModule"

void FInworldVoicePreviewCommands::RegisterCommands()
{
	UI_COMMAND(OpenPluginWindow, "InworldVoicePreview", "Bring up InworldVoicePreview window", EUserInterfaceActionType::Button, FInputChord());
}

#undef LOCTEXT_NAMESPACE
