/*--------------------------------------------------------------------------------------
 Ethanon Engine (C) Copyright 2008-2012 Andre Santee
 http://www.asantee.net/ethanon/

	Permission is hereby granted, free of charge, to any person obtaining a copy of this
	software and associated documentation files (the "Software"), to deal in the
	Software without restriction, including without limitation the rights to use, copy,
	modify, merge, publish, distribute, sublicense, and/or sell copies of the Software,
	and to permit persons to whom the Software is furnished to do so, subject to the
	following conditions:

	The above copyright notice and this permission notice shall be included in all
	copies or substantial portions of the Software.

	THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR IMPLIED,
	INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY, FITNESS FOR A
	PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT
	HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF
	CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE
	OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
--------------------------------------------------------------------------------------*/

#include "ETHVertexLightDiffuse.h"
#include "ETHShaders.h"

const GS_SHADER_PROFILE ETHVertexLightDiffuse::m_profile = GSSP_MODEL_2;

ETHVertexLightDiffuse::ETHVertexLightDiffuse(VideoPtr video, const str_type::string& shaderPath)
{
	m_video = video;
	m_hVertexLightVS = m_video->LoadShaderFromFile(ETHGlobal::GetDataResourceFullPath(shaderPath, ETHShaders::VL_VS_Hor_Diff()).c_str(), GSSF_VERTEX, GSSP_MODEL_2, "sprite_pvl");
	m_vVertexLightVS = m_video->LoadShaderFromFile(ETHGlobal::GetDataResourceFullPath(shaderPath, ETHShaders::VL_VS_Ver_Diff()).c_str(), GSSF_VERTEX, GSSP_MODEL_2, "sprite_pvl");
}

bool ETHVertexLightDiffuse::BeginLightPass(ETHSpriteEntity *pRender, Vector3 &v3LightPos, const Vector2 &v2Size,
	const ETHLight* light, const float maxHeight, const float minHeight, const float lightIntensity,
	const bool drawToTarget)
{
	GS2D_UNUSED_ARGUMENT(drawToTarget);
	const Vector2 &v2Origin = pRender->ComputeOrigin(v2Size);
	const Vector3 &v3EntityPos = pRender->GetPosition();

	m_video->SetPixelShader(ShaderPtr());

	ShaderPtr pLightShader;

	if (pRender->GetType() == ETH_VERTICAL)
	{
		m_vVertexLightVS->SetConstant(GS_L("spaceLength"), (maxHeight-minHeight));
		m_vVertexLightVS->SetConstant(GS_L("topLeft3DPos"), v3EntityPos-(Vector3(v2Origin.x, 0, -v2Origin.y)));
		pLightShader = m_vVertexLightVS;
	}
	else
	{
		m_hVertexLightVS->SetConstant(GS_L("topLeft3DPos"), v3EntityPos-Vector3(v2Origin, 0));
		pLightShader = m_hVertexLightVS;
	}
	m_video->SetVertexShader(pLightShader);

	m_lastAM = m_video->GetAlphaMode();
	m_video->SetAlphaMode(GSAM_ADD);

	// Set a depth value depending on the entity type
	pRender->SetDepth(maxHeight, minHeight);
 
	pLightShader->SetConstant(GS_L("pivotAdjust"), pRender->GetProperties()->pivotAdjust);
	pLightShader->SetConstant(GS_L("lightPos"), v3LightPos);
	pLightShader->SetConstant(GS_L("lightRange"), light->range);
	pLightShader->SetConstant(GS_L("lightColor"), light->color);
	pLightShader->SetConstant(GS_L("lightIntensity"), lightIntensity);
	return true;
}

bool ETHVertexLightDiffuse::EndLightPass()
{
	m_video->SetPixelShader(ShaderPtr());
	m_video->SetVertexShader(ShaderPtr());
	m_video->SetAlphaMode(m_lastAM);
	return true;
}

bool ETHVertexLightDiffuse::IsSupportedByHardware() const
{
	return (m_video->GetHighestVertexProfile() >= m_profile);
}

bool ETHVertexLightDiffuse::IsUsingPixelShader() const
{
	return false;
}

SpritePtr ETHVertexLightDiffuse::GetDefaultNormalMap()
{
	return SpritePtr();
}
