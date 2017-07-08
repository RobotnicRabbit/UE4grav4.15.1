//Copyright 2016-2017 Mookie
#pragma once

#include "ModuleManager.h"

class FSixDOFMovementModule : public IModuleInterface
{
public:

	/** IModuleInterface implementation */
	virtual void StartupModule() override;
	virtual void ShutdownModule() override;
};

