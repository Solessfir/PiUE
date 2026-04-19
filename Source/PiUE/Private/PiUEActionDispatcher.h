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
	/**
	* Runs the appropriate action for the item.
	* Returns true on a successful dispatch, false if the item type is unsupported or the action cannot run.
	*/
	static bool Execute(const FInstancedStruct& Item);
};
