#pragma once

//
// Copyright (c) 2009-2013 Mikko Mononen memon@inside.org
//
// This software is provided 'as-is', without any express or implied
// warranty.  In no event will the authors be held liable for any damages
// arising from the use of this software.
// Permission is granted to anyone to use this software for any purpose,
// including commercial applications, and to alter it and redistribute it
// freely, subject to the following restrictions:
// 1. The origin of this software must not be misrepresented; you must not
//	claim that you wrote the original software. If you use this software
//	in a product, an acknowledgment in the product documentation would be
//	appreciated but is not required.
// 2. Altered source versions must be plainly marked as such, and must not be
//	misrepresented as being the original software.
// 3. This notice may not be removed or altered from any source distribution.
//

#define FONS_INVALID -1
#include <stdlib.h>
#include <stb_truetype.h>
#include <vector>
#include <memory>
#include <string>
#include <filesystem>

#ifndef FONS_SCRATCH_BUF_SIZE
#	define FONS_SCRATCH_BUF_SIZE 64000
#endif
#ifndef FONS_HASH_LUT_SIZE
#	define FONS_HASH_LUT_SIZE 1024
#endif
#ifndef FONS_INIT_FONTS
#	define FONS_INIT_FONTS 4
#endif
#ifndef FONS_INIT_GLYPHS
#	define FONS_INIT_GLYPHS 512
#endif
#ifndef FONS_INIT_ATLAS_NODES
#	define FONS_INIT_ATLAS_NODES 512
#endif
#ifndef FONS_VERTEX_COUNT
#	define FONS_VERTEX_COUNT 2048
#endif
#ifndef FONS_MAX_STATES
#	define FONS_MAX_STATES 20
#endif
#ifndef FONS_MAX_FALLBACKS
#	define FONS_MAX_FALLBACKS 20
#endif


enum FONSflags
{
    FONS_ZERO_TOPLEFT = 1,
    FONS_ZERO_BOTTOMLEFT = 2,
};

enum FONSalign
{
    // Horizontal align
    FONS_ALIGN_LEFT = 1 << 0,
    // Default
    FONS_ALIGN_CENTER = 1 << 1,
    FONS_ALIGN_RIGHT = 1 << 2,
    // Vertical align
    FONS_ALIGN_TOP = 1 << 3,
    FONS_ALIGN_MIDDLE = 1 << 4,
    FONS_ALIGN_BOTTOM = 1 << 5,
    FONS_ALIGN_BASELINE = 1 << 6,
    // Default
};

enum FONSerrorCode
{
    // Font atlas is full.
    FONS_ATLAS_FULL = 1,
    // Scratch memory used to render glyphs is full, requested size reported in 'val', you may need to bump up FONS_SCRATCH_BUF_SIZE.
    FONS_SCRATCH_FULL = 2,
    // Calls to fonsPushState has created too large stack, if you need deep state stack bump up FONS_MAX_STATES.
    FONS_STATES_OVERFLOW = 3,
    // Trying to pop too many states fonsPopState().
    FONS_STATES_UNDERFLOW = 4,
};

struct FONSparams
{
    int width, height;
    unsigned char flags;
    void *userPtr;
    int (*renderCreate)(void *uptr, int width, int height);
    int (*renderResize)(void *uptr, int width, int height);
    void (*renderUpdate)(void *uptr, int *rect, const unsigned char *data);
    void (*renderDraw)(
        void *uptr,
        const float *verts,
        const float *tcoords,
        const unsigned int *colors,
        int nverts);
    void (*renderDelete)(void *uptr);
};

typedef struct FONSparams FONSparams;

struct FONSquad
{
    float x0, y0, s0, t0;
    float x1, y1, s1, t1;
};

typedef struct FONSquad FONSquad;

struct FONStextIter
{
    float x, y, nextx, nexty, scale, spacing;
    unsigned int codepoint;
    short isize, iblur;
    struct FONSfont *font;
    int prevGlyphIndex;
    const char *str;
    const char *next;
    const char *end;
    unsigned int utf8state;
};

typedef struct FONStextIter FONStextIter;

struct FONSglyph
{
    unsigned int codepoint;
    int index;
    int next;
    short size, blur;
    short x0, y0, x1, y1;
    short xadv, xoff, yoff;
};

typedef struct FONSglyph FONSglyph;

struct FONSttFontImpl
{
    stbtt_fontinfo font;
};

typedef struct FONSttFontImpl FONSttFontImpl;

struct FONSfont
{
    FONSttFontImpl font;
    std::string name;
    std::string data;
    int dataSize;
    float ascender;
    float descender;
    float lineh;
    std::vector<FONSglyph> glyphs;
    int lut[FONS_HASH_LUT_SIZE];
    std::vector<int> fallbacks;

    FONSglyph *fons__allocGlyph();
};

typedef struct FONSfont FONSfont;

struct FONSstate
{
    int font = 0;
    int align = FONS_ALIGN_LEFT | FONS_ALIGN_BASELINE;
    float size = 12.f;
    unsigned int color = 0xffffffff;
    float blur = 0;
    float spacing = 0;
};

typedef struct FONSstate FONSstate;

struct FONSatlasNode
{
    short x, y, width;
};

typedef struct FONSatlasNode FONSatlasNode;

struct FONSatlas
{
    int width, height;
    std::vector<FONSatlasNode> nodes;
    int fons__atlasInsertNode(int idx, int x, int y, int w);
    void fons__atlasRemoveNode(int idx);
    void fons__atlasExpand( int w, int h);
    void fons__atlasReset(int w, int h);
    int fons__atlasAddSkylineLevel(
        int idx,
        int x,
        int y,
        int w,
        int h);
    int fons__atlasAddRect(int rw, int rh, int *rx, int *ry);
    int fons__atlasRectFits(int i, int w, int h);
};

typedef struct FONSatlas FONSatlas;

struct FONScontext
{
    FONSparams params;
    float itw = 0 , ith = 0;
    std::unique_ptr<unsigned char[]> texData;
    int dirtyRect[4] = { 0 };
    std::vector<FONSfont> fonts;
    FONSatlas atlas;
    std::vector<float> verts;
    std::vector<float> tcoords;
    std::vector<unsigned int> colors;
    std::unique_ptr<unsigned char[]> scratch;
    int nscratch;
    std::vector<FONSstate> states;
    void (*handleError)(void *uptr, int error, int val);
    void *errorUptr;

    void fons__addWhiteRect(int w, int h);
    FONSstate *getState();

    void vertex(
        float x,
        float y,
        float s,
        float t,
        unsigned int c);
    float getVerticalAlign(FONSfont *font, int align, short isize);

    FONSglyph *getGlyph(
        FONSfont *font,
        unsigned int codepoint,
        short isize,
        short iblur);

    void fons__blur(
        unsigned char *dst,
        int w,
        int h,
        int dstStride,
        int blur);

    void fons__getQuad(
        FONSfont *font,
        int prevGlyphIndex,
        FONSglyph *glyph,
        float scale,
        float spacing,
        float *x,
        float *y,
        FONSquad *q);

    void fons__allocAtlas(int w, int h);

    void init(FONSparams params);
    ~FONScontext();

    void fonsSetErrorCallback(
        void (*callback)(void *uptr, int error, int val),
        void *uptr);
    // Returns current atlas size.
    void fonsGetAtlasSize(int *width, int *height);
    // Expands the atlas size.
    int fonsExpandAtlas(int width, int height);
    // Resets the whole stash.
    int fonsResetAtlas(int width, int height);

    // Add fonts
    int fonsAddFont(std::string name, const std::filesystem::path &path);
    int fonsAddFontMem(
        std::string name,
        std::string data);
    int fonsGetFontByName(const char *name);
    int addFallbackFont(int base, int fallback);

    // State handling
    void pushState();
    void popState();
    void clearState();

    // Draw text
    // returns next horizontal position
    float drawText(float x, float y, std::string_view str);
    float fonsDrawTextWithTransition(
        float x,
        float y,
        const char *string,
        const char *end,
        float);

    // Measure text
    float fonsTextBounds(
        float x,
        float y,
        std::string_view str,
        float *bounds);
    void fonsLineBounds(float y, float *miny, float *maxy);
    void fonsVertMetrics(float *ascender, float *descender, float *lineh);

    // Text iterator
    int fonsTextIterInit(
        FONStextIter *iter,
        float x,
        float y,
        const char *str,
        const char *end);
    int fonsTextIterNext(FONStextIter *iter, struct FONSquad *quad);

    // Pull texture changes
    const unsigned char * fonsGetTextureData(int *width, int *height);
    int fonsValidateTexture(int *dirty);

    // Draws the stash texture for debugging
    void fonsDrawDebug(float x, float y);

    void flush();
};
