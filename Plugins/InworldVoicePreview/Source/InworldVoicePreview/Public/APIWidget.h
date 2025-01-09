// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "Components/TextBlock.h"
#include "Components/ComboBox.h"
#include "Components/Button.h"
#include "Components/ComboBoxString.h"
#include "Components/EditableTextBox.h"
#include "Engine/World.h"
#include "Kismet/GameplayStatics.h"
#include "Sound/SoundWave.h"
#include "Blueprint/UserWidget.h"
#include "Blueprint/WidgetTree.h"
#include "Components/CanvasPanel.h"
#include "Interfaces/IHttpRequest.h"
#include "Components/AudioComponent.h"
#include "Runtime/Engine/Classes/Sound/SoundWaveProcedural.h"
#include "HAL/UnrealMemory.h"

#include "APIWidget.generated.h"

class UImage;
class UEditableText;

USTRUCT()
struct FVoiceData
{
	GENERATED_BODY()

	FString Name;
	FString DisplayName;
	FString Gender;
	FString Age;
};

UCLASS()
class INWORLDVOICEPREVIEW_API UMyVoicePreviewWidget : public UUserWidget
{
	GENERATED_BODY()

	void NativeConstruct() override;


public:
	UFUNCTION(BlueprintCallable)
	void TestPlayWavFile();

	UPROPERTY(EditAnywhere, meta = (BindWidget))
	UCanvasPanel* CanvasPanel;

	UPROPERTY(EditAnywhere, meta = (BindWidget))
	UTextBlock* VoiceTitle;

	UPROPERTY(EditAnywhere, meta = (BindWidget))
	UTextBlock* GenderTitle;

	UPROPERTY(EditAnywhere, meta = (BindWidget))
	UTextBlock* AgeTitle;

	UPROPERTY(EditAnywhere, meta = (BindWidget))
	UComboBoxString* VoiceComboBox;

	UPROPERTY(EditAnywhere, meta = (BindWidget))
	UComboBoxString* GenderComboBox;

	UPROPERTY(EditAnywhere, meta = (BindWidget))
	UComboBoxString* AgeComboBox;

	UPROPERTY(EditAnywhere, meta = (BindWidget))
	UEditableText* PreviewText;

	UPROPERTY(EditAnywhere, meta = (BindWidget))
	UImage* TextBackgroundImage;

	UPROPERTY(EditAnywhere, meta = (BindWidget))
	UButton* PlayButton;

	UPROPERTY()
	USoundWaveProcedural* ProceduralSoundWave;

	FString SelectedVoice;

	FString HintText;

	void HandleVoiceSelectionChanged(const FString& SelectedItem, ESelectInfo::Type SelectInfo);

	virtual void BeginDestroy() override;
	
private:
	UPROPERTY()
	TArray<FVoiceData> VoiceDataArray;
	
	UPROPERTY()
	USoundWave* LastGeneratedSound;
	
	UPROPERTY()
	UAudioComponent* AudioComponent;

	void FetchVoices();
	void OnFetchVoicesResponse(FHttpRequestPtr Request, FHttpResponsePtr Response, bool bWasSuccessful);
	void UpdateVoiceOptions();
	void SendTTSRequest(const FString& Text, const FString& DisplayName);
	void OnTTSResponse(FHttpRequestPtr Request, FHttpResponsePtr Response, bool bWasSuccessful);
	USoundWave* CreateSoundWaveFromWavBytes(const TArray<uint8>& WavBytes);
	void PlayAudioData(USoundWave* SoundWave);

	UFUNCTION()
	void HandlePlayButtonPressed();

	UFUNCTION()
	void OnAudioPlaybackComplete();

	FString GetFullVoicePath(const FString& DisplayName);

	UFUNCTION()
	void OnGenderChanged(FString SelectedItem, ESelectInfo::Type SelectType);
    
	UFUNCTION()
	void OnAgeChanged(FString SelectedItem, ESelectInfo::Type SelectType);

	bool SaveWavToTemp(const TArray<uint8>& WavBytes, FString& OutFilePath);
	void CleanupTempFiles();
	USoundWave* CreateSoundWaveFromFile(const FString& FilePath);
};

