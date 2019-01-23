#define STB_IMAGE_IMPLEMENTATION
#define STB_IMAGE_WRITE_IMPLEMENTATION
#define STB_TRUETYPE_IMPLEMENTATION
#include "Resource.h"

#define A3_MAX_LOAD_GLYPH (A3MAXLOADGLYPHX * A3MAXLOADGLYPHY)

struct a3_buffer
{
	void* buffer;
	u64 size;
};

static inline void a3_STBWriteCallback(void* context, void* data, i32 size)
{
	a3::file_content* fc = (a3::file_content*)context;
	fc->Buffer = new u8[size];
	if (fc->Buffer)
	{
		fc->Size = size;
		a3::MemoryCopy(fc->Buffer, data, size);
	}
}

static inline void a3_STBWriteBufferCallback(void* context, void* data, i32 size)
{
	a3_buffer* fc = (a3_buffer*)context;
	fc->buffer = new u8[size];
	if (fc->buffer)
	{
		fc->size = size;
		a3::MemoryCopy(fc->buffer, data, size);
	}
}

u64 a3::QueryImageSize(void * buffer, i32 length)
{
	stbi_set_flip_vertically_on_load(1);
	i32 w, h, n;
	stbi_info_from_memory((u8*)buffer, length, &w, &h, &n);
	return w * h * n * sizeof(u8);
}

a3::image a3::LoadImageFromBufer(void * imgeBuffer, i32 length, void * destination)
{
	i32 x, y, n;
	stbi_info_from_memory((u8*)imgeBuffer, length, &x, &y, &n);
	u8* pixels = stbi_load_from_memory((u8*)imgeBuffer, length, &x, &y, &n, n);
	a3::image result;
	result.Width = x;
	result.Height = y;
	result.Channels = n;
	result.Pixels = (u8*)destination;
	a3::MemoryCopy(destination, pixels, x*y*n);
	stbi_image_free(pixels);
	return result;
}

b32 a3::WriteImageToBuffer(void * destination, i32 width, i32 height, i32 channels, i32 bytesPerPixel, void * pixels)
{
	stbi_flip_vertically_on_write(1);
	// NOTE(Zero): Stride for images should align to multiple of 4 or 8
	// This is done for SSE optimizations and such
	// Stride reference : https://en.wikipedia.org/wiki/Stride_of_an_array
	// Here the stride is for pitch of the image and not for the pixel
	// Byte alignment didn't work for the single channel image
	// `bytesPerPixel` is only use by images with 3 channel, it's not mistake
	i32 stride;
	if (channels == 1)
		stride = width;
	else if (channels == 3) // TODO(Zero): Test for images with 3 channels
		stride = 4 * ((width * bytesPerPixel + 3) / 4);
	else if (channels == 4)
		stride = width;
	else
		return false; // NOTE(Zero): Ouput only for 1, 3 and 4 channel images

	a3_buffer buffer = {};
	if (stbi_write_png_to_func(a3_STBWriteBufferCallback, &buffer, width, height, channels, pixels, stride))
	{
		a3::MemoryCopy(destination, buffer.buffer, buffer.size);
		delete[] buffer.buffer;
		return true;
	}
	return false;
}

static inline void a3_CalculateFontBitmapMaxDimension(stbtt_fontinfo& info, f32 scale, i32* w, i32* h)
{
	// NOTE(Zero):
	// Maximum dimention for the bitmap required by the largest character in the font
	// Maximum height is set as the given scale factor
	// 0.5 is added to properly ceil when casting to integer
	// 1 is added to the height so that there's a pixel of gap between each bitmaps
	// If 1 is not added, there may be chance that a line of another bitmap may be shown
	i32 maxWidth = 0;
	i32 maxHeight = (u32)(scale + 0.5f) + 1;

	// NOTE(Zero):
	// Now the scale is changed into unit of points and it is no more pixel
	f32 pscale = stbtt_ScaleForPixelHeight(&info, scale);

	for (i32 index = 0; index < A3_MAX_LOAD_GLYPH; ++index)
	{
		int glyphIndex = stbtt_FindGlyphIndex(&info, index);
		i32 leftSizeBearing; // NOTE(Zero): Ignored
		i32 advance;
		stbtt_GetGlyphHMetrics(&info, glyphIndex, &advance, &leftSizeBearing);
		if (stbtt_IsGlyphEmpty(&info, glyphIndex))
		{
			continue;
		}
		else
		{
			i32 x0, x1, y0, y1;
			stbtt_GetGlyphBitmapBox(&info, glyphIndex, pscale, pscale, &x0, &y0, &x1, &y1);
			i32 w = x1 - x0; i32 h = y1 - y0;
			if (maxWidth < w)maxWidth = w;
		}
	}
	*w = maxWidth;
	*h = maxHeight;
}

u64 a3::QueryFontSize(void * buffer, i32 length, f32 scale)
{
	stbtt_fontinfo info;
	stbtt_InitFont(&info, (u8*)buffer, stbtt_GetFontOffsetForIndex((u8*)buffer, 0));
	i32 mw, mh;
	a3_CalculateFontBitmapMaxDimension(info, scale, &mw, &mh);
	return (sizeof(u8) * A3_MAX_LOAD_GLYPH * mw * mh);
}

a3::font a3::LoadFontFromBuffer(void * buffer, f32 scale, void * destination)
{
	stbtt_fontinfo info;
	stbtt_InitFont(&info, (u8*)buffer, stbtt_GetFontOffsetForIndex((u8*)buffer, 0));
	a3::font result;
	result.Info = info;
	result.HeightInPixels = scale;

	// NOTE(Zero):
	// These temp buffers are used to store extracted bitmap from stb
	// This is requied because stb extracts bitmap from top-bottom
	// But we use from bottom-top, so we use this buffer to flip the bitmap vertically
	u8* tempBuffer = A3NULL;
	u64 tempBufferSize = 0;

	// NOTE(Zero):
	// Maximum dimention for the bitmap required by the largest character in the font
	// Maximum height is set as the given scale factor
	// 0.5 is added to properly ceil when casting to integer
	// 1 is added to the height so that there's a pixel of gap between each bitmaps
	// If 1 is not added, there may be chance that a line of another bitmap may be shown
	i32 maxWidth = 0;
	i32 maxHeight = (u32)(scale + 0.5f) + 1;

	// NOTE(Zero):
	// Now the scale is changed into unit of points and it is no more pixel
	f32 pscale = stbtt_ScaleForPixelHeight(&result.Info, scale);
	result.ScalingFactor = pscale;

	u8* bitmaps[A3_MAX_LOAD_GLYPH];
	for (i32 index = 0; index < A3_MAX_LOAD_GLYPH; ++index)
	{
		a3::character* c = &result.Characters[index];
		c->GlyphIndex = stbtt_FindGlyphIndex(&result.Info, index);
		i32 leftSizeBearing; // NOTE(Zero): Ignored
		i32 advance;
		stbtt_GetGlyphHMetrics(&result.Info, c->GlyphIndex, &advance, &leftSizeBearing);
		c->Advance = (f32)advance * pscale;
		if (stbtt_IsGlyphEmpty(&result.Info, c->GlyphIndex))
		{
			c->HasBitmap = false;
			continue;
		}
		else
		{
			c->HasBitmap = true;
			i32 x0, x1, y0, y1;
			stbtt_GetGlyphBitmapBox(&result.Info, c->GlyphIndex, pscale, pscale, &x0, &y0, &x1, &y1);
			i32 w = x1 - x0; i32 h = y1 - y0;
			if (tempBufferSize < w*h)
			{
				tempBuffer = (u8*)a3::Platform.Realloc(tempBuffer, w*h);
				tempBufferSize = w * h;
			}
			// NOTE(Zero): Here stride is equal to width because OpenGL wants packed pixels
			i32 stride = w;
			stbtt_MakeGlyphBitmap(&result.Info, tempBuffer, w, h, stride, pscale, pscale, c->GlyphIndex);
			c->OffsetX = x0;
			c->OffsetY = -y1;
			c->Width = w;
			c->Height = h;
			bitmaps[index] = new u8[w*h];
			a3::ReverseRectCopy(bitmaps[index], tempBuffer, w, h);
			if (maxWidth < w)maxWidth = w;
		}
	}
	a3::Platform.Free(tempBuffer);

	// NOTE(Zero):
	// Here we multiple max dimensions by 16 to get altas dimension
	// We extract bitmaps for 256(0-255) characters from the font file
	// sqrt(256) = 16, thus 16 is used here
	result.AtlasWidth = A3MAXLOADGLYPHX * maxWidth;
	result.AtlasHeight = A3MAXLOADGLYPHY * maxHeight;
	result.Atlas = (u8*)destination;

	for (i32 blockY = 0; blockY < A3MAXLOADGLYPHY; ++blockY)
	{
		u8* destBlockY = result.Atlas + blockY * maxHeight * result.AtlasWidth;
		for (i32 blockX = 0; blockX < A3MAXLOADGLYPHX; ++blockX)
		{
			u8* destBlock = destBlockY + blockX * maxWidth;
			i32 index = blockY * A3MAXLOADGLYPHX + blockX;
			a3::character* c = &result.Characters[index];
			c->NormalX0 = (f32)(blockX * maxWidth) / (f32)result.AtlasWidth;
			c->NormalX1 = (f32)(blockX * maxWidth + maxWidth) / (f32)result.AtlasWidth;
			c->NormalY0 = (f32)(blockY * maxHeight) / (f32)result.AtlasHeight;
			c->NormalY1 = (f32)(blockY * maxHeight + scale) / (f32)result.AtlasHeight;
			if (c->HasBitmap)
			{
				u8* src = bitmaps[index];
				for (i32 y = 0; y < c->Height; ++y)
				{
					u8* dest = destBlock + y * result.AtlasWidth;
					for (i32 x = 0; x < c->Width; ++x)
					{
						*(dest++) = *(src++);
					}
				}
				c->Width = maxWidth;
				c->Height = maxHeight;
				delete[] bitmaps[index];
			}
		}
	}
	return result;
}

f32 a3::GetTTFontKernalAdvance(const font & font, i32 glyph0, i32 glyph1)
{
	i32 res = stbtt_GetGlyphKernAdvance(&font.Info, glyph0, glyph1);
	return res * font.ScalingFactor;
}