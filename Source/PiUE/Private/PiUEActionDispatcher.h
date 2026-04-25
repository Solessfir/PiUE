// Copyright Solessfir 2026. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"

struct FInstancedStruct;

/**
* Executes pie menu items based on their FInstancedStruct script-struct type.
*/
class FPiUEActionDispatcher
{
public:
	/** Runs the item's Execute action. No-op for invalid or navigational items. */
	static void Execute(const FInstancedStruct& Item);
};
