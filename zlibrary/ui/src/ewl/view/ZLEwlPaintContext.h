/*
 * Copyright (C) 2008 Alexander Kerner <lunohod@openinkpot.org>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA
 * 02110-1301, USA.
 */

#ifndef __ZLEWLPAINTCONTEXT_H__
#define __ZLEWLPAINTCONTEXT_H__

#include <ZLPaintContext.h>

#include <vector>

#include <pango/pango.h>
#include <pango/pangoft2.h>

#include <ft2build.h>
#include FT_FREETYPE_H
#include FT_GLYPH_H
#include FT_BITMAP_H

#include <xcb/xcb.h>
#define class class_
#include <xcb/xcb_aux.h>
#undef class
#include <xcb/xcb_image.h>

class ZLEwlPaintContext : public ZLPaintContext {

public:
	ZLEwlPaintContext();
	~ZLEwlPaintContext();

	void setImage(xcb_image_t *img, int w, int h) { image = img; myWidth = w; myHeight = h; };

	int width() const;
	int height() const;

	void clear(ZLColor color);

	void fillFamiliesList(std::vector<std::string> &families) const;
	const std::string realFontFamilyName(std::string &fontFamily) const;

	void setFont(const std::string &family, int size, bool bold, bool italic);
	void setColor(ZLColor color, LineStyle style = SOLID_LINE);
	void setFillColor(ZLColor color, FillStyle style = SOLID_FILL);

	int stringWidth(const char *str, int len, bool rtl) const;
	int spaceWidth() const;
	int stringHeight() const;
	int descent() const;
	void drawString(int x, int y, const char *str, int len, bool rtl);

	void drawImage(int x, int y, const ZLImageData &image);

	void drawLine(int x0, int y0, int x1, int y1);
	void drawLine(int x0, int y0, int x1, int y1, bool fill);
	void fillRectangle(int x0, int y0, int x1, int y1);
	void drawFilledCircle(int x, int y, int r);

	xcb_image_t     *image;

private:
	int myWidth, myHeight;
	//int *image;

	PangoContext *myContext;

	PangoFontDescription *myFontDescription;

	mutable PangoAnalysis myAnalysis;
	PangoGlyphString *myString;

	std::vector<std::string> myFontFamilies;

	mutable int myStringHeight;
	mutable int mySpaceWidth;
	int myDescent;

	int fColor;

	FT_Bitmap *ft2bmp;
	FT_Bitmap *createFTBitmap(int width, int height);
	void freeFTBitmap(FT_Bitmap *bitmap);
	void modifyFTBitmap(FT_Bitmap *bitmap, int width, int height);
	void setFTBitmap(FT_Bitmap *bitmap, int width, int height);
	void clearFTBitmap(FT_Bitmap *bitmap);

	void drawGlyph(FT_Bitmap* bitmap, FT_Int x, FT_Int y);

	void invertRegion(int x0, int y0, int x1, int y1);

	class Font {
		public:
			Font() { }

			~Font() { }
	
			std::map<unsigned long, int> charWidthCacheAll;
			std::map<unsigned long, FT_BitmapGlyph> glyphCacheAll;
			
			std::map<FT_UInt, std::map<FT_UInt, int> > kerningCacheAll;
			std::map<unsigned long, FT_UInt> glyphIdxCacheAll;
	};
	

	mutable std::map<FT_Face, Font> fontCache;
	mutable std::map<unsigned long, int> *charWidthCache;
	mutable std::map<unsigned long, FT_BitmapGlyph> *glyphCache;

	mutable std::map<FT_UInt, std::map<FT_UInt, int> > *kerningCache;
	mutable std::map<unsigned long, FT_UInt> *glyphIdxCache;

	FT_Face face;
};

#endif /* __ZLEWLPAINTCONTEXT_H__ */
