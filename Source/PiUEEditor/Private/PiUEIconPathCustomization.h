// Copyright Solessfir 2026. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "IPropertyTypeCustomization.h"

class IPropertyHandle;
class SComboButton;
class SSearchBox;
class SUniformWrapPanel;

class FPiUEIconPathCustomization final : public IPropertyTypeCustomization
{
public:
	static TSharedRef<IPropertyTypeCustomization> MakeInstance();

	virtual void CustomizeHeader(TSharedRef<IPropertyHandle> InStructPropertyHandle, FDetailWidgetRow& InHeaderRow, IPropertyTypeCustomizationUtils& InCustomizationUtils) override;
	virtual void CustomizeChildren(TSharedRef<IPropertyHandle> InStructPropertyHandle, IDetailChildrenBuilder& InChildBuilder, IPropertyTypeCustomizationUtils& InCustomizationUtils) override;

private:
	void ScanIcons();
	TSharedRef<SWidget> BuildMenuContent();
	void OnIconSelected(const FString& InPath);
	void OnSearchTextChanged(const FText& InText);
	const FSlateBrush* GetPreviewBrush();
	FText GetCurrentIconLabel() const;

	TSharedPtr<IPropertyHandle> PathHandle;
	TSharedPtr<SComboButton> ComboButton;
	TSharedPtr<SSearchBox> SearchBox;
	TSharedPtr<SUniformWrapPanel> IconGrid;
	FText SearchText;

	TArray<FString> AllIconPaths;
	TArray<TUniquePtr<FSlateBrush>> PickerBrushes;
	TUniquePtr<FSlateBrush> PreviewBrush;
	FString CachedPreviewPath;
};
