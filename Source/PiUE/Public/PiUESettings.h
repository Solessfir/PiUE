// Copyright Solessfir 2026. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Engine/DeveloperSettings.h"
#include "InstancedStruct.h"
#include "StructUtils/InstancedStruct.h"
#include "PiUESettings.generated.h"

USTRUCT()
struct PIUE_API FPiUEMenuRing
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, Config, Meta = (BaseStruct = "/Script/PiUE.PiUEMenuItemBase", ExcludeBaseStruct))
	TArray<FInstancedStruct> Items;

	/** When true, the ring can be summoned anywhere in the editor window, not only the level viewport. */
	UPROPERTY(EditAnywhere, Config)
	bool bAvailableAnywhere = false;
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

	bool IsRingAvailableAnywhere(const int32 RingIndex) const
	{
		switch (RingIndex)
		{
			case 0: return Ring1.bAvailableAnywhere;
			case 1: return Ring2.bAvailableAnywhere;
			case 2: return Ring3.bAvailableAnywhere;
			case 3: return Ring4.bAvailableAnywhere;
			case 4: return Ring5.bAvailableAnywhere;
			default: return false;
		}
	}

	/** Short press leaves menu open for click navigation. Long press executes hovered wedge on release. */
	UPROPERTY(EditAnywhere, Config, Meta = (ClampMin = 50.0, ClampMax = 1000.0, ForceUnits = "ms"), Category = "Input")
	double TapThreshold = 150.0;

	/** How long a category wedge must be hovered before auto-navigating into it. */
	UPROPERTY(EditAnywhere, Config, Meta = (ClampMin = 100.0, ClampMax = 5000.0, ForceUnits = "ms"), Category = "Input")
	double CategoryHoverMs = 1000.0;

	/** Radius of the wedge ring in screen pixels. */
	UPROPERTY(EditAnywhere, Config, Meta = (ClampMin = 40.f, ClampMax = 400.f), Category = "Layout")
	float MenuRadius = 120.f;

	/** Cursor distance from center below which nothing is selected. */
	UPROPERTY(EditAnywhere, Config, Meta = (ClampMin = 0.f, ClampMax = 100.f), Category = "Layout")
	float DeadZoneRadius = 25.f;

	/** Speed multiplier for wedge enter/exit translation animation. Higher = snappier. */
	UPROPERTY(EditAnywhere, Config, Meta = (ClampMin = 1.f, ClampMax = 50.f), Category = "Animation")
	float WedgeAnimSpeed = 25.f;

	/** Wedge background tint used when TintOverride alpha is zero. */
	UPROPERTY(EditAnywhere, Config, Category = "Style")
	FLinearColor DefaultWedgeTint = FLinearColor(0.05f, 0.05f, 0.05f, 0.85f);

	/** Wedge tint applied to the currently highlighted slot. */
	UPROPERTY(EditAnywhere, Config, Category = "Style")
	FLinearColor HighlightWedgeTint = FLinearColor(0.1f, 0.5f, 0.9f, 0.95f);

	// Begin UDeveloperSettings
	virtual FName GetContainerName() const override { return TEXT("Editor"); }
	virtual FName GetCategoryName() const override { return TEXT("Plugins"); }
	virtual FName GetSectionName() const override { return TEXT("PiUE"); }
	// End UDeveloperSettings
};
