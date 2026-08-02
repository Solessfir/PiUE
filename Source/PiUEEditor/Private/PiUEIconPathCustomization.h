// Copyright Solessfir 2026. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "IPropertyTypeCustomization.h"

class IPropertyHandle;
class SComboButton;
class SSearchBox;
class SUniformWrapPanel;
struct FPiUEIconPickerItem;

/** Property type customization for FPiUEIconPath. Replaces the raw path string with a visual SVG icon grid picker. */
class FPiUEIconPathCustomization final : public IPropertyTypeCustomization
{
public:
	static TSharedRef<IPropertyTypeCustomization> MakeInstance();

	virtual void CustomizeHeader(TSharedRef<IPropertyHandle> InStructPropertyHandle, FDetailWidgetRow& InHeaderRow, IPropertyTypeCustomizationUtils& InCustomizationUtils) override;
	virtual void CustomizeChildren(TSharedRef<IPropertyHandle> InStructPropertyHandle, IDetailChildrenBuilder& InChildBuilder, IPropertyTypeCustomizationUtils& InCustomizationUtils) override;

private:
	TSharedRef<SWidget> BuildMenuContent();
	TSharedRef<SWidget> BuildIconButton(const TSharedPtr<FPiUEIconPickerItem>& Item, float IconSize);
	void RefreshVisibleIcons();
	void OnIconSelected(const FString& InPath);
	void OnSearchTextChanged(const FText& InText);
	const FSlateBrush* GetPreviewBrush();
	FText GetCurrentIconLabel() const;

	TSharedPtr<IPropertyHandle> PathHandle;
	TSharedPtr<SComboButton> ComboButton;
	TSharedPtr<SSearchBox> SearchBox;
	TSharedPtr<SUniformWrapPanel> IconGrid;
	FText SearchText;

	TUniquePtr<FSlateBrush> PreviewBrush;
	FString CachedPreviewPath;
};
