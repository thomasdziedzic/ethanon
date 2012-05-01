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

#include "ETHPrimitiveDrawer.h"
#include <Platform/Platform.h>

ETHTextDrawer::ETHTextDrawer(const ETHResourceProviderPtr& provider, const Vector2 &pos, 
							 const str_type::string &text, const str_type::string &font,
							 const GS_DWORD color, const unsigned long time,
							 const unsigned long startTime, const float scale)
{
	this->v2Pos = pos;
	this->text = text;
	this->font = font;
	this->color = color;
	this->time = time;
	this->startTime = startTime;
	this->provider = provider;
	this->scale = scale;
}

ETHTextDrawer::ETHTextDrawer(const ETHResourceProviderPtr& provider, const Vector2 &pos, 
							 const str_type::string &text, const str_type::string &font,
							 const GS_DWORD color, const float scale)
{
	this->v2Pos = pos;
	this->text = text;
	this->font = font;
	this->color = color;
	this->time = 0;
	this->startTime = 0;
	this->provider = provider;
	this->scale = scale;
}

bool ETHTextDrawer::Draw(const unsigned long time)
{
	GS_COLOR color = this->color;

	if (this->time > 0)
	{
		const unsigned int  elapsed = time-this->startTime;
		const float fade = 1.0f-Clamp((float)elapsed/(float)this->time, 0.0f, 1.0f);
		color.a = (GS_BYTE)(fade*255.0f);
	}
	return provider->GetVideo()->DrawBitmapText(v2Pos, text, font, color, scale);
}

ETHRectangleDrawer::ETHRectangleDrawer(const ETHResourceProviderPtr& provider, const Vector2 &pos, const Vector2 &size, const GS_COLOR color, const float depth)
{
	this->v2Pos = pos;
	this->v2Size = size;
	this->color0 = color;
	this->color1 = color;
	this->color2 = color;
	this->color3 = color;
	this->depth = depth;
	this->provider = provider;
}

ETHRectangleDrawer::ETHRectangleDrawer(const ETHResourceProviderPtr& provider,
									   const Vector2 &pos, const Vector2 &size, const GS_COLOR color0, const GS_COLOR color1,
									   const GS_COLOR color2, const GS_COLOR color3, const float depth)
{
	this->v2Pos = pos;
	this->v2Size = size;
	this->color0 = color0;
	this->color1 = color1;
	this->color2 = color2;
	this->color3 = color3;
	this->depth = depth;
	this->provider = provider;
}

bool ETHRectangleDrawer::Draw(const unsigned long time)
{
	GS2D_UNUSED_ARGUMENT(time);
	provider->GetVideo()->SetSpriteDepth(depth);
	return provider->GetVideo()->DrawRectangle(v2Pos, v2Size, color0, color1, color2, color3, 0.0f);
}

ETHLineDrawer::ETHLineDrawer(const ETHResourceProviderPtr& provider, const Vector2 &a, const Vector2 &b,
								   const GS_COLOR color0, const GS_COLOR color1, const float width, const float depth)
{
	this->a = a;
	this->b = b;
	this->colorA = color0;
	this->colorB = color1;
	this->depth = depth;
	this->width = width;
	this->provider = provider;
}

bool ETHLineDrawer::Draw(const unsigned long time)
{
	GS2D_UNUSED_ARGUMENT(time);
	provider->GetVideo()->SetSpriteDepth(depth);
	provider->GetVideo()->SetLineWidth(width);
	return provider->GetVideo()->DrawLine(a, b, colorA, colorB);
}

ETHSpriteDrawer::ETHSpriteDrawer(const ETHResourceProviderPtr& provider, ETHGraphicResourceManagerPtr graphicResources,
								 const str_type::string &currentPath, const str_type::string &name, const Vector2 &pos,
								 const Vector2 &size, const GS_COLOR color, const float depth, const float angle,
								 const unsigned int frame)
{
	this->name = name;
	this->color0 = color;
	this->color1 = color;
	this->color2 = color;
	this->color3 = color;
	this->v2Pos = pos;
	this->v2Size = size;
	this->depth = depth;
	this->angle = angle;
	this->provider = provider;
	this->currentPath = currentPath;
	this->frame = frame;

	str_type::string searchPath(Platform::GetFilePath(name.c_str()));
	sprite = graphicResources->GetPointer(provider->GetVideo(), name, currentPath, searchPath, false);
	if (sprite)
		this->v2Origin = sprite->GetOrigin();
}

bool ETHSpriteDrawer::Draw(const unsigned long time)
{
	GS2D_UNUSED_ARGUMENT(time);
	if (sprite)
	{
		const Vector2 size(v2Size == Vector2(0,0) ? sprite->GetBitmapSizeF() : v2Size);
		provider->GetVideo()->SetSpriteDepth(depth);
		sprite->SetOrigin(v2Origin);
		sprite->SetRect(frame);
		return sprite->DrawShaped(v2Pos, size, color0, color1, color2, color3, angle);
	}
	else
	{
		return false;
	}
}
