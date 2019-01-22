#pragma once

#include "Common/Core.h"
#include "Platform/Platform.h"
#include "Utility/String.h"

// We will only be supporting PNG currently
// TODO(Zero): Correctly remove all STD libs from here
#define STBI_ONLY_PNG
#define STBI_NO_GIF
#define STBI_NO_STDIO
#define STBI_ASSERT(s)			a3Assert(s)
#define STBI_MALLOC(sz)			a3::Platform.Malloc(sz)
#define STBI_REALLOC(p,newsz)	a3::Platform.Realloc(p, (newsz))
#define STBI_FREE(p)			a3::Platform.Free(p)
#if defined(A3DEBUG) || defined(A3INTERNAL)
#define STBI_FAILURE_USERMSG
#else
#define STBI_NO_FAILURE_STRINGS
#endif
#define STB_IMAGE_STATIC
#include "External/STBImage.h"

#define STBTT_ifloor(x)			((i32) Floorf(x))
#define STBTT_iceil(x)			((i32) Ceilf(x))
#define STBTT_sqrt(x)			Sqrtf(x)
#define STBTT_pow(x,y)			Powf(x,y)
#define STBTT_fmod(x,y)			FModf(x,y)
#define STBTT_cos(x)			Cosf(x)
#define STBTT_acos(x)			ArcCosf(x)
#define STBTT_fabs(x)			FAbsf(x)
#define STBTT_strlen(x)			a3::GetStringLength(x)
#define STBTT_memcpy			a3::MemoryCopy
#define STBTT_memset			a3::MemorySet
#define STBTT_MALLOC(sz)		a3::Platform.Malloc(sz)
#define STBTT_REALLOC(p,newsz)	a3::Platform.Realloc(p, newsz)
#define STBTT_FREE(p)			a3::Platform.Free(p)
#define STBTT_malloc(x,u)		((void)(u),a3::Platform.Malloc(x))
#define STBTT_free(x,u)			((void)(u),a3::Platform.Free(x))
#define STBTT_assert(x)			a3Assert(x)
#define STBTT_STATIC
#include "External/STBTrueType.h"

#define STBI_MSC_SECURE_CRT
#define STB_IMAGE_WRITE_STATIC
#define STBIW_ASSERT(x)			a3Assert(x)
#define STBIW_MALLOC(s)			a3::Platform.Malloc(s)
#define STBIW_REALLOC(p, s)		a3::Platform.Realloc(p, s)
#define STBIW_FREE(p)			a3::Platform.Free(p)
#define STBIW_MEMMOVE			a3::MemoryMove
#define STBI_WRITE_NO_STDIO
#include "External/STBImageWrite.h"

