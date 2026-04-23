#pragma once

#include "CoreMinimal.h"
#include "Widgets/SCompoundWidget.h"

class SUsdPreviewPanel : public SCompoundWidget
{
public:
	SLATE_BEGIN_ARGS(SUsdPreviewPanel) {}
	SLATE_END_ARGS()

	void Construct(const FArguments& InArgs);

private:
	FReply OnChooseUsdClicked();
	FReply OnLoadUsdClicked();
	FReply OnApplyConsistentViewClicked();
	FText GetSelectedUsdText() const;

	bool SpawnOrUpdateUsdPreviewActor(const FString& UsdFilePath);
	void ShowNotification(const FText& Message, bool bSuccess) const;

	FString SelectedUsdPath;
};
