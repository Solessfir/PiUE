// Copyright Solessfir 2026. All Rights Reserved.

#pragma once

#include "Templates/SharedPointer.h"

class FUICommandInfo;

/**
 * Routes editor commands through the command lists PiUE can access reliably.
 * The editor picker uses the same mapping query, so it never offers commands
 * that the runtime dispatcher cannot execute.
 */
class PIUE_API FPiUECommandRouter
{
public:
	/** Returns true when at least one known editor command list maps this command. */
	static bool IsCommandMapped(const TSharedRef<const FUICommandInfo>& Command);

	/** Executes the command on the first known list where it is mapped and enabled. */
	static bool TryExecuteCommand(const TSharedRef<const FUICommandInfo>& Command);
};
