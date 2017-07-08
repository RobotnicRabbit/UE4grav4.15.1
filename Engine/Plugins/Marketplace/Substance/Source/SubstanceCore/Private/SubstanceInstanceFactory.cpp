// Copyright 2016 Allegorithmic Inc. All rights reserved.
// File: SubstanceInstanceFactory.cpp

#include "SubstanceCorePrivatePCH.h"
#include <substance/framework/package.h>

#include "SubstanceInstanceFactory.h"
#include "SubstanceTexture2D.h"
#include "SubstanceSettings.h"
#include "SubstanceCoreHelpers.h"
#include "SubstanceGraphInstance.h"
#include "SubstanceStructuresSerialization.h"

#if WITH_EDITOR
#include "AssetToolsModule.h"
#include "IAssetTools.h"
#include "ObjectTools.h"
#include "ContentBrowserModule.h"
#include "IContentBrowserSingleton.h"
#endif //WITH_EDITOR

USubstanceInstanceFactory::USubstanceInstanceFactory(const class FObjectInitializer& PCIP)
	: Super(PCIP)
{
	GenerationMode = GetDefault<USubstanceSettings>()->DefaultGenerationMode;
	mPackageUserData.ParentFactory = this;
	SubstancePackage = nullptr;
}

void USubstanceInstanceFactory::BeginDestroy()
{
	//Route BeginDestroy
	Super::BeginDestroy();

	//Make sure all referencing graphs are first cleaned up!
	auto InstItr = GraphInstances.CreateIterator();
	for (; InstItr; ++InstItr)
	{
		if ((*InstItr)->IsValidLowLevel())
		{
			(*InstItr)->CleanupGraphInstance();
		}
	}

	//Clean up package memory
	if (SubstancePackage)
	{
		AIR_DELETE(SubstancePackage);
		SubstancePackage = NULL;
	}
}

ESubstanceGenerationMode USubstanceInstanceFactory::GetGenerationMode() const
{
	if (GenerationMode == ESubstanceGenerationMode::SGM_PlatformDefault)
	{
		ESubstanceGenerationMode systemMode = GetDefault<USubstanceSettings>()->DefaultGenerationMode;

		if (systemMode == ESubstanceGenerationMode::SGM_PlatformDefault)
			return SGM_OnLoadAsyncAndCache;

		return systemMode;
	}

	return GenerationMode;
}

bool USubstanceInstanceFactory::ShouldCacheOutput() const
{
#if PLATFORM_PS4
	return false;
#else
	switch (GetGenerationMode())
	{
	case ESubstanceGenerationMode::SGM_OnLoadAsyncAndCache:
	case ESubstanceGenerationMode::SGM_OnLoadSyncAndCache:
		return true;
	default:
		return false;
	}
#endif
}

void USubstanceInstanceFactory::RegisterGraphInstance(USubstanceGraphInstance* Graph)
{
	GraphInstances.AddUnique(Graph);
}

void USubstanceInstanceFactory::UnregisterGraphInstance(USubstanceGraphInstance* Graph)
{
	GraphInstances.Remove(Graph);
}

const TArray<USubstanceGraphInstance*> USubstanceInstanceFactory::GetGraphInstances()
{
	return GraphInstances;
}

void USubstanceInstanceFactory::ClearReferencingObjects()
{
	auto GraphItr = GraphInstances.CreateIterator();
	for (; GraphItr; ++GraphItr)
	{
		if ((*GraphItr)->IsValidLowLevel())
			Substance::Helpers::RegisterForDeletion(*GraphItr);
	}
	Substance::Helpers::PerformDelayedDeletion();

	GraphInstances.Empty();
}

void USubstanceInstanceFactory::Serialize(FArchive& Ar)
{
	//Parent serialize
	Super::Serialize(Ar);

	//If we aren't loading, always use most up to date serialization method
	if (!Ar.IsLoading())
	{
		Ar.UsingCustomVersion(FSubstanceCoreCustomVersion::GUID);
		SerializeCurrent(Ar);
		return;
	}

	//Check the version to see if we need to serialize legacy
	bool bSerializeLegacy = false;
	if (Ar.CustomVer(FSubstanceCoreCustomVersion::GUID) < FSubstanceCoreCustomVersion::FrameworkRefactor)
	{
		//Handle Legacy Loading here
		bSerializeLegacy = true;
	}

	//Update the Ar with the most recent version
	Ar.UsingCustomVersion(FSubstanceCoreCustomVersion::GUID);

	//Call serialize based on version
	(bSerializeLegacy == true) ? this->SerializeLegacy(Ar) : SerializeCurrent(Ar);
}

void USubstanceInstanceFactory::SerializeCurrent(FArchive& Ar)
{
	//Generation and cooking
	bool bCooked = Ar.IsCooking();
	Ar << GenerationMode;
	Ar << bCooked;

	//Flag to determine if we need to save and load substance archive data
	bool bLoadSubstanceArData = !(GetGenerationMode() == ESubstanceGenerationMode::SGM_Baked && bCooked);

	//Serialize substance archive data
	if (bLoadSubstanceArData && (Ar.IsLoading() || (Ar.IsSaving() && SubstancePackage)))
	{
		Ar << SubstancePackage;
	}

	//Editor only data
	if (!bCooked)
	{
		Ar << AbsoluteSourceFilePath;
		Ar << SourceFileTimestamp;
	}

	//Associate this asset with the current plugin version
	Ar.UsingCustomVersion(FSubstanceCoreCustomVersion::GUID);
	Ar.SetCustomVersion(FSubstanceCoreCustomVersion::GUID, FSubstanceCoreCustomVersion::LatestVersion, FName("LegacyUpdated"));
}

void USubstanceInstanceFactory::SerializeLegacy(FArchive& Ar)
{
	bool bCooked = Ar.IsCooking();

	Ar << GenerationMode;
	Ar << bCooked;

	//Package details
	int32 archSize = 0;

	//Serialize and create package
	char* arArchive = LegacySerailizePackage(Ar, this, archSize);
	SubstancePackage = Substance::Helpers::InstantiatePackage(arArchive, archSize);
	delete arArchive;

	//Clear the archive to be updated with the new serialization on save.
	Ar.FlushCache();

	//Forcing package dirty
	UPackage* Package = GetOutermost();
	Package->SetDirtyFlag(true);
}

void USubstanceInstanceFactory::PostLoad()
{
	mPackageUserData.ParentFactory = this;

	if(SubstancePackage)
		SubstancePackage->mUserData = reinterpret_cast<size_t>(&mPackageUserData);

	Super::PostLoad();
}

void USubstanceInstanceFactory::PostDuplicate(bool bDuplicateForPIE)
{
	Super::PostDuplicate(bDuplicateForPIE);

	if (!bDuplicateForPIE)
	{
		check(SubstancePackage);

#if WITH_EDITOR
		TArray<UObject*> AssetList;
#endif

		//The serialize now brings over the old package desc (ONLY ON DUPLICATE - This is because this is storing a memory address and only relevant while running and very temporary) 
		PackageDescData* DuplicationSourceFactoryUserData = reinterpret_cast<PackageDescData*>(SubstancePackage->mUserData);
		USubstanceInstanceFactory* DuplicationSourceFactory = DuplicationSourceFactoryUserData->ParentFactory;

		//Re-create the instances from the original
		const TArray<USubstanceGraphInstance*> InstancesToRecreate = DuplicationSourceFactory->GetGraphInstances();
		TArray<USubstanceGraphInstance*>::TConstIterator ItPrevInst(InstancesToRecreate);
		for (; ItPrevInst; ++ItPrevInst)
		{
			auto OurGraphDescItr = SubstancePackage->getGraphs().begin();
			for (; OurGraphDescItr != SubstancePackage->getGraphs().end(); ++OurGraphDescItr)
			{
				SubstanceAir::GraphInstance* PreviousInstance = (*ItPrevInst)->Instance;
				if (PreviousInstance->mDesc.mPackageUrl == OurGraphDescItr->mPackageUrl)
				{
					USubstanceGraphInstance* SourceInstance = *ItPrevInst;

					FString BasePath;
					FString ParentName = this->GetOuter()->GetPathName();

					ParentName.Split(TEXT("/"), &(BasePath), NULL, ESearchCase::IgnoreCase, ESearchDir::FromEnd);

					FString AssetNameStr;
					FString PackageNameStr;

#if WITH_EDITOR
					FString Name = ObjectTools::SanitizeObjectName(SourceInstance->Instance->mDesc.mLabel.c_str());
					static FName AssetToolsModuleName = FName("AssetTools");
					FAssetToolsModule& AssetToolsModule = FModuleManager::GetModuleChecked<FAssetToolsModule>(AssetToolsModuleName);
					AssetToolsModule.Get().CreateUniqueAssetName(BasePath + TEXT("/") + Name + TEXT("_INST"), TEXT(""), PackageNameStr, AssetNameStr);
#else
					PackageNameStr = BasePath;
#endif
					UObject* InstanceOuter = CreatePackage(NULL, *PackageNameStr);

					USubstanceGraphInstance* NewInstance = Substance::Helpers::InstantiateGraph(this, (*OurGraphDescItr), InstanceOuter, AssetNameStr, true /*bCreateOutputs*/);

					//Copy values from previous
					Substance::Helpers::CopyInstance((*ItPrevInst), NewInstance, false);
					Substance::Helpers::RenderAsync(NewInstance->Instance);

#if WITH_EDITOR
					//Create the a default material if the others had default material
					if (SourceInstance->CreatedMaterial)
						Substance::Helpers::CreateDefaultNamedMaterial(NewInstance);

					for (auto itout = NewInstance->Instance->getOutputs().begin(); itout != NewInstance->Instance->getOutputs().end(); ++itout)
					{
						if((*itout)->mUserData)
							AssetList.AddUnique(reinterpret_cast<OutputInstanceData*>((*itout)->mUserData)->Texture.Get());
					}
					AssetList.AddUnique(reinterpret_cast<GraphInstanceData*>(NewInstance->Instance->mUserData)->ParentGraph.Get());
#endif
					break;
				}
			}
		}

#if WITH_EDITOR

		if (GIsEditor)
		{
			AssetList.AddUnique(this);
			FContentBrowserModule& ContentBrowserModule = FModuleManager::Get().LoadModuleChecked<FContentBrowserModule>("ContentBrowser");
			ContentBrowserModule.Get().SyncBrowserToAssets(AssetList);
		}
#endif
	}
}

void USubstanceInstanceFactory::GetResourceSizeEx(FResourceSizeEx& CumulativeResourceSize)
{
	if (SubstancePackage)
	{
		//Asm Size
		CumulativeResourceSize.AddDedicatedSystemMemoryBytes(SubstancePackage->getAssemblyData()->length());

		//Xml Size
		CumulativeResourceSize.AddDedicatedSystemMemoryBytes(SubstancePackage->getXmlString().length());
	}
}