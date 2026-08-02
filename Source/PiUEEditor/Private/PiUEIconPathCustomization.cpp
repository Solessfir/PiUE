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

struct FPiUEIconPickerItem
{
	FString AbsolutePath;
	FString StoredPath;
	FString Name;
	TUniquePtr<FSlateBrush> Brush;
	float BrushSize = 0.f;

	const FSlateBrush* GetBrush(const float DesiredSize)
	{
		if (!Brush.IsValid() || BrushSize != DesiredSize)
		{
			Brush = MakeUnique<FSlateVectorImageBrush>(AbsolutePath, FVector2D(DesiredSize, DesiredSize));
			BrushSize = DesiredSize;
		}
		return Brush.Get();
	}
};

namespace
{
	FString MakePortableIconPath(const FString& AbsolutePath)
	{
		FString NormalizedPath = AbsolutePath;
		FPaths::NormalizeFilename(NormalizedPath);

		const FString EngineContentDir = FPaths::ConvertRelativePathToFull(FPaths::EngineContentDir());
		if (FPaths::IsUnderDirectory(NormalizedPath, EngineContentDir))
		{
			FString RelativePath = NormalizedPath;
			if (FPaths::MakePathRelativeTo(RelativePath, *EngineContentDir))
			{
				FPaths::NormalizeFilename(RelativePath);
				return RelativePath;
			}
		}

		return NormalizedPath;
	}

	const TArray<TSharedPtr<FPiUEIconPickerItem>>& GetIconCatalog()
	{
		static TArray<TSharedPtr<FPiUEIconPickerItem>> Catalog;
		static bool bInitialized = false;
		if (bInitialized)
		{
			return Catalog;
		}
		bInitialized = true;

		const TArray<FString> SearchDirs =
		{
			FPaths::EngineDir() / TEXT("Content/Editor/Slate/Starship"),
			FPaths::EngineDir() / TEXT("Content/Slate/Starship"),
		};

		TSet<FString> UniquePaths;
		for (const FString& Dir : SearchDirs)
		{
			TArray<FString> Found;
			IFileManager::Get().FindFilesRecursive(Found, *Dir, TEXT("*.svg"), true, false);
			for (FString& Path : Found)
			{
				FPaths::NormalizeFilename(Path);
				if (UniquePaths.Contains(Path))
				{
					continue;
				}
				UniquePaths.Add(Path);

				const TSharedRef<FPiUEIconPickerItem> Item = MakeShared<FPiUEIconPickerItem>();
				Item->AbsolutePath = Path;
				Item->StoredPath = MakePortableIconPath(Path);
				Item->Name = FPaths::GetBaseFilename(Path);
				Catalog.Add(Item);
			}
		}

		Catalog.Sort([](const TSharedPtr<FPiUEIconPickerItem>& A, const TSharedPtr<FPiUEIconPickerItem>& B)
		{
			const int32 NameOrder = A->Name.Compare(B->Name, ESearchCase::IgnoreCase);
			return NameOrder == 0 ? A->StoredPath < B->StoredPath : NameOrder < 0;
		});

		return Catalog;
	}
}

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

TSharedRef<SWidget> FPiUEIconPathCustomization::BuildIconButton(const TSharedPtr<FPiUEIconPickerItem>& Item, const float IconSize)
{
	return SNew(SButton)
		.ButtonStyle(FAppStyle::Get(), "SimpleButton")
		.ToolTipText(FText::FromString(Item->Name))
		.OnClicked_Lambda([this, StoredPath = Item->StoredPath]() { OnIconSelected(StoredPath); return FReply::Handled(); })
		[
			SNew(SBox)
			.WidthOverride(IconSize)
			.HeightOverride(IconSize)
			[
				SNew(SImage).Image(Item->GetBrush(IconSize))
			]
		];
}

void FPiUEIconPathCustomization::RefreshVisibleIcons()
{
	if (!IconGrid.IsValid())
	{
		return;
	}

	IconGrid->ClearChildren();

	const FString Filter = SearchText.ToString();
	const float IconDim = GetDefault<UPiUESettings>()->IconPickerSize;

	for (const TSharedPtr<FPiUEIconPickerItem>& Item : GetIconCatalog())
	{
		if (Filter.IsEmpty() || Item->Name.Contains(Filter, ESearchCase::IgnoreCase))
		{
			IconGrid->AddSlot()[BuildIconButton(Item, IconDim)];
		}
	}
}

TSharedRef<SWidget> FPiUEIconPathCustomization::BuildMenuContent()
{
	SearchText = FText::GetEmpty();

	TSharedRef<SWidget> Content =
		SNew(SBox)
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
					SAssignNew(IconGrid, SUniformWrapPanel)
					.HAlign(HAlign_Left)
					.SlotPadding(FMargin(4.f))
				]
			]
		];

	RefreshVisibleIcons();

	return Content;
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
	RefreshVisibleIcons();
}

const FSlateBrush* FPiUEIconPathCustomization::GetPreviewBrush()
{
	FString CurrentPath;
	PathHandle->GetValue(CurrentPath);
	if (CurrentPath != CachedPreviewPath)
	{
		CachedPreviewPath = CurrentPath;
		FPiUEIconPath IconPath;
		IconPath.Path = CurrentPath;
		PreviewBrush = CurrentPath.IsEmpty() ? nullptr : MakeUnique<FSlateVectorImageBrush>(IconPath.ResolvePath(), FVector2D(16.f, 16.f));
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
