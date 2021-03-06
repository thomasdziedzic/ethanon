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


#ifndef ETH_PARALLAX_MANAGER_H_
#define ETH_PARALLAX_MANAGER_H_

#include <gs2d.h>
#include <gs2dshader.h>

using namespace gs2d;
using namespace math;

class ETHParallaxManager
{
	Vector2 m_normalizedOrigin;
	float m_intensity;

public:
	ETHParallaxManager();
	ETHParallaxManager(const Vector2& normalizedOrigin, const float intensity);
	void SetNormalizedOrigin(const Vector2& normalizedOrigin);
	Vector2 GetNormalizedOrigin() const;
	Vector2 GetInScreenOrigin(const VideoConstPtr& video) const;

	inline Vector2 ComputeOffset(const VideoPtr& video, const Vector3 &pos, const float& individualParallaxIntensity) const
	{
		const Vector2 screenSpacePos = Vector2(pos.x, pos.y) - video->GetCameraPos();
		return ((screenSpacePos - GetInScreenOrigin(video)) / video->GetScreenSizeF().x) * pos.z * m_intensity * individualParallaxIntensity;
	}

	void SetIntensity(const float intensity);
	float GetIntensity() const;

	void SetShaderParameters(const VideoConstPtr& video, const ShaderPtr& shader, const Vector3& entityPos,
							 const float& individualParallaxIntensity, const bool drawToTarget) const;
};

#endif