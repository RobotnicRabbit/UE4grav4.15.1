// Copyright 2016 Allegorithmic Inc. All rights reserved.
// File: SubstanceFactory.cpp

#include "SubstanceEditorPrivatePCH.h"

#include "SubstanceEditorClasses.h"
#include "SubstanceOptionWindow.h"
#include "SubstanceCoreHelpers.h"
#include "SubstanceCoreClasses.h"
#include "SubstanceInstanceFactory.h"
#include "SubstanceGraphInstance.h"
#include "SubstanceTexture2D.h"
#include "SubstanceUtility.h"

#include "IPluginManager.h"
#include "AssetToolsModule.h"
#include "IAssetTools.h"
#include "AutomatedAssetImportData.h"
#include "ObjectTools.h"
#include "ContentBrowserModule.h"
#include "IContentBrowserSingleton.h"
#include "IMainFrameModule.h"
#include "Paths.h"

#include "Materials/MaterialExpressionTextureSample.h"
#include "Materials/MaterialExpressionTextureSampleParameter2D.h"

#include "substance/framework/output.h"
#include "substance/framework/input.h"
#include "substance/framework/graph.h"
#include "substance/framework/package.h"
#include "substance/framework/preset.h"


//==============================================================================================================

struct ReimportData
{
	FString PackageURL;
	UMaterial* OwnedMaterialReferencee;
	TArray<MaterialParameterSet> ReferencingMaterials;
	SubstanceAir::shared_ptr<SubstanceAir::Preset> PreviousPresetData;

	ReimportData(FString P, UMaterial* M, SubstanceAir::shared_ptr<SubstanceAir::Preset> SAP, TArray<MaterialParameterSet>& RM)
	{
		PackageURL = P;
		OwnedMaterialReferencee = M;
		ReferencingMaterials = RM;
		PreviousPresetData = SAP;
	}
};

/** Reimport data map */
TArray<ReimportData> PreviousInstanceTransfer;

/** Stores if we are importing or reimporting */
bool USubstanceFactory::bSuppressImportOverwriteDialog = false;

namespace local
{
bool bIsPerformingReimport = false;
USubstanceInstanceFactory* FactoryBeingReimported;
}

void Substance::ApplyImportUIToImportOptions(USubstanceImportOptionsUi* ImportUI, FSubstanceImportOptions& InOutImportOptions)
{
	static FName AssetToolsModuleName = FName("AssetTools");
	FAssetToolsModule& AssetToolsModule = FModuleManager::GetModuleChecked<FAssetToolsModule>(AssetToolsModuleName);
	FString PkgName;
	FString AssetName;

	InOutImportOptions.bCreateInstance = ImportUI->bCreateInstance;
	InOutImportOptions.bCreateMaterial = ImportUI->bCreateMaterial;

	AssetToolsModule.Get().CreateUniqueAssetName(ImportUI->InstanceDestinationPath + TEXT("/"), ImportUI->InstanceName, PkgName, AssetName);

	InOutImportOptions.InstanceDestinationPath = PkgName;
	InOutImportOptions.InstanceName = AssetName;

	AssetToolsModule.Get().CreateUniqueAssetName(ImportUI->MaterialDestinationPath + TEXT("/"), ImportUI->MaterialName, PkgName, AssetName);

	InOutImportOptions.MaterialName = AssetName;
	InOutImportOptions.MaterialDestinationPath = PkgName;
}

void Substance::GetImportOptions(
    FString Name,
    FString ParentName,
    FSubstanceImportOptions& InOutImportOptions,
    bool& OutOperationCanceled)
{
	USubstanceImportOptionsUi* ImportUI = NewObject<USubstanceImportOptionsUi>();

	ImportUI->bForceCreateInstance = InOutImportOptions.bForceCreateInstance;
	ImportUI->bCreateInstance = true;
	ImportUI->bCreateMaterial = true;

	Name = ObjectTools::SanitizeObjectName(Name);

	ImportUI->bOverrideFullName = false;
	FString BasePath;
	ParentName.Split(TEXT("/"), &(BasePath), NULL, ESearchCase::IgnoreCase, ESearchDir::FromEnd);
	ImportUI->InstanceDestinationPath = BasePath;
	ImportUI->MaterialDestinationPath = BasePath;

	FString FullAssetPath;
	FAssetToolsModule& AssetToolsModule = FModuleManager::GetModuleChecked<FAssetToolsModule>("AssetTools");
	AssetToolsModule.Get().CreateUniqueAssetName(BasePath + TEXT("/") + Name + TEXT("_INST"), TEXT(""),
	        FullAssetPath, ImportUI->InstanceName);

	AssetToolsModule.Get().CreateUniqueAssetName(BasePath + TEXT("/") + Name + TEXT("_MAT"), TEXT(""),
	        FullAssetPath, ImportUI->MaterialName);


	TSharedPtr<SWindow> ParentWindow;
	//Check if the main frame is loaded.  When using the old main frame it may not be.
	if (FModuleManager::Get().IsModuleLoaded("MainFrame"))
	{
		IMainFrameModule& MainFrame = FModuleManager::LoadModuleChecked<IMainFrameModule>("MainFrame");
		ParentWindow = MainFrame.GetParentWindow();
	}

	TSharedRef<SWindow> Window = SNew(SWindow)
	                             .Title(NSLOCTEXT("UnrealEd", "SubstanceImportOpionsTitle", "Substance Import Options"))
	                             .SizingRule(ESizingRule::Autosized);

	TSharedPtr<SSubstanceOptionWindow> SubstanceOptionWindow;
	Window->SetContent
	(
	    SAssignNew(SubstanceOptionWindow, SSubstanceOptionWindow)
	    .ImportUI(ImportUI)
	    .WidgetWindow(Window)
	);

	//TODO:: We can make this slow as showing progress bar later
	FSlateApplication::Get().AddModalWindow(Window, ParentWindow, false);

	if (SubstanceOptionWindow->ShouldImport())
	{
		//Open dialog. See if it's canceled
		ApplyImportUIToImportOptions(ImportUI, InOutImportOptions);
		OutOperationCanceled = false;
	}
	else
	{
		OutOperationCanceled = true;
	}
}

USubstanceFactory::USubstanceFactory(const class FObjectInitializer& PCIP)
	: Super(PCIP)
{
	bEditAfterNew = true;
	bEditorImport = true;

	SupportedClass = USubstanceInstanceFactory::StaticClass();

	Formats.Empty(1);
	Formats.Add(TEXT("sbsar;Substance Texture"));
}

UObject* USubstanceFactory::FactoryCreateBinary(
    UClass* Class,
    UObject* InParent,
    FName Name,
    EObjectFlags Flags,
    UObject* Context,
    const TCHAR* Type,
    const uint8*& Buffer,
    const uint8* BufferEnd,
    FFeedbackContext* Warn)
{
	USubstanceInstanceFactory* Factory = NULL;
	FName InstanceFactoryName = Name;
	UObject* Parent = InParent;

	//Overwrite defaults to specify which object to overwrite during a reimport.
	if (local::bIsPerformingReimport)
	{
		InstanceFactoryName = local::FactoryBeingReimported->GetFName();
		Parent = local::FactoryBeingReimported->GetOuter();

		//Reset as this should not be used after this point until the next reimport.
		local::FactoryBeingReimported = nullptr;
	}

	//Create a USubstanceFactory UAsset - This will appear in content browser
	Factory = CastChecked<USubstanceInstanceFactory>(CreateOrOverwriteAsset(USubstanceInstanceFactory::StaticClass(), Parent,
	          InstanceFactoryName, RF_Standalone | RF_Public));

	const uint32 BufferLength = BufferEnd - Buffer;

	//Create the framework package
	Factory->SubstancePackage = Substance::Helpers::InstantiatePackage((void*)Buffer, BufferLength);
	Factory->SubstancePackage->mUserData = (size_t)&Factory->mPackageUserData;

	//If the operation failed
	if (false == Factory->SubstancePackage->isValid())
	{
		//Mark the package for garbage collect
		Factory->ClearFlags(RF_Standalone);
		return NULL;
	}

	//Create relative path
	FString RelativePath = GetCurrentFilename();
	FString RootGameDir = FPaths::GameDir();

	//Make relative paths to the source file
	FPaths::MakeStandardFilename(RootGameDir);
	FPaths::MakePathRelativeTo(RelativePath, *RootGameDir);

	//Set the source file data
	Factory->AbsoluteSourceFilePath = GetCurrentFilename();
	Factory->RelativeSourceFilePath = RelativePath;
	Factory->SourceFileTimestamp = IFileManager::Get().GetTimeStamp(*Factory->AbsoluteSourceFilePath).ToString();

	Substance::FSubstanceImportOptions ImportOptions;
	TArray<FString> Names;

	bool bAllCancel = true;

	//Stores references to all of the assets we are about to create to sync with content browser
	TArray<UObject*> AssetList;

	//Reimport Assets compared to plain import
	if (local::bIsPerformingReimport)
	{
		RecreateGraphsPostReimport(Factory, InParent);
		Factory->MarkPackageDirty();
		return Factory;
	}


	//Create all of the graphs
	for (auto GraphIt = Factory->SubstancePackage->getGraphs().begin(); GraphIt != Factory->SubstancePackage->getGraphs().end(); ++GraphIt)
	{
		bool bOperationCanceled = false;

		Substance::GetImportOptions(GraphIt->mLabel.c_str(), InParent->GetName(), ImportOptions, bOperationCanceled);

		if (bOperationCanceled)
		{
			bAllCancel = bOperationCanceled && bAllCancel;
			continue;
		}
		else
		{
			bAllCancel = false;
		}

		if (ImportOptions.bCreateInstance)
		{
			FAssetToolsModule& AssetToolsModule = FModuleManager::GetModuleChecked<FAssetToolsModule>("AssetTools");
			FString InstPath = ImportOptions.InstanceDestinationPath;
			FString InstName = ImportOptions.InstanceName;

			//Create USubstanceGraphInstance
			UObject* GraphBasePackage = CreatePackage(NULL, *InstPath);
			USubstanceGraphInstance* NewInstance = Substance::Helpers::InstantiateGraph(Factory, (*GraphIt), GraphBasePackage,
			                                       InstName, true);
			AssetList.AddUnique(NewInstance);

			Substance::Helpers::RenderSync(NewInstance->Instance);

			if (ImportOptions.bCreateMaterial)
			{
				FString MatPath = ImportOptions.MaterialDestinationPath;
				FString MatName = ImportOptions.MaterialName;

				UObject* MaterialBasePackage = CreatePackage(NULL, *MatPath);
				TWeakObjectPtr<UMaterial> Mat = Substance::Helpers::CreateMaterial(NewInstance, MatName, MaterialBasePackage);
				if (Mat.IsValid())
					AssetList.AddUnique(Mat.Get());
			}
		}

	}

	//If we have created SubstanceGraphInstances and we cancel - They are never cleaned and
	//the package is which will cause a crash
	if (bAllCancel)
	{
		Factory->ClearReferencingObjects();
		Factory->ClearFlags(RF_Standalone);
		CollectGarbage(GARBAGE_COLLECTION_KEEPFLAGS);
		return NULL;
	}

	//Update the content browser with the new assets
	AssetList.AddUnique(Factory);
	FContentBrowserModule& ContentBrowserModule = FModuleManager::Get().LoadModuleChecked<FContentBrowserModule>("ContentBrowser");
	ContentBrowserModule.Get().SyncBrowserToAssets(AssetList, true);

	Factory->MarkPackageDirty();

	return Factory;
}


void USubstanceFactory::RecreateGraphsPostReimport(USubstanceInstanceFactory* ParentFactory, UObject* InParent)
{
	//Stores references to all of the assets we are about to create to sync with content browser
	TArray<UObject*> AssetList;

	//Stores the reference to the graph we will be creating
	const SubstanceAir::GraphDesc* GraphDescToCreate = NULL;

	//Loop through all of the graphs we need to recreate
	auto GrItr = PreviousInstanceTransfer.CreateIterator();
	for (; GrItr; ++GrItr)
	{
		//Find the graph to use to create this instance
		auto GDescItr = ParentFactory->SubstancePackage->getGraphs().cbegin();
		for (; GDescItr != ParentFactory->SubstancePackage->getGraphs().cend(); ++GDescItr)
		{
			if (FString((*GDescItr).mPackageUrl.c_str()) == (*GrItr).PackageURL)
			{
				GraphDescToCreate = &(*GDescItr);
				break;
			}
		}

		//If we can't find the graph instance to create, warn the user and continue gracefully
		if (!GraphDescToCreate)
		{
			UE_LOG(LogSubstanceEditor, Warning, TEXT("Failed to create a graph instance after reimport"))
			continue;
		}

		//First create the graph instance
		FAssetToolsModule& AssetToolsModule = FModuleManager::GetModuleChecked<FAssetToolsModule>("AssetTools");
		FString InstPath;
		FString InstName;

		FString IName = ObjectTools::SanitizeObjectName(GraphDescToCreate->mLabel.c_str());
		FString BasePath;
		InParent->GetName().Split(TEXT("/"), &(BasePath), NULL, ESearchCase::IgnoreCase, ESearchDir::FromEnd);
		AssetToolsModule.Get().CreateUniqueAssetName(BasePath + TEXT("/") + IName + TEXT("_INST"), TEXT(""), InstPath, InstName);


		//Create USubstanceGraphInstance
		UObject* GraphBasePackage = CreatePackage(NULL, *InstPath);
		USubstanceGraphInstance* NewInstance = Substance::Helpers::InstantiateGraph(ParentFactory, *GraphDescToCreate, GraphBasePackage, InstName, true);
		AssetList.AddUnique(NewInstance);

		//Apply presets
		Substance::Helpers::ApplyPresetData(NewInstance->Instance, *(*GrItr).PreviousPresetData.get());

		//Make sure the textures are recomputed post create
		Substance::Helpers::RenderSync(NewInstance->Instance);

		//Check if material is valid if so, set it up with the new outputs
		Substance::Helpers::ResetMaterialTexturesFromGraph(NewInstance, (*GrItr).OwnedMaterialReferencee, (*GrItr).ReferencingMaterials);

	}

	FContentBrowserModule& ContentBrowserModule = FModuleManager::Get().LoadModuleChecked<FContentBrowserModule>("ContentBrowser");
	ContentBrowserModule.Get().SyncBrowserToAssets(AssetList, true);

	//Clear the recreation data as it is no longer needed
	PreviousInstanceTransfer.Empty();
}

//==============================================================================================================
//Begin UReimportSubstanceFactory Implementation
UReimportSubstanceFactory::UReimportSubstanceFactory(class FObjectInitializer const& PCIP) : Super(PCIP)
{

}

bool UReimportSubstanceFactory::CanReimport(UObject* Obj, TArray<FString>& OutFilenames)
{
	const USubstanceGraphInstance* GraphInstance = Cast<USubstanceGraphInstance>(Obj);
	const USubstanceInstanceFactory* InstanceFactory = Cast<USubstanceInstanceFactory>(Obj);

	//Make sure one is valid
	if (NULL == GraphInstance && NULL == InstanceFactory)
	{
		return false;
	}

	//Make sure we have a valid parent factory
	if (GraphInstance && NULL == GraphInstance->ParentFactory)
	{
		UE_LOG(LogSubstanceEditor, Warning, TEXT("Cannot reimport: The Substance Graph Instance does not have any parent package."));
		return false;
	}

	if (GraphInstance)
		InstanceFactory = GraphInstance->ParentFactory;

	//Check absolute file path
	IFileManager& FileManager = IFileManager::Get();
	if (FileManager.FileExists(*InstanceFactory->AbsoluteSourceFilePath))
	{
		OutFilenames.Add(InstanceFactory->AbsoluteSourceFilePath);
		return true;
	}

	//Get absolute path from relative
	FString Blank;
	FString DiskFilename = FileManager.GetFilenameOnDisk(*Blank);

	//Combine relative source path with absolute path to directory to get an absolute source file path
	FString BasePath;
	DiskFilename.Split(TEXT("/Binaries"), &(BasePath), NULL, ESearchCase::IgnoreCase, ESearchDir::FromEnd);
	FString AbsoluteFromRelative = FPaths::Combine(BasePath, InstanceFactory->RelativeSourceFilePath);

	//Check relative path
	if (FileManager.FileExists(*AbsoluteFromRelative))
	{
		OutFilenames.Add(AbsoluteFromRelative);
		return true;
	}

	//If neither path is found, we can not reimport
	return false;
}

void UReimportSubstanceFactory::SetReimportPaths(UObject* Obj, const TArray<FString>& NewReimportPaths)
{
	USubstanceInstanceFactory* Factory = Cast<USubstanceInstanceFactory>(Obj);
	if (Factory && ensure(NewReimportPaths.Num() == 1))
	{
		Factory->AbsoluteSourceFilePath = NewReimportPaths[0];
	}
}

EReimportResult::Type UReimportSubstanceFactory::Reimport(UObject* Obj)
{
	local::bIsPerformingReimport = true;
	local::FactoryBeingReimported = nullptr;

	//Find the parent factory - root of the reimport
	USubstanceInstanceFactory* CurrentInstanceFactory = Cast<USubstanceInstanceFactory>(Obj);

	if (!CurrentInstanceFactory)
	{
		UE_LOG(LogSubstanceEditor, Error, TEXT("Unable to reimport: The instance factory is invalid"))
		local::bIsPerformingReimport = false;
		return EReimportResult::Failed;
	}

	FAssetToolsModule& AssetToolsModule = FModuleManager::Get().LoadModuleChecked<FAssetToolsModule>("AssetTools");
	FString PathName = CurrentInstanceFactory->GetPathName();

	//Check if we can reimport which also will grab to path to reimport from
	TArray<FString> SourcePaths;
	if (!CanReimport(Obj, SourcePaths))
	{
		UE_LOG(LogSubstanceEditor, Error, TEXT("Unable to reimport: Cannot find asset at relative or absolute path saved when asset was first created."));
		local::bIsPerformingReimport = false;
		return EReimportResult::Failed;
	}

	//Fix up path
	FString RootPath;
	FString CurrentName;
	PathName.Split(TEXT("/"), &RootPath, &CurrentName, ESearchCase::IgnoreCase, ESearchDir::FromEnd);

	//Before we clears the objects, store all of the registered graph instances and their materials (if valid)
	//to be able to create all the needed graph instances when reimported and to properly assign the new outputs to the
	//old material.
	SaveRecreationData(CurrentInstanceFactory);

	//Clear the objects referencing the factory! It is about to be replaced!
	CurrentInstanceFactory->ClearReferencingObjects();

	//Import settings
	UAutomatedAssetImportData* ImportData = NewObject<UAutomatedAssetImportData>();

	//Import again - We already know paths are valid from can reimport
	ImportData->bReplaceExisting = true;
	ImportData->Filenames = SourcePaths;
	ImportData->DestinationPath = RootPath;
	ImportData->bSkipReadOnly = true;

	local::FactoryBeingReimported = CurrentInstanceFactory;

	AssetToolsModule.Get().ImportAssetsAutomated(*ImportData);

	//Return success
	UE_LOG(LogSubstanceEditor, Log, TEXT("Re-imported successfully"));
	local::bIsPerformingReimport = false;
	return EReimportResult::Succeeded;
}

/** Saves all of the informations we need to recreate this factory */
void UReimportSubstanceFactory::SaveRecreationData(USubstanceInstanceFactory* FactoryToReimport)
{
	//This should already be empty but making sure we don't try to recreate garbage
	PreviousInstanceTransfer.Empty();

	//Save which graph instance and the material for all graph instances the factory has created
	const TArray<USubstanceGraphInstance*> CurrentInstances = FactoryToReimport->GetGraphInstances();
	auto GraphItr = CurrentInstances.CreateConstIterator();
	for (; GraphItr; ++GraphItr)
	{
		//List of all of the materials currently referencing
		TArray<MaterialParameterSet> ReferencingMaterials;

		//Fill an array with all materials referencing this graph instance and pass it to the reimport data
		for (TObjectIterator<UMaterial> MatItr; MatItr; ++MatItr)
		{
			//The data set for the current material to be filled out if the material references outputs from this graph
			MaterialParameterSet CurrentMaterialSet;

			//Create the material data
			for (int32 ExpressionIndex = (*MatItr)->Expressions.Num() - 1; ExpressionIndex >= 0; ExpressionIndex--)
			{
				UMaterialExpressionTextureSample* Expression = Cast<UMaterialExpressionTextureSample>((*MatItr)->Expressions[ExpressionIndex]);

				if (Expression)
				{
					USubstanceTexture2D* SubstanceTexture = Cast<USubstanceTexture2D>(Expression->Texture);
					if (SubstanceTexture && SubstanceTexture->OutputCopy && SubstanceTexture->ParentInstance && SubstanceTexture->ParentInstance->Instance &&
					        SubstanceTexture->ParentInstance->Instance->mInstanceUid == (*GraphItr)->Instance->mInstanceUid)
					{
						CurrentMaterialSet.Material = *MatItr;
						CurrentMaterialSet.ParameterNames.Add(SubstanceTexture->OutputCopy->mDesc.mIdentifier.c_str(), Expression);
					}
				}
			}

			//If the material has used params, add it to the update list
			if (CurrentMaterialSet.Material)
			{
				ReferencingMaterials.AddUnique(CurrentMaterialSet);
			}
		}

		Substance::Helpers::Tick();

		//Grab and store the presets
		SubstanceAir::shared_ptr<SubstanceAir::Preset> StorePreset = Substance::Helpers::EmptyPresetAsShared();
		SubstanceAir::GraphInstance* CurrentInstance = (*GraphItr)->Instance;
		Substance::Helpers::CopyPresetData(CurrentInstance, *StorePreset.get());

		//Set all of the reference data needed for this graph
		ReimportData CurrentReimportData = ReimportData((*GraphItr)->PackageURL, (*GraphItr)->CreatedMaterial, StorePreset, ReferencingMaterials);
		PreviousInstanceTransfer.Add(CurrentReimportData);
	}
}
