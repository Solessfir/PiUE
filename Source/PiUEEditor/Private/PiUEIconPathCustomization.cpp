// Copyright Solessfir 2026. All Rights Reserved.

#include "PiUEIconPathCustomization.h"
#include "PiUESettings.h"
#include "PiUETypes.h"
#include "Brushes/SlateImageBrush.h"
#include "DetailWidgetRow.h"
#include "HAL/FileManager.h"
#include "Misc/Paths.h"
#include "PropertyHandle.h"
#include "Styling/AppStyle.h"
#include "Styling/CoreStyle.h"
#include "Widgets/Images/SImage.h"
#include "Widgets/Input/SButton.h"
#include "Widgets/Input/SComboButton.h"
#include "Widgets/Input/SSearchBox.h"
#include "Widgets/Layout/SBox.h"
#include "Widgets/Layout/SScrollBox.h"
#include "Widgets/Layout/SUniformWrapPanel.h"
#include "Widgets/SBoxPanel.h"
#include "Widgets/Text/STextBlock.h"
#include "PiUEEditorConstants.h"

#define LOCTEXT_NAMESPACE "PiUEIconPathCustomization"

using namespace PiUEEditor;

TSharedRef<IPropertyTypeCustomization> FPiUEIconPathCustomization::MakeInstance()
{
	return MakeShared<FPiUEIconPathCustomization>();
}

void FPiUEIconPathCustomization::CustomizeHeader(TSharedRef<IPropertyHandle> InStructPropertyHandle, FDetailWidgetRow& InHeaderRow, IPropertyTypeCustomizationUtils& InCustomizationUtils)
{
	PathHandle = InStructPropertyHandle->GetChildHandle(GET_MEMBER_NAME_CHECKED(FPiUEIconPath, Path));

	InHeaderRow
	.NameContent()
	[
		InStructPropertyHandle->CreatePropertyNameWidget()
	]
	.ValueContent()
	.MinDesiredWidth(PickerButtonWidth)
	.MaxDesiredWidth(PickerButtonWidth)
	[
		SNew(SBox)
		.WidthOverride(PickerButtonWidth)
		[
			SAssignNew(ComboButton, SComboButton)
			.OnGetMenuContent(this, &FPiUEIconPathCustomization::BuildMenuContent)
			.ButtonContent()
			[
				SNew(SHorizontalBox)
				+ SHorizontalBox::Slot()
				.AutoWidth()
				.VAlign(VAlign_Center)
				.Padding(0.f, 0.f, 4.f, 0.f)
				[
					SNew(SBox)
					.WidthOverride(16.f)
					.HeightOverride(16.f)
					.Visibility_Lambda([this]() -> EVisibility
					{
						FString CurrentPath;
						PathHandle->GetValue(CurrentPath);
						return CurrentPath.IsEmpty() ? EVisibility::Collapsed : EVisibility::Visible;
					})
					[
						SNew(SImage)
						.Image_Lambda([this]() -> const FSlateBrush* { return GetPreviewBrush(); })
					]
				]
				+ SHorizontalBox::Slot()
				.FillWidth(1.f)
				.VAlign(VAlign_Center)
				[
					SNew(STextBlock)
					.Text(this, &FPiUEIconPathCustomization::GetCurrentIconLabel)
					.Font(FCoreStyle::GetDefaultFontStyle("Regular", 9))
					.OverflowPolicy(ETextOverflowPolicy::Ellipsis)
				]
			]
		]
	];
}

void FPiUEIconPathCustomization::CustomizeChildren(TSharedRef<IPropertyHandle> InStructPropertyHandle, IDetailChildrenBuilder& InChildBuilder, IPropertyTypeCustomizationUtils& InCustomizationUtils)
{
	// Intentionally empty - combo button in CustomizeHeader is the complete interface. Exposing raw Path string would be redundant.
}

void FPiUEIconPathCustomization::ScanIcons()
{
	AllIconPaths.Reset();

	const TArray<FString> SearchDirs =
	{
		FPaths::EngineDir() / TEXT("Content/Editor/Slate/Starship"),
		FPaths::EngineDir() / TEXT("Content/Slate/Starship"),
	};

	for (const FString& Dir : SearchDirs)
	{
		TArray<FString> Found;
		IFileManager::Get().FindFilesRecursive(Found, *Dir, TEXT("*.svg"), true, false);
		AllIconPaths.Append(Found);
	}
}

TSharedRef<SWidget> FPiUEIconPathCustomization::BuildIconButton(const FString& Path, const FSlateBrush* Brush, const float IconSize)
{
	const FString Name = FPaths::GetBaseFilename(Path);
	return SNew(SButton)
		.ButtonStyle(FAppStyle::Get(), "SimpleButton")
		.ToolTipText(FText::FromString(Name))
		.OnClicked_Lambda([this, Path]() { OnIconSelected(Path); return FReply::Handled(); })
		.Visibility_Lambda([this, Name]() -> EVisibility
		{
			return SearchText.IsEmpty() || Name.Contains(SearchText.ToString(), ESearchCase::IgnoreCase) ? EVisibility::Visible : EVisibility::Collapsed;
		})
		[
			SNew(SBox)
			.WidthOverride(IconSize)
			.HeightOverride(IconSize)
			[
				SNew(SImage).Image(Brush)
			]
		];
}

TSharedRef<SWidget> FPiUEIconPathCustomization::BuildIconGrid()
{
	if (CachedIconGrid.IsValid())
	{
		return CachedIconGrid.ToSharedRef();
	}

	if (AllIconPaths.IsEmpty())
	{
		ScanIcons();
	}

	PickerBrushes.Reset();
	PickerBrushes.Reserve(AllIconPaths.Num());

	const float IconDim = GetDefault<UPiUESettings>()->IconPickerSize;
	SAssignNew(IconGrid, SUniformWrapPanel)
	.HAlign(HAlign_Left)
	.SlotPadding(FMargin(4.f));

	for (const FString& Path : AllIconPaths)
	{
		PickerBrushes.Add(MakeUnique<FSlateVectorImageBrush>(Path, FVector2D(IconDim, IconDim)));
		IconGrid->AddSlot()[BuildIconButton(Path, PickerBrushes.Last().Get(), IconDim)];
	}

	CachedIconGrid = IconGrid;
	return CachedIconGrid.ToSharedRef();
}

TSharedRef<SWidget> FPiUEIconPathCustomization::BuildMenuContent()
{
	SearchText = FText::GetEmpty();
	if (CachedIconGrid.IsValid())
	{
		IconGrid->Invalidate(EInvalidateWidgetReason::Layout);
	}

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
			.OnTextChanged(this, &FPiUEIconPathCustomization::OnSearchTextChanged)
		]
		+ SVerticalBox::Slot()
		.FillHeight(1.f)
		.Padding(4.f, 0.f, 4.f, 4.f)
		[
			SNew(SScrollBox)
			+ SScrollBox::Slot()
			[
				BuildIconGrid()
			]
		]
	];
}

void FPiUEIconPathCustomization::OnIconSelected(const FString& InPath)
{
	PathHandle->SetValue(InPath);
	if (ComboButton.IsValid())
	{
		ComboButton->SetIsOpen(false);
	}
}

void FPiUEIconPathCustomization::OnSearchTextChanged(const FText& InText)
{
	SearchText = InText;
	if (IconGrid.IsValid())
	{
		IconGrid->Invalidate(EInvalidateWidgetReason::Layout);
	}
}

const FSlateBrush* FPiUEIconPathCustomization::GetPreviewBrush()
{
	FString CurrentPath;
	PathHandle->GetValue(CurrentPath);
	if (CurrentPath != CachedPreviewPath)
	{
		CachedPreviewPath = CurrentPath;
		PreviewBrush = CurrentPath.IsEmpty() ? nullptr : MakeUnique<FSlateVectorImageBrush>(CurrentPath, FVector2D(16.f, 16.f));
	}

	return PreviewBrush.Get();
}

FText FPiUEIconPathCustomization::GetCurrentIconLabel() const
{
	FString Path;
	PathHandle->GetValue(Path);
	if (Path.IsEmpty())
	{
		return LOCTEXT("SelectIconPrompt", "Select icon...");
	}

	return FText::FromString(FPaths::GetBaseFilename(Path));
}

#undef LOCTEXT_NAMESPACE
