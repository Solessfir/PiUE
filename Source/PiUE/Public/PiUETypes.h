// Copyright Solessfir 2026. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "StructUtils/InstancedStruct.h"
#include "PiUETypes.generated.h"

class UEditorUtilityBlueprint;
class UEditorUtilityWidgetBlueprint;
struct FSlateBrush;

/** Bitmask flags controlling when an item is visible in the radial menu. */
UENUM(Meta = (Bitflags, UseEnumValuesAsMaskValuesInEditor = "true"))
enum class EPiUEItemMode : uint8
{
	None   = 0       UMETA(Hidden),
	Editor = 1 << 0  UMETA(DisplayName = "Editor"),
	PIE    = 1 << 1  UMETA(DisplayName = "PIE / Game"),
};

ENUM_CLASS_FLAGS(EPiUEItemMode);

/** Wrapper for a path to a Slate SVG icon. Displayed via a visual picker in the editor. */
USTRUCT(BlueprintType)
struct PIUE_API FPiUEIconPath
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, Category = "PiUE")
	FString Path;
};

/**
* Base class for all pie menu items. Subclass via USTRUCT and add via FInstancedStruct.
* Holds presentation data shared across every item type.
*/
USTRUCT(BlueprintType)
struct PIUE_API FPiUEMenuItemBase
{
	GENERATED_BODY()

	/** Executes the item's action. No-op for navigational items (Category, Close). */
	virtual void Execute() const {}

	virtual ~FPiUEMenuItemBase() = default;

	/** Text drawn on the wedge. Keep it short - long labels overflow. */
	UPROPERTY(EditAnywhere, Category = "PiUE")
	FText Label;

	/** Optional icon drawn beside the label. Select a Slate SVG via the icon picker. */
	UPROPERTY(EditAnywhere, Category = "PiUE")
	FPiUEIconPath Icon;

	/** Overrides the wedge background color. Unset = use theme default. */
	UPROPERTY(EditAnywhere, Category = "PiUE|Style")
	TOptional<FLinearColor> BackgroundTint;

	/** Renders the label in bold. */
	UPROPERTY(EditAnywhere, Category = "PiUE|Style")
	bool bBold = false;

	/** When this item is visible - Editor or PIE. Defaults to both. */
	UPROPERTY(EditAnywhere, Meta = (Bitmask, BitmaskEnum = "/Script/PiUE.EPiUEItemMode"), Category = "PiUE")
	uint8 Mode = static_cast<uint8>(EPiUEItemMode::Editor) | static_cast<uint8>(EPiUEItemMode::PIE);
};

/**
* Nested category. Hovering its wedge pushes a new pie onto the breadcrumb stack.
*/
USTRUCT(BlueprintType, DisplayName = "Category")
struct PIUE_API FPiUECategoryItem : public FPiUEMenuItemBase
{
	GENERATED_BODY()

	/** When true, this category's Label is drawn as the menu title above the center ring while inside it. */
	UPROPERTY(EditAnywhere, Meta = (DisplayAfter = "Label"), Category = "PiUE")
	bool bUseLabelAsTitle = false;

	/** Child items displayed when this category is entered. */
	UPROPERTY(EditAnywhere, Meta = (BaseStruct = "/Script/PiUE.PiUEMenuItemBase", ExcludeBaseStruct, DisplayAfter = "bUseLabelAsTitle"), Category = "PiUE")
	TArray<FInstancedStruct> Children;
};

/**
* Executes a registered editor command (FUICommandInfo) by context + name.
* Example: CommandContext = "LevelEditor", CommandName = "PlayInViewport".
*/
USTRUCT(BlueprintType, DisplayName = "Editor Command")
struct PIUE_API FPiUEEditorCommandItem : public FPiUEMenuItemBase
{
	GENERATED_BODY()

	virtual void Execute() const override;

	/** Binding context that owns the command (e.g. "LevelEditor"). */
	UPROPERTY(EditAnywhere, Category = "PiUE")
	FName CommandContext = NAME_None;

	/** Command name within the context (e.g. "PlayInViewport"). */
	UPROPERTY(EditAnywhere, Category = "PiUE")
	FName CommandName = NAME_None;
};

/**
* Executes a console command string against the editor world.
* Example: "viewmode lit", "ShowFlag.Bounds 1", "stat fps".
*/
USTRUCT(BlueprintType, DisplayName = "Console Command")
struct PIUE_API FPiUEConsoleCommandItem : public FPiUEMenuItemBase
{
	GENERATED_BODY()

	virtual void Execute() const override;

	/** Console command string passed to GEngine->Exec. */
	UPROPERTY(EditAnywhere, Meta = (DisplayAfter = "Label"), Category = "PiUE")
	FString Command;
};

/**
* Runs an Editor Utility Object blueprint. Instantiates the object and calls its Run event.
*/
USTRUCT(BlueprintType, DisplayName = "Editor Utility Object")
struct PIUE_API FPiUEEditorUtilityObjectItem : public FPiUEMenuItemBase
{
	GENERATED_BODY()

	virtual void Execute() const override;

	/** Editor Utility Object Blueprint to instantiate and run when the wedge is selected. */
	UPROPERTY(EditAnywhere, Meta = (DisplayThumbnail = false, DisplayAfter = "Label"), Category = "PiUE")
	TSoftObjectPtr<UEditorUtilityBlueprint> Object;
};

/**
* Closes the current level of the pie menu. At root: closes the menu. In a sub-ring: navigates back one level.
* Place this anywhere in a Children array to control its wedge position.
*/
USTRUCT(BlueprintType, DisplayName = "Close")
struct PIUE_API FPiUECloseItem : public FPiUEMenuItemBase
{
	GENERATED_BODY()
};

/**
* Launches an Editor Utility Widget Blueprint as a new tab.
*/
USTRUCT(BlueprintType, DisplayName = "Editor Utility Widget")
struct PIUE_API FPiUEEditorUtilityItem : public FPiUEMenuItemBase
{
	GENERATED_BODY()

	virtual void Execute() const override;

	/** Editor Utility Widget Blueprint to spawn when the wedge is selected. */
	UPROPERTY(EditAnywhere, Meta = (DisplayThumbnail = false, DisplayAfter = "Label"), Category = "PiUE")
	TSoftObjectPtr<UEditorUtilityWidgetBlueprint> Widget;
};

/**
* Inline-expanding spread that emits one wedge for the Level plus N wedges for the currently-open
* asset editors and standalone tabs (Project Settings, Editor Preferences). Tabs are sorted by the
* engine's last-activation timestamp (same signal used by the Ctrl+Tab dialog). Closed tabs do not
* appear. Replaces itself with up to MaxCount consecutive wedges in the parent ring at its array
* position. No nested user-facing item types - generated wedges are runtime-only.
*/
USTRUCT(BlueprintType, DisplayName = "Open Tabs")
struct PIUE_API FPiUEOpenTabsItem : public FPiUEMenuItemBase
{
	GENERATED_BODY()

	/** Total wedges this item emits, including the Level wedge. Clamped 1-12. Default 6 = Level + up to 5 tabs. */
	UPROPERTY(EditAnywhere, Meta = (ClampMin = 1, ClampMax = 12, DisplayAfter = "Label"), Category = "PiUE")
	int32 MaxCount = 6;

	/** When false: Level first, tabs newest-to-oldest after. When true: tabs oldest-to-newest, Level last. */
	UPROPERTY(EditAnywhere, Category = "PiUE")
	bool bReversed = false;

	/** When true, generated wedges show the source tab's icon next to the label. */
	UPROPERTY(EditAnywhere, Category = "PiUE")
	bool bShowIcons = true;
};
