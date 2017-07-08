// Copyright 1998-2017 Epic Games, Inc. All Rights Reserved.

/*==============================================================================
	Texture2DPreview.h: Implementation for previewing 2D Textures and normal maps.
==============================================================================*/

#include "Texture2DPreview.h"
#include "Shader.h"
#include "GlobalShader.h"
#include "SimpleElementShaders.h"
#include "ShaderParameterUtils.h"

/*------------------------------------------------------------------------------
	Batched element shaders for previewing 2d textures.
------------------------------------------------------------------------------*/
/**
 * Simple pixel shader for previewing 2d textures at a specified mip level
 */
class FSimpleElementTexture2DPreviewPS : public FGlobalShader
{
	DECLARE_SHADER_TYPE(FSimpleElementTexture2DPreviewPS,Global);
public:

	FSimpleElementTexture2DPreviewPS(const ShaderMetaType::CompiledShaderInitializerType& Initializer) :
		FGlobalShader(Initializer)
	{
		InTexture.Bind(Initializer.ParameterMap,TEXT("InTexture"), SPF_Mandatory);
		InTextureSampler.Bind(Initializer.ParameterMap,TEXT("InTextureSampler"));	
		TextureComponentReplicate.Bind(Initializer.ParameterMap,TEXT("TextureComponentReplicate"));
		TextureComponentReplicateAlpha.Bind(Initializer.ParameterMap,TEXT("TextureComponentReplicateAlpha"));
		ColorWeights.Bind(Initializer.ParameterMap,TEXT("ColorWeights"));
		PackedParameters.Bind(Initializer.ParameterMap,TEXT("PackedParams"));
	}
	FSimpleElementTexture2DPreviewPS() {}

	/** Should the shader be cached? Always. */
	static bool ShouldCache(EShaderPlatform Platform)
	{
		return IsFeatureLevelSupported(Platform, ERHIFeatureLevel::SM4) && !IsConsolePlatform(Platform);
	}

	void SetParameters(FRHICommandList& RHICmdList, const FTexture* TextureValue, const FMatrix& ColorWeightsValue, float GammaValue, float MipLevel, bool bIsNormalMap)
	{
		SetTextureParameter(RHICmdList, GetPixelShader(),InTexture,InTextureSampler,TextureValue);
		SetShaderValue(RHICmdList, GetPixelShader(),ColorWeights,ColorWeightsValue);
		FVector4 PackedParametersValue(GammaValue, MipLevel, bIsNormalMap ? 1.0 : -1.0f, 0.0f);
		SetShaderValue(RHICmdList, GetPixelShader(), PackedParameters, PackedParametersValue);

		SetShaderValue(RHICmdList, GetPixelShader(),TextureComponentReplicate,TextureValue->bGreyScaleFormat ? FLinearColor(1,0,0,0) : FLinearColor(0,0,0,0));
		SetShaderValue(RHICmdList, GetPixelShader(),TextureComponentReplicateAlpha,TextureValue->bGreyScaleFormat ? FLinearColor(1,0,0,0) : FLinearColor(0,0,0,1));
	}

	virtual bool Serialize(FArchive& Ar) override
	{
		bool bShaderHasOutdatedParameters = FGlobalShader::Serialize(Ar);
		Ar << InTexture;
		Ar << InTextureSampler;
		Ar << TextureComponentReplicate;
		Ar << TextureComponentReplicateAlpha;
		Ar << ColorWeights;
		Ar << PackedParameters;
		return bShaderHasOutdatedParameters;
	}

private:
	FShaderResourceParameter InTexture;
	FShaderResourceParameter InTextureSampler;
	FShaderParameter TextureComponentReplicate;
	FShaderParameter TextureComponentReplicateAlpha;
	FShaderParameter ColorWeights; 
	FShaderParameter PackedParameters;
};

IMPLEMENT_SHADER_TYPE(,FSimpleElementTexture2DPreviewPS,TEXT("SimpleElementTexture2DPreviewPixelShader"),TEXT("Main"),SF_Pixel);

/** Binds vertex and pixel shaders for this element */
void FBatchedElementTexture2DPreviewParameters::BindShaders(
	FRHICommandList& RHICmdList,
	ERHIFeatureLevel::Type InFeatureLevel,
	const FMatrix& InTransform,
	const float InGamma,
	const FMatrix& ColorWeights,
	const FTexture* Texture)
{
	TShaderMapRef<FSimpleElementVS> VertexShader(GetGlobalShaderMap(InFeatureLevel));
	TShaderMapRef<FSimpleElementTexture2DPreviewPS> PixelShader(GetGlobalShaderMap(InFeatureLevel));

	
	static FGlobalBoundShaderState BoundShaderState;
	SetGlobalBoundShaderState(RHICmdList, InFeatureLevel, BoundShaderState, GSimpleElementVertexDeclaration.VertexDeclarationRHI, *VertexShader, *PixelShader);

	VertexShader->SetParameters(RHICmdList, InTransform);
	PixelShader->SetParameters(RHICmdList, Texture, ColorWeights, InGamma, MipLevel, bIsNormalMap);

	if( bIsSingleChannelFormat )
	{
		RHICmdList.SetBlendState(TStaticBlendState<>::GetRHI());
	}
}
