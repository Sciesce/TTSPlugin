#include "APIWidget.h"
#include "Http.h"
#include "Components/EditableText.h"
#include "Json.h"
#include "JsonUtilities.h"
#include "HAL/PlatformFileManager.h"
#include "Misc/FileHelper.h"

void UMyVoicePreviewWidget::NativeConstruct()
{
    Super::NativeConstruct();
    
    // Initialize combo boxes
    if (GenderComboBox)
    {
        GenderComboBox->ClearOptions();
        GenderComboBox->AddOption(TEXT("All"));
        GenderComboBox->AddOption(TEXT("Male"));
        GenderComboBox->AddOption(TEXT("Female"));
        GenderComboBox->AddOption(TEXT("Neutral"));
        GenderComboBox->SetSelectedIndex(0);
    }
    GenderComboBox->OnSelectionChanged.AddDynamic(this, &UMyVoicePreviewWidget::OnGenderChanged);
    
    if (AgeComboBox)
    {
        AgeComboBox->ClearOptions();
        AgeComboBox->AddOption(TEXT("All"));
        AgeComboBox->AddOption(TEXT("Child"));
        AgeComboBox->AddOption(TEXT("Teen"));
        AgeComboBox->AddOption(TEXT("Adult"));
        AgeComboBox->AddOption(TEXT("Senior"));
        AgeComboBox->SetSelectedIndex(0);
    }
    AgeComboBox->OnSelectionChanged.AddDynamic(this, &UMyVoicePreviewWidget::OnAgeChanged);

    if (VoiceComboBox)
    {
        VoiceComboBox->AddOption(TEXT("Loading..."));
    }

    if (PlayButton)
    {
        PlayButton->OnClicked.AddDynamic(this, &UMyVoicePreviewWidget::HandlePlayButtonPressed);
    }

    // Start fetching voices
    FetchVoices();
}

void UMyVoicePreviewWidget::FetchVoices()
{
    TSharedRef<IHttpRequest, ESPMode::ThreadSafe> HttpRequest = FHttpModule::Get().CreateRequest();
    
    const FString ApiKey = TEXT("YTJqb2QyREFQWm9rUEg3WVB5U0o3d0RSTlRzTzFlOHo6WlpiendzV1RLbXhnQlVpYWFsUFUxN3JydEJhZlkyaEljT05kaDdzcGlDRnhCdjlSd1hlOFRtdE9jS0pCWGgyWg==");
    const FString Url = TEXT("https://api.inworld.ai/tts/v1alpha/voices");
    
    HttpRequest->SetURL(Url);
    HttpRequest->SetVerb(TEXT("GET"));
    HttpRequest->SetHeader(TEXT("Authorization"), FString::Printf(TEXT("Basic %s"), *ApiKey));
    HttpRequest->SetHeader(TEXT("Content-Type"), TEXT("application/json"));
    
    HttpRequest->OnProcessRequestComplete().BindUObject(this, &UMyVoicePreviewWidget::OnFetchVoicesResponse);
    HttpRequest->ProcessRequest();
}

void UMyVoicePreviewWidget::OnFetchVoicesResponse(FHttpRequestPtr Request, FHttpResponsePtr Response, bool bWasSuccessful)
{
    if (bWasSuccessful && Response.IsValid())
    {
        const FString ResponseString = Response->GetContentAsString();
        
        TSharedPtr<FJsonObject> JsonObject;
        TSharedRef<TJsonReader<>> JsonReader = TJsonReaderFactory<>::Create(ResponseString);
        
        if (FJsonSerializer::Deserialize(JsonReader, JsonObject) && JsonObject.IsValid())
        {
            const TArray<TSharedPtr<FJsonValue>>* VoicesArray;
            if (JsonObject->TryGetArrayField(TEXT("voices"), VoicesArray))
            {
                VoiceDataArray.Empty();
                
                for (const TSharedPtr<FJsonValue>& Value : *VoicesArray)
                {
                    const TSharedPtr<FJsonObject> VoiceObject = Value->AsObject();
                    if (VoiceObject.IsValid())
                    {
                        FVoiceData NewVoiceData;
                        
                        VoiceObject->TryGetStringField(TEXT("name"), NewVoiceData.Name);
                        
                        const TSharedPtr<FJsonObject>* VoiceMetadataObj;
                        if (VoiceObject->TryGetObjectField(TEXT("voiceMetadata"), VoiceMetadataObj))
                        {
                            FString DisplayName;
                            if (!(*VoiceMetadataObj)->TryGetStringField(TEXT("displayName"), DisplayName))
                            {
                                FString NamePart;
                                if (NewVoiceData.Name.Split(TEXT("/"), nullptr, &NamePart, ESearchCase::CaseSensitive, ESearchDir::FromEnd))
                                {
                                    NewVoiceData.DisplayName = NamePart;
                                }
                                else
                                {
                                    NewVoiceData.DisplayName = NewVoiceData.Name;
                                }
                            }
                            else
                            {
                                NewVoiceData.DisplayName = DisplayName;
                            }
                            
                            (*VoiceMetadataObj)->TryGetStringField(TEXT("age"), NewVoiceData.Age);
                            (*VoiceMetadataObj)->TryGetStringField(TEXT("gender"), NewVoiceData.Gender);
                        }
                        
                        VoiceDataArray.Add(NewVoiceData);
                    }
                }
                
                UpdateVoiceOptions();
            }
        }
    }
}

void UMyVoicePreviewWidget::UpdateVoiceOptions()
{
    if (!VoiceComboBox || !GenderComboBox || !AgeComboBox)
    {
        return;
    }

    VoiceComboBox->ClearOptions();

    FString SelectedGender = GenderComboBox->GetSelectedOption();
    FString SelectedAge = AgeComboBox->GetSelectedOption();

    for (const FVoiceData& Voice : VoiceDataArray)
    {
        bool bMatchesGender = (SelectedGender == TEXT("All") || Voice.Gender == SelectedGender);
        bool bMatchesAge = (SelectedAge == TEXT("All") || Voice.Age == SelectedAge);

        if (bMatchesGender && bMatchesAge)
        {
            VoiceComboBox->AddOption(Voice.DisplayName);
        }
    }

    if (VoiceComboBox->GetOptionCount() > 0)
    {
        VoiceComboBox->SetSelectedIndex(0);
    }
}

void UMyVoicePreviewWidget::HandleVoiceSelectionChanged(const FString& SelectedItem, ESelectInfo::Type SelectInfo)
{
    SelectedVoice = SelectedItem;
}

void UMyVoicePreviewWidget::HandlePlayButtonPressed()
{
    if (!PreviewText || VoiceComboBox->GetSelectedOption().IsEmpty())
    {
        return;
    }

    FString PreviewString = PreviewText->GetText().ToString();
    if (!PreviewString.IsEmpty())
    {
        SendTTSRequest(PreviewString, VoiceComboBox->GetSelectedOption());
    }
}

void UMyVoicePreviewWidget::SendTTSRequest(const FString& Text, const FString& DisplayName)
{
    FString FullVoicePath = GetFullVoicePath(DisplayName);
    FString CleanedText = Text.TrimStartAndEnd();
    
    if (CleanedText.IsEmpty())
    {
        return;
    }

    TSharedRef<IHttpRequest, ESPMode::ThreadSafe> HttpRequest = FHttpModule::Get().CreateRequest();
    
    TSharedPtr<FJsonObject> JsonObject = MakeShared<FJsonObject>();
    TSharedPtr<FJsonObject> InputObject = MakeShared<FJsonObject>();
    TSharedPtr<FJsonObject> VoiceObject = MakeShared<FJsonObject>();
    
    InputObject->SetStringField(TEXT("text"), CleanedText);
    VoiceObject->SetStringField(TEXT("name"), FullVoicePath);
    
    JsonObject->SetObjectField(TEXT("input"), InputObject);
    JsonObject->SetObjectField(TEXT("voice"), VoiceObject);
    
    FString JsonString;
    TSharedRef<TJsonWriter<TCHAR, TPrettyJsonPrintPolicy<TCHAR>>> Writer = 
        TJsonWriterFactory<TCHAR, TPrettyJsonPrintPolicy<TCHAR>>::Create(&JsonString);
    FJsonSerializer::Serialize(JsonObject.ToSharedRef(), Writer);

    const FString ApiKey = TEXT("YTJqb2QyREFQWm9rUEg3WVB5U0o3d0RSTlRzTzFlOHo6WlpiendzV1RLbXhnQlVpYWFsUFUxN3JydEJhZlkyaEljT05kaDdzcGlDRnhCdjlSd1hlOFRtdE9jS0pCWGgyWg==");
    const FString Url = TEXT("https://api.inworld.ai/tts/v1alpha/text:synthesize-sync");
    
    HttpRequest->SetURL(Url);
    HttpRequest->SetVerb(TEXT("POST"));
    HttpRequest->SetHeader(TEXT("Authorization"), FString::Printf(TEXT("Basic %s"), *ApiKey));
    HttpRequest->SetHeader(TEXT("Content-Type"), TEXT("application/json"));
    HttpRequest->SetContentAsString(JsonString);
    
    HttpRequest->OnProcessRequestComplete().BindUObject(this, &UMyVoicePreviewWidget::OnTTSResponse);
    HttpRequest->ProcessRequest();
}

FString UMyVoicePreviewWidget::GetFullVoicePath(const FString& DisplayName)
{
    for (const FVoiceData& Voice : VoiceDataArray)
    {
        if (Voice.DisplayName == DisplayName)
        {
            return Voice.Name;
        }
    }
    return DisplayName;
}

bool UMyVoicePreviewWidget::SaveWavToTemp(const TArray<uint8>& WavBytes, FString& OutFilePath)
{
    // Create a unique filename in the project's saved directory
    FString TempPath = FPaths::Combine(FPaths::ProjectSavedDir(), TEXT("InworldTemp"));
    // Ensure the directory exists
    FPlatformFileManager::Get().GetPlatformFile().CreateDirectoryTree(*TempPath);
    
    FString Timestamp = FDateTime::Now().ToString(TEXT("%Y%m%d_%H%M%S"));
    FString Filename = FString::Printf(TEXT("InworldPreview_%s.wav"), *Timestamp);
    OutFilePath = FPaths::Combine(TempPath, Filename);

    // Save the WAV bytes to the file
    bool bSuccess = FFileHelper::SaveArrayToFile(WavBytes, *OutFilePath);
    if (bSuccess)
    {
        UE_LOG(LogTemp, Log, TEXT("Successfully saved WAV file to: %s"), *OutFilePath);
    }
    else
    {
        UE_LOG(LogTemp, Error, TEXT("Failed to save WAV file to: %s"), *OutFilePath);
    }

    return bSuccess;
}

void UMyVoicePreviewWidget::CleanupTempFiles()
{
    // Get temp directory
    FString TempPath = FPaths::Combine(FPaths::ProjectSavedDir(), TEXT("InworldTemp"));
    
    // Find and delete all temp files matching our prefix that are older than 1 hour
    TArray<FString> FoundFiles;
    IFileManager::Get().FindFiles(FoundFiles, 
        *(TempPath / TEXT("InworldPreview_*.wav")), 
        true, 
        false);

    for (const FString& File : FoundFiles)
    {
        FString FullPath = FPaths::Combine(TempPath, File);
        FDateTime FileTime = IFileManager::Get().GetTimeStamp(*FullPath);
        if ((FDateTime::Now() - FileTime).GetTotalHours() > 1)
        {
            IFileManager::Get().Delete(*FullPath);
            UE_LOG(LogTemp, Log, TEXT("Cleaned up temp file: %s"), *FullPath);
        }
    }
}

void UMyVoicePreviewWidget::OnTTSResponse(FHttpRequestPtr Request, FHttpResponsePtr Response, bool bWasSuccessful)
{
    if (!bWasSuccessful || !Response.IsValid())
    {
        UE_LOG(LogTemp, Error, TEXT("TTS API request failed"));
        return;
    }

    TSharedPtr<FJsonObject> JsonObject;
    TSharedRef<TJsonReader<>> JsonReader = TJsonReaderFactory<>::Create(Response->GetContentAsString());
    
    if (!FJsonSerializer::Deserialize(JsonReader, JsonObject) || !JsonObject.IsValid())
    {
        UE_LOG(LogTemp, Error, TEXT("Failed to parse TTS response"));
        return;
    }

    FString Base64Audio;
    if (JsonObject->TryGetStringField(TEXT("audioContent"), Base64Audio))
    {
        TArray<uint8> WavBytes;
        if (FBase64::Decode(Base64Audio, WavBytes))
        {
            // Save to temp file for verification
            FString TempFilePath;
            if (SaveWavToTemp(WavBytes, TempFilePath))
            {
                UE_LOG(LogTemp, Log, TEXT("Saved WAV to: %s"), *TempFilePath);
            }

            // Create and play sound wave
            if (USoundWave* SoundWave = CreateSoundWaveFromWavBytes(WavBytes))
            {
                AsyncTask(ENamedThreads::GameThread, [this, SoundWave]()
                {
                    PlayAudioData(SoundWave);
                });
            }
            else
            {
                UE_LOG(LogTemp, Error, TEXT("Failed to create SoundWave"));
            }
        }
    }
}

USoundWave* UMyVoicePreviewWidget::CreateSoundWaveFromFile(const FString& FilePath)
{
    // Load the WAV file into a byte array
    TArray<uint8> WavBytes;
    if (!FFileHelper::LoadFileToArray(WavBytes, *FilePath))
    {
        UE_LOG(LogTemp, Error, TEXT("Failed to load WAV file from: %s"), *FilePath);
        return nullptr;
    }

    USoundWave* SoundWave = NewObject<USoundWave>(GetTransientPackage());
    if (!SoundWave)
    {
        return nullptr;
    }

    // Parse WAV header
    if (WavBytes.Num() <= 44)
    {
        return nullptr;
    }

    // Read WAV header information
    uint32 SampleRate = *reinterpret_cast<const uint32*>(&WavBytes[24]);
    uint16 NumChannels = *reinterpret_cast<const uint16*>(&WavBytes[22]);
    uint16 BitsPerSample = *reinterpret_cast<const uint16*>(&WavBytes[34]);
    uint32 DataSize = *reinterpret_cast<const uint32*>(&WavBytes[40]);

    // Configure sound wave
    SoundWave->SetSampleRate(SampleRate);
    SoundWave->NumChannels = NumChannels;
    SoundWave->RawPCMDataSize = DataSize;
    SoundWave->Duration = static_cast<float>(DataSize) / (SampleRate * NumChannels * (BitsPerSample / 8));
    SoundWave->bLooping = false;

    // Copy audio data (skip WAV header)
    SoundWave->RawPCMData = (uint8*)FMemory::Malloc(DataSize);
    FMemory::Memcpy(SoundWave->RawPCMData, WavBytes.GetData() + 44, DataSize);

    // Add to root to prevent garbage collection
    SoundWave->AddToRoot();

    return SoundWave;
}

USoundWave* UMyVoicePreviewWidget::CreateSoundWaveFromWavBytes(const TArray<uint8>& WavBytes)
{
    if (WavBytes.Num() <= 44)
    {
        UE_LOG(LogTemp, Error, TEXT("WAV data too short"));
        return nullptr;
    }

    // Create a procedural sound wave
    ProceduralSoundWave = NewObject<USoundWaveProcedural>(GetTransientPackage());
    if (!ProceduralSoundWave)
    {
        return nullptr;
    }

    const uint8* WavData = WavBytes.GetData();
    
    // Verify RIFF header
    if (FMemory::Memcmp(WavData, "RIFF", 4) != 0 || FMemory::Memcmp(WavData + 8, "WAVE", 4) != 0)
    {
        UE_LOG(LogTemp, Error, TEXT("Invalid WAV header"));
        return nullptr;
    }

    // Parse WAV header
    uint16 NumChannels = *reinterpret_cast<const uint16*>(WavData + 22);
    uint32 SampleRate = *reinterpret_cast<const uint32*>(WavData + 24);
    uint16 BitsPerSample = *reinterpret_cast<const uint16*>(WavData + 34);

    // Find data chunk
    int32 DataOffset = -1;
    int32 DataSize = 0;
    
    for (int32 i = 12; i < WavBytes.Num() - 8; ++i)
    {
        if (FMemory::Memcmp(WavData + i, "data", 4) == 0)
        {
            DataOffset = i + 8;  // Skip "data" and chunk size
            DataSize = *reinterpret_cast<const int32*>(WavData + i + 4);
            break;
        }
    }

    if (DataOffset == -1 || DataSize == 0)
    {
        UE_LOG(LogTemp, Error, TEXT("Failed to find WAV data chunk"));
        return nullptr;
    }

    // Configure procedural sound wave
    ProceduralSoundWave->SetSampleRate(SampleRate);
    ProceduralSoundWave->NumChannels = NumChannels;
    ProceduralSoundWave->Duration = static_cast<float>(DataSize) / (SampleRate * NumChannels * (BitsPerSample / 8));
    ProceduralSoundWave->bLooping = false;
    ProceduralSoundWave->bCanProcessAsync = true;
    ProceduralSoundWave->bProcedural = true;

    // Copy PCM data
    const uint8* PCMData = WavData + DataOffset;
    TArray<uint8> PCMBuffer;
    PCMBuffer.Append(PCMData, DataSize);
    ProceduralSoundWave->QueueAudio(PCMBuffer.GetData(), DataSize);

    UE_LOG(LogTemp, Log, TEXT("Created procedural sound wave - Duration: %f, Channels: %d, Sample Rate: %d"), 
        ProceduralSoundWave->Duration, ProceduralSoundWave->NumChannels, SampleRate);

    return ProceduralSoundWave;
}

void UMyVoicePreviewWidget::PlayAudioData(USoundWave* SoundWave)
{
    if (!SoundWave || !GetWorld())
    {
        UE_LOG(LogTemp, Error, TEXT("Invalid SoundWave or World"));
        return;
    }

    // Clean up previous sound
    if (LastGeneratedSound && LastGeneratedSound != SoundWave)
    {
        LastGeneratedSound->RemoveFromRoot();
        LastGeneratedSound = nullptr;
    }

    // Store new sound
    LastGeneratedSound = SoundWave;
    LastGeneratedSound->AddToRoot();

    // Create a new audio component
    if (!AudioComponent || !AudioComponent->IsValidLowLevel())
    {
        AudioComponent = NewObject<UAudioComponent>(GetTransientPackage());
        AudioComponent->bAutoDestroy = false;
        AudioComponent->bAllowSpatialization = false;
        AudioComponent->AddToRoot();
    }

    // Set up audio component
    AudioComponent->Sound = LastGeneratedSound;
    AudioComponent->bIsUISound = true;
    AudioComponent->VolumeMultiplier = 1.0f;
    AudioComponent->PitchMultiplier = 1.0f;
    AudioComponent->bAlwaysPlay = true;
    AudioComponent->OnAudioFinished.AddDynamic(this, &UMyVoicePreviewWidget::OnAudioPlaybackComplete);

    // Ensure we stop any currently playing sound
    if (AudioComponent->IsPlaying())
    {
        AudioComponent->Stop();
    }

    // Play the sound
    AudioComponent->Play();

    UE_LOG(LogTemp, Log, TEXT("Playing audio via component - Duration: %f, Channels: %d"), 
        LastGeneratedSound->Duration,
        LastGeneratedSound->NumChannels);
}



void UMyVoicePreviewWidget::OnGenderChanged(FString SelectedItem, ESelectInfo::Type SelectType)
{
    UpdateVoiceOptions();
}

void UMyVoicePreviewWidget::OnAgeChanged(FString SelectedItem, ESelectInfo::Type SelectType)
{
    UpdateVoiceOptions();
}

void UMyVoicePreviewWidget::OnAudioPlaybackComplete()
{
    if (AudioComponent && AudioComponent->GetOwner())
    {
        AActor* Owner = AudioComponent->GetOwner();
        AudioComponent->DestroyComponent();
        AudioComponent = nullptr;
        Owner->Destroy();
    }
}

void UMyVoicePreviewWidget::TestPlayWavFile()
{
    // Load the sound wave asset
    USoundWave* TestSound = LoadObject<USoundWave>(nullptr, TEXT("/Game/DrumLoop"));
    
    if (!TestSound)
    {
        UE_LOG(LogTemp, Error, TEXT("Failed to load test sound file"));
        return;
    }

    UE_LOG(LogTemp, Log, TEXT("Test sound loaded - Duration: %f, Channels: %d, Sound Valid: %d"), 
        TestSound->Duration, TestSound->NumChannels, IsValid(TestSound));

    if (UWorld* World = GetWorld())
    {
        float VolumeMultiplier = 1.0f;
        float PitchMultiplier = 1.0f;
        float StartTime = 0.0f;
        
        UGameplayStatics::PlaySound2D(
            World,
            TestSound,
            VolumeMultiplier,
            PitchMultiplier,
            StartTime
        );
        
        UE_LOG(LogTemp, Log, TEXT("Attempted to play sound via PlaySound2D"));
    }
    else
    {
        UE_LOG(LogTemp, Error, TEXT("Failed to get World pointer"));
    }
}

void UMyVoicePreviewWidget::BeginDestroy()
{
    if (ProceduralSoundWave)
    {
        ProceduralSoundWave->RemoveFromRoot();
        ProceduralSoundWave = nullptr;
    }

    if (LastGeneratedSound)
    {
        LastGeneratedSound->RemoveFromRoot();
        LastGeneratedSound = nullptr;
    }

    if (AudioComponent)
    {
        AudioComponent->Stop();
        AudioComponent->RemoveFromRoot();
        AudioComponent = nullptr;
    }

    Super::BeginDestroy();
}