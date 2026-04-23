#include "USDPreviewEditorModule.h"

#include "SUsdPreviewPanel.h"
#include "ToolMenus.h"
#include "Widgets/Docking/SDockTab.h"
#include "Framework/Docking/TabManager.h"
#include "LevelEditor.h"

static const FName USDPreviewTabName(TEXT("USDPreview"));

#define LOCTEXT_NAMESPACE "FUSDPreviewEditorModule"

void FUSDPreviewEditorModule::StartupModule()
{
	FGlobalTabmanager::Get()->RegisterNomadTabSpawner(
		USDPreviewTabName,
		FOnSpawnTab::CreateRaw(this, &FUSDPreviewEditorModule::OnSpawnPluginTab))
		.SetDisplayName(LOCTEXT("USDPreviewTabTitle", "USD Preview"))
		.SetMenuType(ETabSpawnerMenuType::Hidden);

	UToolMenus::RegisterStartupCallback(
		FSimpleMulticastDelegate::FDelegate::CreateRaw(this, &FUSDPreviewEditorModule::RegisterMenus));
}

void FUSDPreviewEditorModule::ShutdownModule()
{
	UToolMenus::UnRegisterStartupCallback(this);
	UToolMenus::UnregisterOwner(this);
	FGlobalTabmanager::Get()->UnregisterNomadTabSpawner(USDPreviewTabName);
}

void FUSDPreviewEditorModule::PluginButtonClicked()
{
	FGlobalTabmanager::Get()->TryInvokeTab(USDPreviewTabName);
}

TSharedRef<SDockTab> FUSDPreviewEditorModule::OnSpawnPluginTab(const FSpawnTabArgs& SpawnTabArgs)
{
	return SNew(SDockTab)
		.TabRole(ETabRole::NomadTab)
		[
			SNew(SUsdPreviewPanel)
		];
}

void FUSDPreviewEditorModule::RegisterMenus()
{
	FToolMenuOwnerScoped OwnerScoped(this);

	if (UToolMenu* Menu = UToolMenus::Get()->ExtendMenu("LevelEditor.MainMenu.Window"))
	{
		FToolMenuSection& Section = Menu->FindOrAddSection("WindowLayout");
		Section.AddMenuEntry(
			"USDPreview_OpenWindow",
			LOCTEXT("USDPreview_OpenWindow_Label", "USD Preview"),
			LOCTEXT("USDPreview_OpenWindow_Tooltip", "Open USD preview panel."),
			FSlateIcon(),
			FUIAction(FExecuteAction::CreateRaw(this, &FUSDPreviewEditorModule::PluginButtonClicked))
		);
	}

	if (UToolMenu* ToolbarMenu = UToolMenus::Get()->ExtendMenu("LevelEditor.LevelEditorToolBar.User"))
	{
		FToolMenuSection& Section = ToolbarMenu->FindOrAddSection("USDPreview");
		FToolMenuEntry Entry = FToolMenuEntry::InitToolBarButton(
			"USDPreview_OpenWindow",
			FUIAction(FExecuteAction::CreateRaw(this, &FUSDPreviewEditorModule::PluginButtonClicked)),
			LOCTEXT("USDPreview_ToolbarLabel", "USD Preview"),
			LOCTEXT("USDPreview_ToolbarTooltip", "Open USD preview panel."),
			FSlateIcon()
		);
		Section.AddEntry(Entry);
	}
}

#undef LOCTEXT_NAMESPACE

IMPLEMENT_MODULE(FUSDPreviewEditorModule, USDPreviewEditor)
