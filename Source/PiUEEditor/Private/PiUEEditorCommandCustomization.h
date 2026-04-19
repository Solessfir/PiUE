// Copyright Solessfir 2026. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "IPropertyTypeCustomization.h"
#include "Widgets/Views/STreeView.h"

class FBindingContext;
class FUICommandInfo;
class IPropertyHandle;
class SComboButton;
class SSearchBox;

struct FPiUECommandPickerNode
{
	TSharedPtr<FBindingContext> Context;
	TSharedPtr<FUICommandInfo> Command;
	TArray<TSharedPtr<FPiUECommandPickerNode>> Children;

	bool IsCommand() const { return Command.IsValid(); }
};

class FPiUEEditorCommandCustomization final : public IPropertyTypeCustomization
{
public:
	static TSharedRef<IPropertyTypeCustomization> MakeInstance();

	virtual void CustomizeHeader(TSharedRef<IPropertyHandle> InStructPropertyHandle, FDetailWidgetRow& InHeaderRow, IPropertyTypeCustomizationUtils& InCustomizationUtils) override;
	virtual void CustomizeChildren(TSharedRef<IPropertyHandle> InStructPropertyHandle, IDetailChildrenBuilder& InChildBuilder, IPropertyTypeCustomizationUtils& InCustomizationUtils) override;

private:
	void BuildAllNodes();
	void RebuildVisibleRoots();
	TSharedRef<SWidget> BuildMenuContent();
	TSharedRef<ITableRow> OnGenerateRow(TSharedPtr<FPiUECommandPickerNode> InNode, const TSharedRef<STableViewBase>& InOwnerTable);
	void OnGetChildren(TSharedPtr<FPiUECommandPickerNode> InNode, TArray<TSharedPtr<FPiUECommandPickerNode>>& OutChildren);
	void OnTreeSelectionChanged(TSharedPtr<FPiUECommandPickerNode> InNode, ESelectInfo::Type InSelectInfo);
	void OnSearchTextChanged(const FText& InText);
	FText GetSelectedCommandLabel() const;
	FText GetSelectedCommandTooltip() const;
	static FText GetContextDisplayText(const TSharedPtr<FBindingContext>& InContext);

	TSharedPtr<IPropertyHandle> CommandContextHandle;
	TSharedPtr<IPropertyHandle> CommandNameHandle;
	TArray<TSharedPtr<FPiUECommandPickerNode>> AllRootNodes;
	TArray<TSharedPtr<FPiUECommandPickerNode>> VisibleRootNodes;
	TSharedPtr<SComboButton> CommandComboButton;
	TSharedPtr<SSearchBox> SearchBox;
	TSharedPtr<STreeView<TSharedPtr<FPiUECommandPickerNode>>> TreeView;
	FText SearchText;
};
