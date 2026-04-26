#include "SUsdPreviewPanel.h"

#include "DesktopPlatformModule.h"
#include "Editor.h"
#include "GameFramework/Actor.h"
#include "Engine/World.h"
#include "EngineUtils.h"
#include "Framework/Notifications/NotificationManager.h"
#include "Styling/AppStyle.h"
#include "HAL/IConsoleManager.h"
#include "IDesktopPlatform.h"
#include "Misc/Paths.h"
#include "UObject/FieldIterator.h"
#include "UObject/UnrealType.h"
#include "Widgets/Input/SButton.h"
#include "Widgets/Layout/SBox.h"
#include "Widgets/Layout/SSeparator.h"
#include "Widgets/Layout/SUniformGridPanel.h"
#include "Widgets/SBoxPanel.h"
#include "Widgets/Notifications/SNotificationList.h"
#include "Widgets/Text/STextBlock.h"
#include "Logging/LogMacros.h"

DEFINE_LOG_CATEGORY_STATIC(LogUSDPreviewPanel, Log, All);

namespace USDPreviewPanelPrivate
{
	static const FName PreviewActorName(TEXT("USDPreview_StageActor"));
}

void SUsdPreviewPanel::Construct(const FArguments& InArgs)
{
	ChildSlot
	[
		SNew(SVerticalBox)
		+ SVerticalBox::Slot()
		.AutoHeight()
		.Padding(8.0f)
		[
			SNew(STextBlock)
			.Text(FText::FromString(TEXT("USD Preview (Editor Only)")))
		]
		+ SVerticalBox::Slot()
		.AutoHeight()
		.Padding(8.0f, 0.0f, 8.0f, 8.0f)
		[
			SNew(STextBlock)
			.Text(this, &SUsdPreviewPanel::GetSelectedUsdText)
		]
		+ SVerticalBox::Slot()
		.AutoHeight()
		.Padding(8.0f, 0.0f)
		[
			SNew(SUniformGridPanel)
			.SlotPadding(FMargin(4.0f))
			+ SUniformGridPanel::Slot(0, 0)
			[
				SNew(SButton)
				.Text(FText::FromString(TEXT("Choose USD File")))
				.OnClicked(this, &SUsdPreviewPanel::OnChooseUsdClicked)
			]
			+ SUniformGridPanel::Slot(1, 0)
			[
				SNew(SButton)
				.Text(FText::FromString(TEXT("Load to Preview")))
				.OnClicked(this, &SUsdPreviewPanel::OnLoadUsdClicked)
			]
		]
		+ SVerticalBox::Slot()
		.AutoHeight()
		.Padding(8.0f)
		[
			SNew(SSeparator)
		]
		+ SVerticalBox::Slot()
		.AutoHeight()
		.Padding(8.0f, 0.0f, 8.0f, 8.0f)
		[
			SNew(SButton)
			.Text(FText::FromString(TEXT("Apply Consistent View")))
			.ToolTipText(FText::FromString(TEXT("Disable auto exposure and force stable tone mapping for consistent visual checks.")))
			.OnClicked(this, &SUsdPreviewPanel::OnApplyConsistentViewClicked)
		]
	];
}

FReply SUsdPreviewPanel::OnChooseUsdClicked()
{
	IDesktopPlatform* DesktopPlatform = FDesktopPlatformModule::Get();
	if (!DesktopPlatform)
	{
		ShowNotification(FText::FromString(TEXT("Desktop platform service unavailable.")), false);
		return FReply::Handled();
	}

	void* ParentWindowHandle = nullptr;
	TArray<FString> OutFiles;
	const bool bOpened = DesktopPlatform->OpenFileDialog(
		ParentWindowHandle,
		TEXT("Choose USD File"),
		FPaths::ProjectDir(),
		TEXT(""),
		TEXT("USD Files (*.usd;*.usda;*.usdc)|*.usd;*.usda;*.usdc"),
		EFileDialogFlags::None,
		OutFiles
	);

	if (bOpened && OutFiles.Num() > 0)
	{
		SelectedUsdPath = OutFiles[0];
		ShowNotification(FText::Format(FText::FromString(TEXT("Selected: {0}")), FText::FromString(SelectedUsdPath)), true);
	}

	return FReply::Handled();
}

FReply SUsdPreviewPanel::OnLoadUsdClicked()
{
	if (SelectedUsdPath.IsEmpty())
	{
		ShowNotification(FText::FromString(TEXT("Choose a USD file first.")), false);
		return FReply::Handled();
	}

	if (SpawnOrUpdateUsdPreviewActor(SelectedUsdPath))
	{
		ShowNotification(FText::FromString(TEXT("USD preview actor loaded/updated.")), true);
	}
	else
	{
		ShowNotification(FText::FromString(TEXT("Failed to load preview actor. Ensure USD plugins are enabled in UE.")), false);
	}

	return FReply::Handled();
}

FReply SUsdPreviewPanel::OnApplyConsistentViewClicked()
{
	auto SetCVarInt = [](const TCHAR* Name, int32 Value)
	{
		if (IConsoleVariable* Var = IConsoleManager::Get().FindConsoleVariable(Name))
		{
			Var->Set(Value, ECVF_SetByCode);
		}
	};

	auto SetCVarFloat = [](const TCHAR* Name, float Value)
	{
		if (IConsoleVariable* Var = IConsoleManager::Get().FindConsoleVariable(Name))
		{
			Var->Set(Value, ECVF_SetByCode);
		}
	};

	SetCVarInt(TEXT("r.EyeAdaptationQuality"), 0);
	SetCVarInt(TEXT("r.DefaultFeature.AutoExposure"), 0);
	SetCVarFloat(TEXT("r.ExposureOffset"), 0.0f);
	SetCVarInt(TEXT("r.TonemapperFilm"), 1);

	if (GEditor)
	{
		GEditor->RedrawAllViewports(true);
	}

	ShowNotification(FText::FromString(TEXT("Consistent view settings applied.")), true);
	return FReply::Handled();
}

FText SUsdPreviewPanel::GetSelectedUsdText() const
{
	if (SelectedUsdPath.IsEmpty())
	{
		return FText::FromString(TEXT("No USD file selected."));
	}
	return FText::FromString(FString::Printf(TEXT("Selected: %s"), *SelectedUsdPath));
}

bool SUsdPreviewPanel::SpawnOrUpdateUsdPreviewActor(const FString& UsdFilePath)
{
	if (!GEditor)
	{
		UE_LOG(LogUSDPreviewPanel, Error, TEXT("GEditor is null."));
		return false;
	}

	const FString NormalizedUsdPath = FPaths::ConvertRelativePathToFull(UsdFilePath);
	if (!FPaths::FileExists(NormalizedUsdPath))
	{
		UE_LOG(LogUSDPreviewPanel, Error, TEXT("USD file does not exist: %s"), *NormalizedUsdPath);
		return false;
	}

	UWorld* World = GEditor->GetEditorWorldContext().World();
	if (!World)
	{
		UE_LOG(LogUSDPreviewPanel, Error, TEXT("Editor world is null."));
		return false;
	}

	UClass* UsdActorClass = FindObject<UClass>(nullptr, TEXT("/Script/USDStage.UsdStageActor"));
	if (!UsdActorClass)
	{
		UsdActorClass = LoadObject<UClass>(nullptr, TEXT("/Script/USDStage.UsdStageActor"));
	}
	if (!UsdActorClass)
	{
		UE_LOG(LogUSDPreviewPanel, Error, TEXT("Could not find /Script/USDStage.UsdStageActor. Ensure USDStage plugin is enabled."));
		return false;
	}

	AActor* TargetActor = nullptr;
	for (TActorIterator<AActor> It(World, UsdActorClass); It; ++It)
	{
		if (IsValid(*It) && It->GetFName() == USDPreviewPanelPrivate::PreviewActorName)
		{
			TargetActor = *It;
			break;
		}
	}

	if (!TargetActor)
	{
		FActorSpawnParameters SpawnParams;
		SpawnParams.Name = USDPreviewPanelPrivate::PreviewActorName;
		TargetActor = World->SpawnActor<AActor>(UsdActorClass, FTransform::Identity, SpawnParams);
	}
	if (!TargetActor)
	{
		UE_LOG(LogUSDPreviewPanel, Error, TEXT("Failed to spawn USD preview actor."));
		return false;
	}

	bool bSetPathProperty = false;
	UFunction* SetRootLayerFunction = UsdActorClass->FindFunctionByName(TEXT("SetRootLayer"));
	if (SetRootLayerFunction)
	{
		for (TFieldIterator<FProperty> ParamIt(SetRootLayerFunction); ParamIt; ++ParamIt)
		{
			FProperty* Param = *ParamIt;
			if (!Param->HasAnyPropertyFlags(CPF_Parm) || Param->HasAnyPropertyFlags(CPF_ReturnParm))
			{
				continue;
			}

			TArray<uint8> ParamsBuffer;
			ParamsBuffer.SetNumZeroed(SetRootLayerFunction->ParmsSize);

			if (FStrProperty* StringParam = CastField<FStrProperty>(Param))
			{
				StringParam->SetPropertyValue_InContainer(ParamsBuffer.GetData(), NormalizedUsdPath);
				TargetActor->ProcessEvent(SetRootLayerFunction, ParamsBuffer.GetData());
				bSetPathProperty = true;
			}
			else if (FStructProperty* StructParam = CastField<FStructProperty>(Param))
			{
				if (StructParam->Struct && StructParam->Struct->GetFName() == TEXT("FilePath"))
				{
					void* StructData = StructParam->ContainerPtrToValuePtr<void>(ParamsBuffer.GetData());
					if (StructData)
					{
						if (FStrProperty* InnerFilePathProp = FindFProperty<FStrProperty>(StructParam->Struct, TEXT("FilePath")))
						{
							void* InnerFilePathData = InnerFilePathProp->ContainerPtrToValuePtr<void>(StructData);
							InnerFilePathProp->SetPropertyValue(InnerFilePathData, NormalizedUsdPath);
							TargetActor->ProcessEvent(SetRootLayerFunction, ParamsBuffer.GetData());
							bSetPathProperty = true;
						}
					}
				}
			}

			break;
		}
	}

	bool bFoundCandidateProperty = false;
	for (TFieldIterator<FProperty> PropIt(UsdActorClass); PropIt && !bSetPathProperty; ++PropIt)
	{
		FProperty* Property = *PropIt;
		const FName PropName = Property->GetFName();
		if (PropName != TEXT("RootLayer") && PropName != TEXT("FilePath"))
		{
			continue;
		}

		bFoundCandidateProperty = true;
		if (FStrProperty* StringProperty = CastField<FStrProperty>(Property))
		{
			StringProperty->SetPropertyValue_InContainer(TargetActor, NormalizedUsdPath);
			bSetPathProperty = true;
			break;
		}

		// Newer UE versions may expose RootLayer as a FFilePath struct.
		if (FStructProperty* StructProperty = CastField<FStructProperty>(Property))
		{
			if (StructProperty->Struct && StructProperty->Struct->GetFName() == TEXT("FilePath"))
			{
				void* StructData = StructProperty->ContainerPtrToValuePtr<void>(TargetActor);
				if (StructData)
				{
					if (FStrProperty* InnerFilePathProp = FindFProperty<FStrProperty>(StructProperty->Struct, TEXT("FilePath")))
					{
						void* InnerFilePathData = InnerFilePathProp->ContainerPtrToValuePtr<void>(StructData);
						InnerFilePathProp->SetPropertyValue(InnerFilePathData, NormalizedUsdPath);
						bSetPathProperty = true;
						break;
					}
				}
			}
		}
	}

	if (!bFoundCandidateProperty)
	{
		UE_LOG(LogUSDPreviewPanel, Error, TEXT("UsdStageActor has no RootLayer/FilePath property in this UE version."));
	}
	else if (!bSetPathProperty)
	{
		UE_LOG(LogUSDPreviewPanel, Error, TEXT("Found RootLayer/FilePath but could not set USD path due to unsupported property type."));
	}

	TargetActor->Modify();
	TargetActor->RerunConstructionScripts();
	TargetActor->PostEditChange();
	if (UFunction* ReloadStageFunction = UsdActorClass->FindFunctionByName(TEXT("ReloadAnimations")))
	{
		TargetActor->ProcessEvent(ReloadStageFunction, nullptr);
	}
	if (UFunction* LoadStageFunction = UsdActorClass->FindFunctionByName(TEXT("LoadUsdStage")))
	{
		TargetActor->ProcessEvent(LoadStageFunction, nullptr);
	}
	if (UFunction* OpenStageFunction = UsdActorClass->FindFunctionByName(TEXT("OpenUsdStage")))
	{
		TargetActor->ProcessEvent(OpenStageFunction, nullptr);
	}

	if (GEditor)
	{
		GEditor->SelectNone(false, true, false);
		GEditor->SelectActor(TargetActor, true, true);
		GEditor->RedrawAllViewports(true);
	}

	return bSetPathProperty;
}

void SUsdPreviewPanel::ShowNotification(const FText& Message, bool bSuccess) const
{
	FNotificationInfo Info(Message);
	Info.bFireAndForget = true;
	Info.ExpireDuration = bSuccess ? 3.0f : 5.0f;
	Info.bUseSuccessFailIcons = true;
	Info.Image = bSuccess ? FAppStyle::Get().GetBrush("Icons.SuccessWithColor") : FAppStyle::Get().GetBrush("Icons.ErrorWithColor");
	FSlateNotificationManager::Get().AddNotification(Info);
}
