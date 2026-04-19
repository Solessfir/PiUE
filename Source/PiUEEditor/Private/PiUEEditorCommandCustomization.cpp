// Copyright Solessfir 2026. All Rights Reserved.

#include "PiUEEditorCommandCustomization.h"
#include "PiUETypes.h"
#include "DetailWidgetRow.h"
#include "Framework/Commands/InputBindingManager.h"
#include "Framework/Commands/UICommandInfo.h"
#include "IDetailChildrenBuilder.h"
#include "PropertyHandle.h"
#include "Styling/CoreStyle.h"
#include "Widgets/Input/SComboButton.h"
#include "Widgets/Input/SSearchBox.h"
#include "Widgets/Layout/SBox.h"
#include "Widgets/SBoxPanel.h"
#include "Widgets/Text/STextBlock.h"
#include "Widgets/Views/STableRow.h"

#define LOCTEXT_NAMESPACE "PiUEEditorCommandCustomization"

namespace
{
	constexpr float PickerButtonWidth = 280.f;
	constexpr float MenuWidth = 440.f;
	constexpr float MenuHeight = 500.f;
}

TSharedRef<IPropertyTypeCustomization> FPiUEEditorCommandCustomization::MakeInstance()
{
	return MakeShared<FPiUEEditorCommandCustomization>();
}

void FPiUEEditorCommandCustomization::CustomizeHeader(TSharedRef<IPropertyHandle> InStructPropertyHandle, FDetailWidgetRow& InHeaderRow, IPropertyTypeCustomizationUtils& InCustomizationUtils)
{
	CommandContextHandle = InStructPropertyHandle->GetChildHandle(GET_MEMBER_NAME_CHECKED(FPiUEEditorCommandItem, CommandContext));
	CommandNameHandle = InStructPropertyHandle->GetChildHandle(GET_MEMBER_NAME_CHECKED(FPiUEEditorCommandItem, CommandName));

	InHeaderRow
	.NameContent()
	.MinDesiredWidth(PickerButtonWidth)
	.MaxDesiredWidth(PickerButtonWidth)
	[
		SNew(SBox)
		.WidthOverride(PickerButtonWidth)
		[
			SAssignNew(CommandComboButton, SComboButton)
			.OnGetMenuContent(this, &FPiUEEditorCommandCustomization::BuildMenuContent)
			.ToolTipText(this, &FPiUEEditorCommandCustomization::GetSelectedCommandTooltip)
			.ButtonContent()
			[
				SNew(STextBlock)
				.Text(this, &FPiUEEditorCommandCustomization::GetSelectedCommandLabel)
				.OverflowPolicy(ETextOverflowPolicy::Ellipsis)
			]
		]
	];
}

void FPiUEEditorCommandCustomization::CustomizeChildren(TSharedRef<IPropertyHandle> InStructPropertyHandle, IDetailChildrenBuilder& InChildBuilder, IPropertyTypeCustomizationUtils& InCustomizationUtils)
{
	uint32 NumChildren = 0;
	InStructPropertyHandle->GetNumChildren(NumChildren);
	for (uint32 i = 0; i < NumChildren; ++i)
	{
		const TSharedPtr<IPropertyHandle> Child = InStructPropertyHandle->GetChildHandle(i);
		if (!Child.IsValid())
		{
			continue;
		}
		const FName PropName = Child->GetProperty()->GetFName();
		if (PropName == GET_MEMBER_NAME_CHECKED(FPiUEEditorCommandItem, CommandContext) || PropName == GET_MEMBER_NAME_CHECKED(FPiUEEditorCommandItem, CommandName))
		{
			continue;
		}
		InChildBuilder.AddProperty(Child.ToSharedRef());
	}
}

void FPiUEEditorCommandCustomization::BuildAllNodes()
{
	AllRootNodes.Reset();

	FInputBindingManager& BindingManager = FInputBindingManager::Get();

	TArray<TSharedPtr<FBindingContext>> Contexts;
	BindingManager.GetKnownInputContexts(Contexts);

	Contexts.Sort([](const TSharedPtr<FBindingContext>& A, const TSharedPtr<FBindingContext>& B)
	{
		return GetContextDisplayText(A).CompareTo(GetContextDisplayText(B)) < 0;
	});

	for (const TSharedPtr<FBindingContext>& Context : Contexts)
	{
		if (!Context.IsValid())
		{
			continue;
		}

		TArray<TSharedPtr<FUICommandInfo>> ContextCommands;
		BindingManager.GetCommandInfosFromContext(Context->GetContextName(), ContextCommands);

		if (ContextCommands.Num() == 0)
		{
			continue;
		}

		ContextCommands.Sort([](const TSharedPtr<FUICommandInfo>& A, const TSharedPtr<FUICommandInfo>& B)
		{
			return A->GetLabel().CompareTo(B->GetLabel()) < 0;
		});

		const TSharedRef<FPiUECommandPickerNode> ContextNode = MakeShared<FPiUECommandPickerNode>();
		ContextNode->Context = Context;

		for (const TSharedPtr<FUICommandInfo>& Command : ContextCommands)
		{
			const TSharedRef<FPiUECommandPickerNode> CommandNode = MakeShared<FPiUECommandPickerNode>();
			CommandNode->Command = Command;
			ContextNode->Children.Add(CommandNode);
		}

		AllRootNodes.Add(ContextNode);
	}
}

void FPiUEEditorCommandCustomization::RebuildVisibleRoots()
{
	VisibleRootNodes.Reset();

	const FString FilterString = SearchText.ToString();
	const bool bFilterEmpty = FilterString.IsEmpty();

	for (const TSharedPtr<FPiUECommandPickerNode>& ContextNode : AllRootNodes)
	{
		if (bFilterEmpty)
		{
			VisibleRootNodes.Add(ContextNode);
			continue;
		}

		const FString ContextLabel = GetContextDisplayText(ContextNode->Context).ToString();
		const bool bContextMatches = ContextLabel.Contains(FilterString);

		const TSharedRef<FPiUECommandPickerNode> FilteredContext = MakeShared<FPiUECommandPickerNode>();
		FilteredContext->Context = ContextNode->Context;

		for (const TSharedPtr<FPiUECommandPickerNode>& CommandNode : ContextNode->Children)
		{
			const FString CommandLabel = CommandNode->Command->GetLabel().ToString();
			if (bContextMatches || CommandLabel.Contains(FilterString))
			{
				FilteredContext->Children.Add(CommandNode);
			}
		}

		if (FilteredContext->Children.Num() > 0)
		{
			VisibleRootNodes.Add(FilteredContext);
		}
	}

	if (TreeView.IsValid())
	{
		TreeView->RequestTreeRefresh();

		if (!bFilterEmpty)
		{
			for (const TSharedPtr<FPiUECommandPickerNode>& Node : VisibleRootNodes)
			{
				TreeView->SetItemExpansion(Node, true);
			}
		}
	}
}

TSharedRef<SWidget> FPiUEEditorCommandCustomization::BuildMenuContent()
{
	SearchText = FText::GetEmpty();
	BuildAllNodes();
	RebuildVisibleRoots();

	return SNew(SBox)
	.WidthOverride(MenuWidth)
	.HeightOverride(MenuHeight)
	[
		SNew(SVerticalBox)
		+ SVerticalBox::Slot()
		.AutoHeight()
		.Padding(4.f)
		[
			SAssignNew(SearchBox, SSearchBox)
			.OnTextChanged(this, &FPiUEEditorCommandCustomization::OnSearchTextChanged)
		]
		+ SVerticalBox::Slot()
		.FillHeight(1.f)
		.Padding(4.f, 0.f, 4.f, 4.f)
		[
			SAssignNew(TreeView, STreeView<TSharedPtr<FPiUECommandPickerNode>>)
			.TreeItemsSource(&VisibleRootNodes)
			.OnGenerateRow(this, &FPiUEEditorCommandCustomization::OnGenerateRow)
			.OnGetChildren(this, &FPiUEEditorCommandCustomization::OnGetChildren)
			.OnSelectionChanged(this, &FPiUEEditorCommandCustomization::OnTreeSelectionChanged)
			.SelectionMode(ESelectionMode::Single)
		]
	];
}

TSharedRef<ITableRow> FPiUEEditorCommandCustomization::OnGenerateRow(TSharedPtr<FPiUECommandPickerNode> InNode, const TSharedRef<STableViewBase>& InOwnerTable)
{
	const bool bIsCommand = InNode->IsCommand();
	const FText RowText = bIsCommand ? InNode->Command->GetLabel() : GetContextDisplayText(InNode->Context);
	const FSlateFontInfo RowFont = FCoreStyle::GetDefaultFontStyle(bIsCommand ? "Regular" : "Bold", 9);

	TSharedRef<SHorizontalBox> RowBox = SNew(SHorizontalBox)
	+ SHorizontalBox::Slot()
	.FillWidth(1.f)
	.VAlign(VAlign_Center)
	[
		SNew(STextBlock)
		.Text(RowText)
		.Font(RowFont)
	];

	if (bIsCommand)
	{
		const TSharedRef<const FInputChord> Primary = InNode->Command->GetActiveChord(EMultipleKeyBindingIndex::Primary);
		const TSharedRef<const FInputChord> Secondary = InNode->Command->GetActiveChord(EMultipleKeyBindingIndex::Secondary);

		FString ChordText;
		if (Primary->IsValidChord())
		{
			ChordText = Primary->GetInputText().ToString();
		}
		if (Secondary->IsValidChord())
		{
			if (!ChordText.IsEmpty())
			{
				ChordText += TEXT("  /  ");
			}
			ChordText += Secondary->GetInputText().ToString();
		}

		if (!ChordText.IsEmpty())
		{
			RowBox->AddSlot()
			.AutoWidth()
			.VAlign(VAlign_Center)
			.Padding(8.f, 0.f, 0.f, 0.f)
			[
				SNew(STextBlock)
				.Text(FText::FromString(ChordText))
				.Font(RowFont)
				.ColorAndOpacity(FSlateColor::UseSubduedForeground())
			];
		}
	}

	return SNew(STableRow<TSharedPtr<FPiUECommandPickerNode>>, InOwnerTable)
	[
		RowBox
	];
}

void FPiUEEditorCommandCustomization::OnGetChildren(TSharedPtr<FPiUECommandPickerNode> InNode, TArray<TSharedPtr<FPiUECommandPickerNode>>& OutChildren)
{
	if (InNode.IsValid())
	{
		OutChildren.Append(InNode->Children);
	}
}

void FPiUEEditorCommandCustomization::OnTreeSelectionChanged(TSharedPtr<FPiUECommandPickerNode> InNode, ESelectInfo::Type InSelectInfo)
{
	if (InSelectInfo == ESelectInfo::Direct)
	{
		return;
	}

	if (!InNode.IsValid() || !InNode->IsCommand())
	{
		return;
	}

	CommandContextHandle->SetValue(InNode->Command->GetBindingContext());
	CommandNameHandle->SetValue(InNode->Command->GetCommandName());

	if (CommandComboButton.IsValid())
	{
		CommandComboButton->SetIsOpen(false);
	}
}

void FPiUEEditorCommandCustomization::OnSearchTextChanged(const FText& InText)
{
	SearchText = InText;
	RebuildVisibleRoots();
}

FText FPiUEEditorCommandCustomization::GetSelectedCommandLabel() const
{
	FName ContextName;
	FName CommandName;
	CommandContextHandle->GetValue(ContextName);
	CommandNameHandle->GetValue(CommandName);

	if (ContextName.IsNone() || CommandName.IsNone())
	{
		return LOCTEXT("SelectCommandPrompt", "Select command...");
	}

	const FInputBindingManager& BindingManager = FInputBindingManager::Get();
	const TSharedPtr<FUICommandInfo> Command = BindingManager.FindCommandInContext(ContextName, CommandName);
	if (Command.IsValid())
	{
		return Command->GetLabel();
	}

	return FText::Format(LOCTEXT("MissingCommandFmt", "{0} (missing)"), FText::FromName(CommandName));
}

FText FPiUEEditorCommandCustomization::GetSelectedCommandTooltip() const
{
	FName ContextName;
	FName CommandName;
	CommandContextHandle->GetValue(ContextName);
	CommandNameHandle->GetValue(CommandName);

	if (ContextName.IsNone() || CommandName.IsNone())
	{
		return LOCTEXT("SelectCommandTooltip", "Pick a command from the list.");
	}

	FInputBindingManager& BindingManager = FInputBindingManager::Get();
	const TSharedPtr<FBindingContext> Context = BindingManager.GetContextByName(ContextName);
	const FText ContextText = Context.IsValid() ? Context->GetContextDesc() : FText::FromName(ContextName);

	const TSharedPtr<FUICommandInfo> Command = BindingManager.FindCommandInContext(ContextName, CommandName);
	const FText CommandText = Command.IsValid() ? Command->GetLabel() : FText::FromName(CommandName);

	return FText::Format(LOCTEXT("CommandTooltipFmt", "{0}\n{1}"), ContextText, CommandText);
}

FText FPiUEEditorCommandCustomization::GetContextDisplayText(const TSharedPtr<FBindingContext>& InContext)
{
	if (!InContext.IsValid())
	{
		return FText::GetEmpty();
	}

	const FText Desc = InContext->GetContextDesc();
	if (!Desc.IsEmpty())
	{
		return Desc;
	}

	return FText::FromName(InContext->GetContextName());
}

#undef LOCTEXT_NAMESPACE
