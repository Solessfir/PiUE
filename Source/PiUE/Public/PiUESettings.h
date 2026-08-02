// Copyright Solessfir 2026. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Engine/DeveloperSettings.h"
#include "StructUtils/InstancedStruct.h"
#include "PiUESettings.generated.h"

/** Configuration for one radial ring: its items and where it is allowed to open. */
USTRUCT()
struct PIUE_API FPiUEMenuRing
{
	GENERATED_BODY()

	/** Optional title drawn above the small center ring at the root level. Leave empty for no title. */
	UPROPERTY(EditAnywhere, Config, Category = "Menu")
	FText Title;

	/** When true, the ring only opens when the cursor is over the level viewport (or PIE viewport during play). When false, it opens in any editor window. */
	UPROPERTY(EditAnywhere, Config, Category = "Menu")
	bool bViewportOnly = true;

	// NoResetToDefault: engine crashes computing default-value diff for direct properties of a struct
	// held inside FInstancedStruct (FInstancedStructProvider::GetValueBaseAddress, dangling ScriptStruct).
	UPROPERTY(EditAnywhere, Config, Meta = (BaseStruct = "/Script/PiUE.PiUEMenuItemBase", ExcludeBaseStruct, NoResetToDefault), Category = "Menu")
	TArray<FInstancedStruct> Items;
};

/**
* PiUE configuration. Edited via Editor Preferences -> Plugins -> PiUE.
* Saves to Saved/Config/[Platform]/PiUE.ini (per-user, not checked in).
* Copy that file to Plugins/PiUE/Config/DefaultPiUE.ini to distribute defaults to all users.
* Ring summon keys are rebound via Editor Preferences -> General -> Keyboard Shortcuts -> PiUE.
*/
UCLASS(Config = "PiUE", Meta = (DisplayName = "PiUE"))
class PIUE_API UPiUESettings : public UDeveloperSettings
{
	GENERATED_BODY()

public:
	virtual FName GetContainerName() const override { return TEXT("Editor"); }

	virtual FName GetCategoryName() const override { return TEXT("Plugins"); }

	virtual FName GetSectionName() const override { return TEXT("PiUE"); }

	UPROPERTY(EditAnywhere, Config, Category = "Menu")
	FPiUEMenuRing Ring1;

	UPROPERTY(EditAnywhere, Config, Category = "Menu")
	FPiUEMenuRing Ring2;

	UPROPERTY(EditAnywhere, Config, Category = "Menu")
	FPiUEMenuRing Ring3;

	UPROPERTY(EditAnywhere, Config, Category = "Menu")
	FPiUEMenuRing Ring4;

	UPROPERTY(EditAnywhere, Config, Category = "Menu")
	FPiUEMenuRing Ring5;

	/** Returns pointer to the items array for the given ring index (0-4), or nullptr if out of range. */
	const TArray<FInstancedStruct>* GetRingItems(const int32 RingIndex) const
	{
		switch (RingIndex)
		{
			case 0: return &Ring1.Items;
			case 1: return &Ring2.Items;
			case 2: return &Ring3.Items;
			case 3: return &Ring4.Items;
			case 4: return &Ring5.Items;
			default: return nullptr;
		}
	}

	/** Returns the user-configured title for the given ring index (0-4), or empty text if out of range. */
	FText GetRingTitle(const int32 RingIndex) const
	{
		switch (RingIndex)
		{
			case 0: return Ring1.Title;
			case 1: return Ring2.Title;
			case 2: return Ring3.Title;
			case 3: return Ring4.Title;
			case 4: return Ring5.Title;
			default: return FText::GetEmpty();
		}
	}

	/** Returns true if the ring at the given index (0-4) is restricted to the level / PIE viewport. */
	bool IsRingViewportOnly(const int32 RingIndex) const
	{
		switch (RingIndex)
		{
			case 0: return Ring1.bViewportOnly;
			case 1: return Ring2.bViewportOnly;
			case 2: return Ring3.bViewportOnly;
			case 3: return Ring4.bViewportOnly;
			case 4: return Ring5.bViewportOnly;
			default: return true;
		}
	}

	/** Short press leaves menu open for click navigation. Long press executes hovered wedge on release. */
	UPROPERTY(EditAnywhere, Config, Meta = (ClampMin = 50.0, ClampMax = 1000.0, ForceUnits = "ms"), Category = "Input")
	double TapThreshold = 100.0;

	/** How long a category wedge must be hovered before auto-navigating into it. */
	UPROPERTY(EditAnywhere, Config, Meta = (ClampMin = 100.0, ClampMax = 5000.0, ForceUnits = "ms"), Category = "Input")
	double CategoryHoverMs = 300.0;

	/** Overall menu scale. 1.0 = native size; uniformly scales wedges, icons, text, and the hit-test region. */
	UPROPERTY(EditAnywhere, Config, Meta = (ClampMin = 0.5f, ClampMax = 3.f), Category = "Layout")
	float MenuScale = 1.f;

	/** Radius of the wedge ring in screen pixels. */
	UPROPERTY(EditAnywhere, Config, Meta = (ClampMin = 40.f, ClampMax = 400.f), Category = "Layout")
	float MenuRadius = 120.f;

	/** Cursor distance from center below which nothing is selected. */
	UPROPERTY(EditAnywhere, Config, Meta = (ClampMin = 0.f, ClampMax = 100.f), Category = "Layout")
	float DeadZoneRadius = 25.f;

	/** Duration of the wedge exit animation when the menu closes or navigates. */
	UPROPERTY(EditAnywhere, Config, Meta = (ClampMin = 50.f, ClampMax = 500.f, ForceUnits = "ms"), Category = "Animation")
	float WedgeExitDuration = 130.f;

	/** Speed multiplier for wedge enter/exit translation animation. Higher = snappier. */
	UPROPERTY(EditAnywhere, Config, Meta = (ClampMin = 1.f, ClampMax = 50.f), Category = "Animation")
	float WedgeAnimSpeed = 25.f;

	/** Speed multiplier for wedge highlight color transition. Higher = snappier. */
	UPROPERTY(EditAnywhere, Config, Meta = (ClampMin = 1.f, ClampMax = 50.f), Category = "Animation")
	float HighlightAnimSpeed = 20.f;

	/** Speed multiplier for the arc indicator tracking the hovered wedge. Higher = snappier. */
	UPROPERTY(EditAnywhere, Config, Meta = (ClampMin = 1.f, ClampMax = 100.f), Category = "Animation")
	float ArcTrackSpeed = 18.f;

	/** Speed multiplier for the arc indicator fade in/out. Higher = snappier. */
	UPROPERTY(EditAnywhere, Config, Meta = (ClampMin = 1.f, ClampMax = 50.f), Category = "Animation")
	float ArcFadeSpeed = 10.f;

	/** Size of icons in the editor icon picker grid. */
	UPROPERTY(EditAnywhere, Config, Meta = (ClampMin = 16.f, ClampMax = 64.f), Category = "Style")
	float IconPickerSize = 16.f;

	/** Wedge background tint used when TintOverride alpha is zero. */
	UPROPERTY(EditAnywhere, Config, Category = "Style")
	FLinearColor DefaultWedgeTint = FLinearColor(0.05f, 0.05f, 0.05f, 0.85f);

	/** Wedge tint applied to the currently highlighted slot. */
	UPROPERTY(EditAnywhere, Config, Category = "Style")
	FLinearColor HighlightWedgeTint = FLinearColor(0.1f, 0.5f, 0.9f, 0.95f);
};
