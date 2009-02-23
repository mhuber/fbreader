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

#include <algorithm>

#include <ZLUnicodeUtil.h>
#include <ZLImage.h>
#include "../image/ZLEwlImageManager.h"

#include "ZLEwlPaintContext.h"

#include <ft2build.h>
#include FT_FREETYPE_H
#include FT_GLYPH_H
#include FT_BITMAP_H

extern xcb_connection_t     *connection;
extern xcb_window_t          window;
extern xcb_screen_t         *screen;
extern xcb_image_t *im;
extern unsigned int *pal;


ZLEwlPaintContext::ZLEwlPaintContext() {
	image = NULL;

	myWidth = 600;
	myHeight = 800;

	myContext = 0;

	ft2bmp = NULL;

	myFontDescription = 0;
	myAnalysis.font = 0;
	myAnalysis.lang_engine = 0;
	myAnalysis.level = 0;
	myAnalysis.language = 0;
	myAnalysis.extra_attrs = 0;
	myString = pango_glyph_string_new();

	myStringHeight = -1;
	mySpaceWidth = -1;
	myDescent = 0;

	if (myContext == 0) {
		PangoFontMap *font_map;
		font_map = pango_ft2_font_map_new();
		pango_ft2_font_map_set_resolution (PANGO_FT2_FONT_MAP (font_map), 170, 170);
		myContext = pango_ft2_font_map_create_context (PANGO_FT2_FONT_MAP(font_map));

		if (myFontDescription != 0) {
			myAnalysis.font = pango_context_load_font(myContext, myFontDescription);
			myAnalysis.shape_engine = pango_font_find_shaper(myAnalysis.font, 0, 0);
			PangoFontMetrics *metrics = pango_font_get_metrics(myAnalysis.font, myAnalysis.language);
			myDescent = pango_font_metrics_get_descent(metrics) / PANGO_SCALE;
		}
	}
}

ZLEwlPaintContext::~ZLEwlPaintContext() {
	pango_glyph_string_free(myString);
	
	if (myFontDescription != 0) {
		pango_font_description_free(myFontDescription);
	}

	if (myContext != 0) {
		g_object_unref(myContext);
	}

	if(ft2bmp)
		clearFTBitmap(ft2bmp);
}


void ZLEwlPaintContext::fillFamiliesList(std::vector<std::string> &families) const {
	if (myContext != 0) {
		PangoFontFamily **pangoFamilies;
		int nFamilies;
		pango_context_list_families(myContext, &pangoFamilies, &nFamilies);
		for (int i = 0; i < nFamilies; ++i) {
			const char *name = pango_font_family_get_name(pangoFamilies[i]);
			if(strncmp(name, "Monospace", 9)
				&& strncmp(name, "Serif", 5)
				&& strncmp(name, "Sans", 4))
				families.push_back(name);
		}
		std::sort(families.begin(), families.end());
		g_free(pangoFamilies);
	}
}

const std::string ZLEwlPaintContext::realFontFamilyName(std::string &fontFamily) const {
	if (myContext == 0) {
		return fontFamily;
	}
	PangoFontDescription *description = pango_font_description_new();
	pango_font_description_set_family(description, fontFamily.c_str());
	pango_font_description_set_size(description, 12);
	PangoFont *font = pango_context_load_font(myContext, description);
	pango_font_description_free(description);
	description = pango_font_describe(font);
	std::string realFamily = pango_font_description_get_family(description);
	pango_font_description_free(description);
	return realFamily;
}

void ZLEwlPaintContext::setFont(const std::string &family, int size, bool bold, bool italic) {
	bool fontChanged = false;

	if (myFontDescription == 0) {
		myFontDescription = pango_font_description_new();
		fontChanged = true;
	}

	const char *oldFamily = pango_font_description_get_family(myFontDescription);
	if ((oldFamily == 0) || (family != oldFamily)) {
		pango_font_description_set_family(myFontDescription, family.c_str());
		fontChanged = true;
	}

	int newSize = size * PANGO_SCALE;
	if (pango_font_description_get_size(myFontDescription) != newSize) {
		pango_font_description_set_size(myFontDescription, newSize);
		fontChanged = true;
	}

	PangoWeight newWeight = bold ? PANGO_WEIGHT_BOLD : PANGO_WEIGHT_NORMAL;
	if (pango_font_description_get_weight(myFontDescription) != newWeight) {
		pango_font_description_set_weight(myFontDescription, newWeight);
		fontChanged = true;
	}

	PangoStyle newStyle = italic ? PANGO_STYLE_ITALIC : PANGO_STYLE_NORMAL;
	if (pango_font_description_get_style(myFontDescription) != newStyle) {
		pango_font_description_set_style(myFontDescription, newStyle);
		fontChanged = true;
	}

	if (fontChanged) {
		if (myContext != 0) {
			if(myAnalysis.font)
				pango_fc_font_unlock_face((PangoFcFont*)myAnalysis.font);

			myAnalysis.font = pango_context_load_font(myContext, myFontDescription);
			myAnalysis.shape_engine = pango_font_find_shaper(myAnalysis.font, 0, 0);
			PangoFontMetrics *metrics = pango_font_get_metrics(myAnalysis.font, myAnalysis.language);
			myDescent = pango_font_metrics_get_descent(metrics) / PANGO_SCALE;

			face = pango_fc_font_lock_face((PangoFcFont*)myAnalysis.font);

			Font *fc = &fontCache[face];
			charWidthCache = &fc->charWidthCacheAll;
			glyphCache = &fc->glyphCacheAll;
			kerningCache = &fc->kerningCacheAll;
			glyphIdxCache = &fc->glyphIdxCacheAll;
		}
		myStringHeight = -1;
		mySpaceWidth = -1;
	}
}

void ZLEwlPaintContext::setColor(ZLColor color, LineStyle style) {
	fColor = (0.299 * color.Red + 0.587 * color.Green + 0.114 * color.Blue ) / 64;
}

void ZLEwlPaintContext::setFillColor(ZLColor color, FillStyle style) {
	//fColor = color;
	fColor = (0.299 * color.Red + 0.587 * color.Green + 0.114 * color.Blue ) / 64;
	fColor = pal[fColor & 3];
}

/*int ZLEwlPaintContext::stringWidth(const char *str, int len) const {
	if (myContext == 0) {
		return 0;
	}

	if (!g_utf8_validate(str, len, 0)) {
		return 0;
	}

	if(!len)
		return 0;

	pango_shape(str, len, &myAnalysis, myString);
	PangoRectangle logicalRectangle;
	pango_glyph_string_extents(myString, myAnalysis.font, 0, &logicalRectangle);
	return (logicalRectangle.width + PANGO_SCALE / 2) / PANGO_SCALE;
}
*/

int ZLEwlPaintContext::stringWidth(const char *str, int len, bool rtl) const {
	int w = 0;
	int ch_w;
	char *p = (char *)str;
	unsigned long         codepoint;
	unsigned char         in_code;
	int                   expect;
	FT_UInt glyph_idx = 0;
	FT_UInt previous;
	FT_Bool use_kerning;
	FT_Vector delta; 
	int kerning = 0;

	use_kerning = face->face_flags & FT_FACE_FLAG_KERNING;

	while ( *p && len-- > 0)
	{
		in_code = *p++ ;

		if ( in_code >= 0xC0 )
		{
			if ( in_code < 0xE0 )           /*  U+0080 - U+07FF   */
			{
				expect = 1;
				codepoint = in_code & 0x1F;
			}
			else if ( in_code < 0xF0 )      /*  U+0800 - U+FFFF   */
			{
				expect = 2;
				codepoint = in_code & 0x0F;
			}
			else if ( in_code < 0xF8 )      /* U+10000 - U+10FFFF */
			{
				expect = 3;
				codepoint = in_code & 0x07;
			}
			continue;
		}
		else if ( in_code >= 0x80 )
		{
			--expect;

			if ( expect >= 0 )
			{
				codepoint <<= 6;
				codepoint  += in_code & 0x3F;
			}
			if ( expect >  0 )
				continue;

			expect = 0;
		}
		else                              /* ASCII, U+0000 - U+007F */
			codepoint = in_code;

		if(glyphIdxCache->find(codepoint) != glyphIdxCache->end()) {
			glyph_idx = (*glyphIdxCache)[codepoint];
		} else {
			glyph_idx = FT_Get_Char_Index(face, codepoint);

			(*glyphIdxCache)[codepoint] = glyph_idx;
		}

		if ( use_kerning && previous && glyph_idx ) { 
			if((kerningCache->find(glyph_idx) != kerningCache->end()) &&
					((*kerningCache)[glyph_idx].find(previous) != (*kerningCache)[glyph_idx].end())) {

				kerning = ((*kerningCache)[glyph_idx])[previous];
			} else {

				FT_Get_Kerning(face, previous, glyph_idx, FT_KERNING_DEFAULT, &delta ); 
				kerning = delta.x >> 6;

				int *k = &((*kerningCache)[glyph_idx])[previous];
				*k = kerning;
			}
		} else 
			kerning = 0;

		if(charWidthCache->find(codepoint) != charWidthCache->end()) {
			w += (*charWidthCache)[codepoint] + kerning;
		} else {
			if(!FT_Load_Glyph(face, glyph_idx,  FT_LOAD_RENDER | FT_LOAD_DEFAULT)) { 
				ch_w = (face->glyph->advance.x + 63) >> 6; // or face->glyph->metrics->horiAdvance >> 6
				w += ch_w + kerning;
				charWidthCache->insert(std::make_pair(codepoint, ch_w));
			} 
		}
		previous = glyph_idx;
	}

	return w;
}

int ZLEwlPaintContext::spaceWidth() const {
	if (mySpaceWidth == -1) {
		mySpaceWidth = stringWidth(" ", 1, false);
	}
	return mySpaceWidth;
}

int ZLEwlPaintContext::stringHeight() const {
	if (myFontDescription == 0) {
		return 0;
	}
	if (myStringHeight == -1) {
		if (pango_font_description_get_size_is_absolute(myFontDescription)) {
			myStringHeight = pango_font_description_get_size(myFontDescription) / PANGO_SCALE + 2;
		} else {
			myStringHeight = pango_font_description_get_size(myFontDescription) * 170 / 72 / PANGO_SCALE + 2;
		}
	}
	return myStringHeight;
}

int ZLEwlPaintContext::descent() const {
	return myDescent;
}

/*void ZLEwlPaintContext::drawString(int x, int y, const char *str, int len) {
	if (!g_utf8_validate(str, len, 0)) {
		return;
	}

	pango_shape(str, len, &myAnalysis, myString);

	PangoRectangle logR;
	PangoRectangle inkR;
	pango_glyph_string_extents(myString, myAnalysis.font, &inkR, &logR);

	int s_width;

	if(PANGO_RBEARING(inkR) > PANGO_RBEARING(logR))
		s_width = PANGO_RBEARING(inkR);
	else {
		s_width = PANGO_LBEARING(inkR);
		if(s_width < 0)
			s_width = - s_width + logR.width;
		else
			s_width = logR.width;
	}

	if(!ft2bmp)
		ft2bmp = createFTBitmap(
				(s_width + PANGO_SCALE / 2) / PANGO_SCALE,
				(logR.height + PANGO_SCALE / 2) / PANGO_SCALE);
	else
		modifyFTBitmap(
				ft2bmp,
				(s_width + PANGO_SCALE / 2) / PANGO_SCALE,
				(logR.height + PANGO_SCALE / 2) / PANGO_SCALE);

	int lb;
	lb = (PANGO_LBEARING(inkR) + PANGO_SCALE / 2) / PANGO_SCALE;
	if(lb > 0)
		lb = 0;

	pango_ft2_render(ft2bmp, myAnalysis.font, myString, -lb , ft2bmp->rows - 1 - myDescent);

	unsigned char val;
	unsigned char *p_ft = (unsigned char *)ft2bmp->buffer;
	for(int i = ft2bmp->rows - 1; i >= 0; i--) {
			for (int k = 0; k < ft2bmp->width; k++) {

				if((x + k) < 0)
					continue;

				if(p_ft[k] == 0)
					continue;

				val = 0x55 * ((unsigned char)~p_ft[k] / 64);
				image[x + k + (y - i + myDescent) * myWidth] = (255 << 24) | (val << 16) | (val << 8) | val;
			}
		p_ft += ft2bmp->pitch;
	}
}
*/

void ZLEwlPaintContext::drawString(int x, int y, const char *str, int len, bool rtl) {
/*	FT_Face _face = pango_fc_font_lock_face((PangoFcFont*)myAnalysis.font);
	FT_Face *face = &_face;

	Font *fc = &fontCache[_face];
	charWidthCache = &fc->charWidthCacheAll;
	glyphCache = &fc->glyphCacheAll;
	kerningCache = &fc->kerningCacheAll;
	glyphIdxCache = &fc->glyphIdxCacheAll;
*/

	FT_GlyphSlot  slot = face->glyph;
	FT_BitmapGlyph glyph;
	FT_BitmapGlyph *pglyph;
	FT_Matrix     matrix;                 /* transformation matrix */
	FT_Vector     pen;                    /* untransformed origin  */

	FT_UInt glyph_idx = 0;
	FT_UInt previous;
	FT_Bool use_kerning;
	FT_Vector delta; 

	use_kerning = face->face_flags & FT_FACE_FLAG_KERNING;

	char *p = (char *)str;
	unsigned long         codepoint;
	unsigned char         in_code;
	int                   expect;
	int kerning;

	//	bool mark = false;


	pen.x = x;
	pen.y = y;


	while ( *p && len--)
	{
		in_code = *p++ ;

		if ( in_code >= 0xC0 )
		{
			if ( in_code < 0xE0 )           /*  U+0080 - U+07FF   */
			{
				expect = 1;
				codepoint = in_code & 0x1F;
			}
			else if ( in_code < 0xF0 )      /*  U+0800 - U+FFFF   */
			{
				expect = 2;
				codepoint = in_code & 0x0F;
			}
			else if ( in_code < 0xF8 )      /* U+10000 - U+10FFFF */
			{
				expect = 3;
				codepoint = in_code & 0x07;
			}
			continue;
		}
		else if ( in_code >= 0x80 )
		{
			--expect;

			if ( expect >= 0 )
			{
				codepoint <<= 6;
				codepoint  += in_code & 0x3F;
			}
			if ( expect >  0 )
				continue;

			expect = 0;
		}
		else                              /* ASCII, U+0000 - U+007F */
			codepoint = in_code;

		if(glyphIdxCache->find(codepoint) != glyphIdxCache->end()) {
			glyph_idx = (*glyphIdxCache)[codepoint];
		} else {
			glyph_idx = FT_Get_Char_Index(face, codepoint);

			(*glyphIdxCache)[codepoint] = glyph_idx;
		}

		if ( use_kerning && previous && glyph_idx ) { 
			if((kerningCache->find(glyph_idx) != kerningCache->end()) &&
					((*kerningCache)[glyph_idx].find(previous) != (*kerningCache)[glyph_idx].end())) {

				kerning = ((*kerningCache)[glyph_idx])[previous];
			} else {

				FT_Get_Kerning(face, previous, glyph_idx, FT_KERNING_DEFAULT, &delta ); 
				kerning = delta.x >> 6;

				int *k = &((*kerningCache)[glyph_idx])[previous];
				*k = kerning;
			}
			pen.x += kerning;		
		}

		if(glyphCache->find(codepoint) != glyphCache->end()) { 
			//printf("glyph cache hit\n");
			pglyph = &(*glyphCache)[codepoint];
		} else {
			//printf("glyph cache miss\n");
			if(FT_Load_Glyph(face, glyph_idx,  FT_LOAD_RENDER | FT_LOAD_TARGET_NORMAL | FT_RENDER_MODE_NORMAL)) {
				continue;
			}	

			FT_Get_Glyph(slot, (FT_Glyph*)&glyph);

			glyph->root.advance.x = slot->advance.x;	  			

			(*glyphCache)[codepoint] = glyph;		  
			pglyph = &glyph;
		}

		drawGlyph( &(*pglyph)->bitmap,
				pen.x + (*pglyph)->left,
				pen.y - (*pglyph)->top);


		/*		if(!mark) {
				drawLine(pen.x + (*pglyph)->left, y+1, pen.x + ((*pglyph)->root.advance.x >> 6), y+1);
				mark = true;
				}
				*/		

		/* increment pen position */
		pen.x += (*pglyph)->root.advance.x >> 6;
		previous = glyph_idx;
	}

	if(fColor >= 2)
		invertRegion(x, y - stringHeight() + stringHeight() / 4, pen.x, y + stringHeight() / 5);

//	pango_fc_font_unlock_face((PangoFcFont*)myAnalysis.font);
}

void ZLEwlPaintContext::drawImage(int x, int y, const ZLImageData &image) {
	char *c;
	char *c_src;
	int s, s_src;
	int iW = image.width();
	int iH = image.height();
	unsigned char val;

	if((iW + x) > myWidth || y > myHeight)
		return;

	ZLEwlImageData *source_image = (ZLEwlImageData *)&image;

	char *src = source_image->getImageData();

	for(int j = 0; j < iH; j++)
		for(int i = 0; i < iW; i++) {
			c_src = src + i / 4 + iW * j / 4;
			s_src = (i & 3) << 1;

			val = (*c_src << s_src) & 0xc0;

			if(val == 0xc0)
				continue;

			if(val == 0x00)
				xcb_image_put_pixel (this->image, i+x, j + (y - iH), pal[0]);
			else if(val == 0x40)
				xcb_image_put_pixel (this->image, i+x, j + (y - iH), pal[1]);
			else
				xcb_image_put_pixel (this->image, i+x, j + (y - iH), pal[2]);
		}
}

void ZLEwlPaintContext::drawLine(int x0, int y0, int x1, int y1) {
	drawLine(x0, y0, x1, y1, false);
}

void ZLEwlPaintContext::drawLine(int x0, int y0, int x1, int y1, bool fill) {
	int i, j;
	int k, s;
	int p;
	bool done = false;

	if(x1 != x0) {
		k = (y1 - y0) / (x1 - x0);
		j = y0;
		i = x0;

		do {
			if(i == x1)
				done = true;

			if(fill)
				xcb_image_put_pixel (image, i, j, fColor);
			else
				xcb_image_put_pixel (image, i, j, pal[0]);

			j += k;

			if(x1 > x0)
				i++;
			else 
				i--;

		} while(!done);

	} else {
		i = x0;
		j = y0;

		do {
			if(j == y1)
				done = true;

			if(fill)
				xcb_image_put_pixel (image, i, j, fColor);
			else
				xcb_image_put_pixel (image, i, j, pal[0]);

			if(y1 > y0)
				j++;
			else if(y1 < y0)
				j--;

		} while(!done);
	}
}

void ZLEwlPaintContext::fillRectangle(int x0, int y0, int x1, int y1) {
	int j;

	j = y0;
	do {
		drawLine(x0, j, x1, j, true);

		if(y1 > y0)
			j++;
		else if(y1 < y0)
			j--;
		else
			return;
	} while(((y1 > y0) && (j <= y1)) || ((y1 < y0) && (j >= y1)));
}

void ZLEwlPaintContext::drawFilledCircle(int x, int y, int r) {
}

void ZLEwlPaintContext::clear(ZLColor color) {
	memset(image->data, pal[3], image->width * image->height * image->bpp / 8);
}

int ZLEwlPaintContext::width() const {
	return myWidth;
}

int ZLEwlPaintContext::height() const {
	return myHeight;
}

void ZLEwlPaintContext::setFTBitmap(FT_Bitmap *bitmap, int width, int height)
{
	bitmap->width = width;
	bitmap->rows = height;
	bitmap->pitch = (width + 3) & ~3;
	bitmap->num_grays = 256;
	bitmap->pixel_mode = FT_PIXEL_MODE_GRAY;
}

FT_Bitmap * ZLEwlPaintContext::createFTBitmap(int width, int height)
{
	FT_Bitmap *bitmap;
	bitmap = (FT_Bitmap*)g_malloc(sizeof(FT_Bitmap));
	setFTBitmap(bitmap, width, height);
	bitmap->buffer = g_new0(guchar, bitmap->pitch * bitmap->rows);
	return bitmap;
}

void ZLEwlPaintContext::freeFTBitmap(FT_Bitmap *bitmap)
{
	if (bitmap) {
		g_free(bitmap->buffer);
		g_free(bitmap);
		bitmap = NULL;
	}
}

void ZLEwlPaintContext::modifyFTBitmap(FT_Bitmap *bitmap, int width, int height)
{
	if (bitmap->width != width || bitmap->rows != height) {
		setFTBitmap(bitmap, width, height);
		bitmap->buffer = (unsigned char *)g_realloc(bitmap->buffer, bitmap->pitch * bitmap->rows);
	}
	clearFTBitmap(bitmap);
}

void ZLEwlPaintContext::clearFTBitmap(FT_Bitmap *bitmap)
{
	unsigned char *p = (unsigned char *)bitmap->buffer;
	int length = bitmap->pitch * bitmap->rows;
	memset(p, 0, length);
}

void ZLEwlPaintContext::drawGlyph(FT_Bitmap* bitmap, FT_Int x, FT_Int y)
{
	FT_Int  i, j, p, q;
	FT_Int  x_max = x + bitmap->width;
	FT_Int  y_max = y + bitmap->rows;
	unsigned char val;
	int s;

	for ( i = x, p = 0; i < x_max; i++, p++ ) {
		for ( j = y, q = 0; j < y_max; j++, q++ ) {

			if ( i < 0      || j < 0       ||
					i >= myWidth || j >= myHeight )
				continue;

			if(bitmap->pixel_mode == ft_pixel_mode_grays)
				val = bitmap->buffer[q * bitmap->pitch + p];
			else if(bitmap->pixel_mode == ft_pixel_mode_mono)
				val = (bitmap->buffer[q * bitmap->pitch + p / 8] & (1 << (7 - (p % 8)))) ? 255 : 0;
			else
				val = 0;

			if(val < 64)
				continue;

			val = 3 - val / 64;
			xcb_image_put_pixel (image, i, j, pal[val]);
		}
	}
}

void ZLEwlPaintContext::invertRegion(int x0, int y0, int x1, int y1)
{
	int pixel;
	for(int i = x0; i <= x1; i++) {
		for(int j = y0; j <= y1; j++) {
			pixel = 0xffffff & xcb_image_get_pixel(im, i, j);
			for(int idx = 0; idx < 4; idx++) {
				if(pixel == (0xffffff & pal[idx])) {
					xcb_image_put_pixel(im, i, j, pal[3 - idx]);
					break;
				}
			}
		}
	}
}
