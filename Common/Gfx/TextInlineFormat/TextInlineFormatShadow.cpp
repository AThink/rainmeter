/* Copyright (C) 2015 Rainmeter Project Developers
 *
 * This Source Code Form is subject to the terms of the GNU General Public
 * License; either version 2 of the License, or (at your option) any later
 * version. If a copy of the GPL was not distributed with this file, You can
 * obtain one at <https://www.gnu.org/licenses/gpl-2.0.html>. */

#include "StdAfx.h"
#include "TextInlineFormatShadow.h"
#include "../../../Library/DialogAbout.h"

namespace Gfx {

TextInlineFormat_Shadow::TextInlineFormat_Shadow(const std::wstring& pattern, const FLOAT& blur,
	const D2D1_POINT_2F& offset, const D2D1_COLOR_F& color) :
		TextInlineFormat(pattern),
		m_Offset(offset),
		m_Blur(blur),
		m_Color(color)
{
}

TextInlineFormat_Shadow::~TextInlineFormat_Shadow()
{
}

void TextInlineFormat_Shadow::ApplyInlineFormat(ID2D1DeviceContext* target, IDWriteTextLayout* layout,
	ID2D1SolidColorBrush* solidBrush, const UINT32& strLen, const D2D1_POINT_2F& drawPosition)
{
	if (!target || !layout) return;

	// In order to make a shadow effect using the built-in D2D effect, we first need to make
	// certain parts of the string transparent. We then draw only the parts of the string we
	// we want a shadow for onto a memory bitmap. From this bitmap we can create the shadow
	// effect and draw it.

	D2D1_COLOR_F color = D2D1::ColorF(0.0f, 0.0f, 0.0f, 0.0f);
	Microsoft::WRL::ComPtr<ID2D1SolidColorBrush> transparent;
	HRESULT hr = target->CreateSolidColorBrush(color, transparent.GetAddressOf());
	if (FAILED(hr)) return;

	// Only change characters outside of the range(s) transparent
	for (UINT32 i = 0; i < strLen; ++i)
	{
		bool found = false;
		for (const auto& range : GetRanges())
		{
			if (range.length <= 0) continue;

			if (i >= range.startPosition && i < (range.startPosition + range.length))
			{
				found = true;
				break;
			}
		}

		if (!found)
		{
			DWRITE_TEXT_RANGE temp = { i, 1 };
			layout->SetDrawingEffect(transparent.Get(), temp);
		}
	}

	Microsoft::WRL::ComPtr<ID2D1Bitmap> bitmap;
	Microsoft::WRL::ComPtr<ID2D1BitmapRenderTarget> bTarget;
	hr = target->CreateCompatibleRenderTarget(bTarget.GetAddressOf());
	if (FAILED(hr)) return;
	
	// Draw onto memory bitmap target
	bTarget->BeginDraw();
	bTarget->DrawTextLayout(drawPosition, layout, solidBrush);
	bTarget->EndDraw();
	hr = bTarget->GetBitmap(bitmap.GetAddressOf());
	if (FAILED(hr)) return;

	// Create shadow effect
	Microsoft::WRL::ComPtr<ID2D1Effect> shadow;
	hr = target->CreateEffect(CLSID_D2D1Shadow, shadow.GetAddressOf());
	if (FAILED(hr)) return;

	// Load shadow options to effect
	shadow->SetInput(0, bitmap.Get());
	shadow->SetValue(D2D1_SHADOW_PROP_BLUR_STANDARD_DEVIATION, m_Blur);
	shadow->SetValue(D2D1_SHADOW_PROP_COLOR, m_Color);
	shadow->SetValue(D2D1_SHADOW_PROP_OPTIMIZATION, D2D1_SHADOW_OPTIMIZATION_SPEED);

	// Draw effect
	target->DrawImage(shadow.Get(), m_Offset);
}

bool TextInlineFormat_Shadow::CompareAndUpdateProperties(const std::wstring& pattern, const FLOAT& blur,
	const D2D1_POINT_2F& offset, const D2D1_COLOR_F& color)
{
	if (_wcsicmp(GetPattern().c_str(), pattern.c_str()) != 0 || !Util::ColorFEquals(m_Color, color))
	{
		SetPattern(pattern);
		m_Offset = offset;
		m_Color = color;
		return true;
	}

	return false;
}

}  // namespace Gfx
