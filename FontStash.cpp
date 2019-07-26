#include "FontStash.hpp"

#include <stdio.h>
#include <math.h>
#include <stdexcept>
#include <utf8.h>
#include <Usagi/Utility/File.hpp>

#define FONS_NOTUSED(v)  (void)sizeof(v)

#ifdef FONS_USE_FREETYPE

#include <ft2build.h>
#include FT_FREETYPE_H
#include FT_ADVANCES_H
#include <math.h>

struct FONSttFontImpl {
    FT_Face font;
};
typedef struct FONSttFontImpl FONSttFontImpl;

FT_Library ftLibrary;

int fons__tt_init()
{
    FT_Error ftError;
    FONS_NOTUSED(context);
    ftError = FT_Init_FreeType(&ftLibrary);
    return ftError == 0;
}

int fons__tt_loadFont(FONScontext *context, FONSttFontImpl *font, unsigned char *data, int dataSize)
{
    FT_Error ftError;
    FONS_NOTUSED(context);

    //font->font.userdata = stash;
    ftError = FT_New_Memory_Face(ftLibrary, (const FT_Byte*)data, dataSize, 0, &font->font);
    return ftError == 0;
}

void fons__tt_getFontVMetrics(FONSttFontImpl *font, int *ascent, int *descent, int *lineGap)
{
    *ascent = font->font->ascender;
    *descent = font->font->descender;
    *lineGap = font->font->height - (*ascent - *descent);
}

float fons__tt_getPixelHeightScale(FONSttFontImpl *font, float size)
{
    return size / (font->font->ascender - font->font->descender);
}

int fons__tt_getGlyphIndex(FONSttFontImpl *font, int codepoint)
{
    return FT_Get_Char_Index(font->font, codepoint);
}

int fons__tt_buildGlyphBitmap(FONSttFontImpl *font, int glyph, float size, float scale,
    int *advance, int *lsb, int *x0, int *y0, int *x1, int *y1)
{
    FT_Error ftError;
    FT_GlyphSlot ftGlyph;
    FT_Fixed advFixed;
    FONS_NOTUSED(scale);

    ftError = FT_Set_Pixel_Sizes(font->font, 0, (FT_UInt)(size * (float)font->font->units_per_EM / (float)(font->font->ascender - font->font->descender)));
    if (ftError) return 0;
    ftError = FT_Load_Glyph(font->font, glyph, FT_LOAD_RENDER);
    if (ftError) return 0;
    ftError = FT_Get_Advance(font->font, glyph, FT_LOAD_NO_SCALE, &advFixed);
    if (ftError) return 0;
    ftGlyph = font->font->glyph;
    *advance = (int)advFixed;
    *lsb = (int)ftGlyph->metrics.horiBearingX;
    *x0 = ftGlyph->bitmap_left;
    *x1 = *x0 + ftGlyph->bitmap.width;
    *y0 = -ftGlyph->bitmap_top;
    *y1 = *y0 + ftGlyph->bitmap.rows;
    return 1;
}

void fons__tt_renderGlyphBitmap(FONSttFontImpl *font, unsigned char *output, int outWidth, int outHeight, int outStride,
    float scaleX, float scaleY, int glyph)
{
    FT_GlyphSlot ftGlyph = font->font->glyph;
    int ftGlyphOffset = 0;
    int x, y;
    FONS_NOTUSED(outWidth);
    FONS_NOTUSED(outHeight);
    FONS_NOTUSED(scaleX);
    FONS_NOTUSED(scaleY);
    FONS_NOTUSED(glyph);	// glyph has already been loaded by fons__tt_buildGlyphBitmap

    for ( y = 0; y < ftGlyph->bitmap.rows; y++ ) {
        for ( x = 0; x < ftGlyph->bitmap.width; x++ ) {
            output[(y * outStride) + x] = ftGlyph->bitmap.buffer[ftGlyphOffset++];
        }
    }
}

int fons__tt_getGlyphKernAdvance(FONSttFontImpl *font, int glyph1, int glyph2)
{
    FT_Vector ftKerning;
    FT_Get_Kerning(font->font, glyph1, glyph2, FT_KERNING_DEFAULT, &ftKerning);
    return (int)((ftKerning.x + 32) >> 6);  // Round up and convert to integer
}

#else

#include <Windows.h>
#define STB_TRUETYPE_IMPLEMENTATION
#define STBTT_STATIC
#include <stb_truetype.h>

int fons__tt_init(FONScontext *context)
{
    FONS_NOTUSED(context);
    return 1;
}

int fons__tt_loadFont(
    FONScontext *context,
    FONSttFontImpl *font,
    unsigned char *data,
    int dataSize)
{
    int stbError;
    FONS_NOTUSED(dataSize);

    font->font.userdata = context;
    stbError = stbtt_InitFont(&font->font, data, 0);
    return stbError;
}

void fons__tt_getFontVMetrics(
    FONSttFontImpl *font,
    int *ascent,
    int *descent,
    int *lineGap)
{
    stbtt_GetFontVMetrics(&font->font, ascent, descent, lineGap);
}

float fons__tt_getPixelHeightScale(FONSttFontImpl *font, float size)
{
    return stbtt_ScaleForPixelHeight(&font->font, size);
}

int fons__tt_getGlyphIndex(FONSttFontImpl *font, int codepoint)
{
    return stbtt_FindGlyphIndex(&font->font, codepoint);
}

int fons__tt_buildGlyphBitmap(
    FONSttFontImpl *font,
    int glyph,
    float size,
    float scale,
    int *advance,
    int *lsb,
    int *x0,
    int *y0,
    int *x1,
    int *y1)
{
    FONS_NOTUSED(size);
    stbtt_GetGlyphHMetrics(&font->font, glyph, advance, lsb);
    stbtt_GetGlyphBitmapBox(&font->font, glyph, scale, scale, x0, y0, x1, y1);
    return 1;
}

void fons__tt_renderGlyphBitmap(
    FONSttFontImpl *font,
    unsigned char *output,
    int outWidth,
    int outHeight,
    int outStride,
    float scaleX,
    float scaleY,
    int glyph)
{
    stbtt_MakeGlyphBitmap(&font->font, output, outWidth, outHeight, outStride,
        scaleX, scaleY, glyph);
}

int fons__tt_getGlyphKernAdvance(FONSttFontImpl *font, int glyph1, int glyph2)
{
    return stbtt_GetGlyphKernAdvance(&font->font, glyph1, glyph2);
}

#endif


unsigned int fons__hashint(unsigned int a)
{
    a += ~(a << 15);
    a ^= (a >> 10);
    a += (a << 3);
    a ^= (a >> 6);
    a += ~(a << 11);
    a ^= (a >> 16);
    return a;
}

int fons__mini(int a, int b)
{
    return a < b ? a : b;
}

int fons__maxi(int a, int b)
{
    return a > b ? a : b;
}

// Copyright (c) 2008-2010 Bjoern Hoehrmann <bjoern@hoehrmann.de>
// See http://bjoern.hoehrmann.de/utf-8/decoder/dfa/ for details.

#define FONS_UTF8_ACCEPT 0
#define FONS_UTF8_REJECT 12

unsigned int fons__decutf8(
    unsigned int *state,
    unsigned int *codep,
    unsigned int byte)
{
    static const unsigned char utf8d[] = {
        // The first part of the table maps bytes to character classes that
        // to reduce the size of the transition table and create bitmasks.
        0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
        0, 0, 0, 0, 0, 0, 0, 0,
        0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
        0, 0, 0, 0, 0, 0, 0, 0,
        0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
        0, 0, 0, 0, 0, 0, 0, 0,
        0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
        0, 0, 0, 0, 0, 0, 0, 0,
        1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 9, 9, 9, 9, 9, 9, 9, 9,
        9, 9, 9, 9, 9, 9, 9, 9,
        7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7,
        7, 7, 7, 7, 7, 7, 7, 7,
        8, 8, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2,
        2, 2, 2, 2, 2, 2, 2, 2,
        10, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 4, 3, 3, 11, 6, 6, 6, 5, 8, 8,
        8, 8, 8, 8, 8, 8, 8, 8, 8,

        // The second part is a transition table that maps a combination
        // of a state of the automaton and a character class to a state.
        0, 12, 24, 36, 60, 96, 84, 12, 12, 12, 48, 72, 12, 12, 12, 12, 12, 12,
        12, 12, 12, 12, 12, 12,
        12, 0, 12, 12, 12, 12, 12, 0, 12, 0, 12, 12, 12, 24, 12, 12, 12, 12, 12,
        24, 12, 24, 12, 12,
        12, 12, 12, 12, 12, 12, 12, 24, 12, 12, 12, 12, 12, 24, 12, 12, 12, 12,
        12, 12, 12, 24, 12, 12,
        12, 12, 12, 12, 12, 12, 12, 36, 12, 36, 12, 12, 12, 36, 12, 12, 12, 12,
        12, 36, 12, 36, 12, 12,
        12, 36, 12, 12, 12, 12, 12, 12, 12, 12, 12, 12,
    };

    unsigned int type = utf8d[byte];

    *codep = (*state != FONS_UTF8_ACCEPT)
                 ? (byte & 0x3fu) | (*codep << 6)
                 : (0xff >> type) & (byte);

    *state = utf8d[256 + *state + type];
    return *state;
}

// Atlas based on Skyline Bin Packer by Jukka Jylänki


void FONScontext::fons__allocAtlas(int w, int h)
{
    atlas.width = w;
    atlas.height = h;

    // Init root node.
    atlas.nodes.emplace_back();
    atlas.nodes[0].x = 0;
    atlas.nodes[0].y = 0;
    atlas.nodes[0].width = (short)w;
}

int FONSatlas::fons__atlasInsertNode(int idx, int x, int y, int w)
{
    nodes.insert(nodes.begin() + idx, { (short)x, (short)y, (short)w });

    return 1;
}

void FONSatlas::fons__atlasRemoveNode(int idx)
{
    nodes.erase(nodes.begin() + idx);
}

void FONSatlas::fons__atlasExpand(int w, int h)
{
    // Insert node for empty space
    if(w > width)
        fons__atlasInsertNode((int)nodes.size(), width, 0,
            w - width);
    width = w;
    height = h;
}

void FONSatlas::fons__atlasReset( int w, int h)
{
    width = w;
    height = h;
    nodes.resize(1);

    // Init root node.
    nodes[0].x = 0;
    nodes[0].y = 0;
    nodes[0].width = (short)w;
}

int FONSatlas::fons__atlasAddSkylineLevel(
    int idx,
    int x,
    int y,
    int w,
    int h)
{
    int i;

    // Insert new node
    if(fons__atlasInsertNode( idx, x, y + h, w) == 0)
        return 0;

    // Delete skyline segments that fall under the shadow of the new segment.
    for(i = idx + 1; i < nodes.size(); i++)
    {
        if(nodes[i].x < nodes[i - 1].x + nodes[i - 1].width
        )
        {
            int shrink = nodes[i - 1].x + nodes[i - 1].width -
                nodes[i].x;
            nodes[i].x += (short)shrink;
            nodes[i].width -= (short)shrink;
            if(nodes[i].width <= 0)
            {
                fons__atlasRemoveNode(i);
                i--;
            }
            else
            {
                break;
            }
        }
        else
        {
            break;
        }
    }

    // Merge same height skyline segments that are next to each other.
    for(i = 0; i < nodes.size()- 1; i++)
    {
        if(nodes[i].y == nodes[i + 1].y)
        {
            nodes[i].width += nodes[i + 1].width;
            fons__atlasRemoveNode(i + 1);
            i--;
        }
    }

    return 1;
}

int FONSatlas::fons__atlasRectFits(int i, int w, int h)
{
    // Checks if there is enough space at the location of skyline span 'i',
    // and return the max height of all skyline spans under that at that location,
    // (think tetris block being dropped at that position). Or -1 if no space found.
    int x = nodes[i].x;
    int y = nodes[i].y;
    int spaceLeft;
    if(x + w > width)
        return -1;
    spaceLeft = w;
    while(spaceLeft > 0)
    {
        if(i == nodes.size()) return -1;
        y = fons__maxi(y, nodes[i].y);
        if(y + h > height) return -1;
        spaceLeft -= nodes[i].width;
        ++i;
    }
    return y;
}

int FONSatlas::fons__atlasAddRect(int rw, int rh, int *rx, int *ry)
{
    int besth = height, bestw = width, besti = -1;
    int bestx = -1, besty = -1, i;

    // Bottom left fit heuristic.
    for(i = 0; i < nodes.size(); i++)
    {
        int y = fons__atlasRectFits( i, rw, rh);
        if(y != -1)
        {
            if(y + rh < besth || (y + rh == besth && nodes[i].width <
                bestw))
            {
                besti = i;
                bestw = nodes[i].width;
                besth = y + rh;
                bestx = nodes[i].x;
                besty = y;
            }
        }
    }

    if(besti == -1)
        return 0;

    // Perform the actual packing.
    if(fons__atlasAddSkylineLevel(besti, bestx, besty, rw, rh) == 0)
        return 0;

    *rx = bestx;
    *ry = besty;

    return 1;
}

void FONScontext::fons__addWhiteRect(int w, int h)
{
    int x, y, gx, gy;
    unsigned char *dst;
    if(atlas.fons__atlasAddRect( w, h, &gx, &gy) == 0)
        return;

    // Rasterize
    dst = &texData[gx + gy * params.width];
    for(y = 0; y < h; y++)
    {
        for(x = 0; x < w; x++)
            dst[x] = 0xff;
        dst += params.width;
    }

    dirtyRect[0] = fons__mini(dirtyRect[0], gx);
    dirtyRect[1] = fons__mini(dirtyRect[1], gy);
    dirtyRect[2] = fons__maxi(dirtyRect[2], gx + w);
    dirtyRect[3] = fons__maxi(dirtyRect[3], gy + h);
}

void FONScontext::init(FONSparams params_)
{
    params = params_;
    // Allocate scratch buffer.
    scratch.reset(new unsigned char[FONS_SCRATCH_BUF_SIZE]);

    // Initialize implementation library
    if(!fons__tt_init(this)) throw std::runtime_error("init failed");

    if(params.renderCreate != NULL)
    {
        if(params.renderCreate(params.userPtr, params.width, params.height) == 0
        )
            throw std::runtime_error("failed to create texture");
    }

    fons__allocAtlas(params.width, params.height);

    // Create texture for the cache.
    itw = 1.0f / params.width;
    ith = 1.0f / params.height;
    texData.reset(new unsigned char[params.width * params.height]);

    dirtyRect[0] = params.width;
    dirtyRect[1] = params.height;
    dirtyRect[2] = 0;
    dirtyRect[3] = 0;

    // Add white rect at 0,0 for debug drawing.
    fons__addWhiteRect(2, 2);

    pushState();
    clearState();
}

FONSstate * FONScontext::getState()
{
    return &states.back();
}

int FONScontext::addFallbackFont(int base, int fallback)
{
    FONSfont &baseFont = fonts[base];
    baseFont.fallbacks.push_back(fallback);
    return 1;
}

void FONScontext::pushState()
{
    states.push_back(states.back());
}

void FONScontext::popState()
{
    if(states.size() <= 1)
    {
        throw std::runtime_error("state stack underflow");
    }
    states.pop_back();
}

void FONScontext::clearState()
{
    FONSstate *state = getState();
    *state = FONSstate();
}

int FONScontext::fonsAddFont(std::string name, const std::filesystem::path &path)
{
    const auto data = usagi::readFileAsString(path);
    return fonsAddFontMem(std::move(name), std::move(data));
}

int FONScontext::fonsAddFontMem(
    std::string name,
    std::string data)
{
    int i, ascent, descent, fh, lineGap;
    FONSfont *font;

    fonts.emplace_back();
    auto idx = fonts.size() - 1;
    font = &fonts.back();

    font->name = std::move(name);

    // Init hash lookup.
    for(i = 0; i < FONS_HASH_LUT_SIZE; ++i)
        font->lut[i] = -1;

    // Read in the font data.
    font->data = std::move(data);

    // Init font
    nscratch = 0;
    if(!fons__tt_loadFont(this, &font->font, (unsigned char*)data.c_str(), (int)data.size()))
        throw std::runtime_error("failed to load font");

    // Store normalized line height. The real line height is got
    // by multiplying the lineh by font size.
    fons__tt_getFontVMetrics(&font->font, &ascent, &descent, &lineGap);
    fh = ascent - descent;
    font->ascender = (float)ascent / (float)fh;
    font->descender = (float)descent / (float)fh;
    font->lineh = (float)(fh + lineGap) / (float)fh;

    return (int)idx;
}

int FONScontext::fonsGetFontByName(const char *name)
{
    const auto iter = std::find_if(fonts.begin(), fonts.end(), [=](auto &&f) {
        return f.name == name;
    });
    if(iter == fonts.end())
        return FONS_INVALID;
    return (int)(iter - fonts.begin());
}


FONSglyph * FONSfont::fons__allocGlyph()
{
    return &glyphs.emplace_back();
}

// Based on Exponential blur, Jani Huhtanen, 2006

#define APREC 16
#define ZPREC 7

void fons__blurCols(unsigned char *dst, int w, int h, int dstStride, int alpha)
{
    int x, y;
    for(y = 0; y < h; y++)
    {
        int z = 0; // force zero border
        for(x = 1; x < w; x++)
        {
            z += (alpha * (((int)(dst[x]) << ZPREC) - z)) >> APREC;
            dst[x] = (unsigned char)(z >> ZPREC);
        }
        dst[w - 1] = 0; // force zero border
        z = 0;
        for(x = w - 2; x >= 0; x--)
        {
            z += (alpha * (((int)(dst[x]) << ZPREC) - z)) >> APREC;
            dst[x] = (unsigned char)(z >> ZPREC);
        }
        dst[0] = 0; // force zero border
        dst += dstStride;
    }
}

void fons__blurRows(unsigned char *dst, int w, int h, int dstStride, int alpha)
{
    int x, y;
    for(x = 0; x < w; x++)
    {
        int z = 0; // force zero border
        for(y = dstStride; y < h * dstStride; y += dstStride)
        {
            z += (alpha * (((int)(dst[y]) << ZPREC) - z)) >> APREC;
            dst[y] = (unsigned char)(z >> ZPREC);
        }
        dst[(h - 1) * dstStride] = 0; // force zero border
        z = 0;
        for(y = (h - 2) * dstStride; y >= 0; y -= dstStride)
        {
            z += (alpha * (((int)(dst[y]) << ZPREC) - z)) >> APREC;
            dst[y] = (unsigned char)(z >> ZPREC);
        }
        dst[0] = 0; // force zero border
        dst++;
    }
}


void FONScontext::fons__blur(
    unsigned char *dst,
    int w,
    int h,
    int dstStride,
    int blur)
{
    int alpha;
    float sigma;

    if(blur < 1)
        return;
    // Calculate the alpha such that 90% of the kernel is within the radius. (Kernel extends to infinity)
    sigma = (float)blur * 0.57735f; // 1 / sqrt(3)
    alpha = (int)((1 << APREC) * (1.0f - expf(-2.3f / (sigma + 1.0f))));
    fons__blurRows(dst, w, h, dstStride, alpha);
    fons__blurCols(dst, w, h, dstStride, alpha);
    fons__blurRows(dst, w, h, dstStride, alpha);
    fons__blurCols(dst, w, h, dstStride, alpha);
    //	fons__blurrows(dst, w, h, dstStride, alpha);
    //	fons__blurcols(dst, w, h, dstStride, alpha);
}

FONSglyph * FONScontext::getGlyph(
    FONSfont *font,
    unsigned int codepoint,
    short isize,
    short iblur)
{
    int i, g, advance, lsb, x0, y0, x1, y1, gw, gh, gx, gy, x, y;
    float scale;
    FONSglyph *glyph = NULL;
    unsigned int h;
    float size = isize / 10.0f;
    int pad, added;
    unsigned char *bdst;
    unsigned char *dst;
    FONSfont *renderFont = font;

    if(isize < 2) return NULL;
    if(iblur > 20) iblur = 20;
    pad = iblur + 2;

    // Reset allocator.
    nscratch = 0;

    // Find code point and size.
    h = fons__hashint(codepoint) & (FONS_HASH_LUT_SIZE - 1);
    i = font->lut[h];
    while(i != -1)
    {
        if(font->glyphs[i].codepoint == codepoint && font->glyphs[i].size ==
            isize && font->glyphs[i].blur == iblur)
            return &font->glyphs[i];
        i = font->glyphs[i].next;
    }

    // Could not find glyph, create it.
    g = fons__tt_getGlyphIndex(&font->font, codepoint);
    // Try to find the glyph in fallback fonts.
    if(g == 0)
    {
        for(i = 0; i < font->fallbacks.size(); ++i)
        {
            FONSfont *fallbackFont = &fonts[font->fallbacks[i]];
            int fallbackIndex = fons__tt_getGlyphIndex(&fallbackFont->font,
                codepoint);
            if(fallbackIndex != 0)
            {
                g = fallbackIndex;
                renderFont = fallbackFont;
                break;
            }
        }
        // It is possible that we did not find a fallback glyph.
        // In that case the glyph index 'g' is 0, and we'll proceed below and cache empty glyph.
    }
    scale = fons__tt_getPixelHeightScale(&renderFont->font, size);
    fons__tt_buildGlyphBitmap(&renderFont->font, g, size, scale, &advance, &lsb,
        &x0, &y0, &x1, &y1);
    gw = x1 - x0 + pad * 2;
    gh = y1 - y0 + pad * 2;

    // Find free spot for the rect in the atlas
    added = atlas.fons__atlasAddRect(gw, gh, &gx, &gy);
    if(added == 0 && handleError != NULL)
    {
        // Atlas is full, let the user to resize the atlas (or not), and try again.
        handleError(errorUptr, FONS_ATLAS_FULL, 0);
        added = atlas.fons__atlasAddRect(gw, gh, &gx, &gy);
    }
    if(added == 0) return NULL;

    // Init glyph.
    glyph = font->fons__allocGlyph();
    glyph->codepoint = codepoint;
    glyph->size = isize;
    glyph->blur = iblur;
    glyph->index = g;
    glyph->x0 = (short)gx;
    glyph->y0 = (short)gy;
    glyph->x1 = (short)(glyph->x0 + gw);
    glyph->y1 = (short)(glyph->y0 + gh);
    glyph->xadv = (short)(scale * advance * 10.0f);
    glyph->xoff = (short)(x0 - pad);
    glyph->yoff = (short)(y0 - pad);
    glyph->next = 0;

    // Insert char to hash lookup.
    glyph->next = font->lut[h];
    font->lut[h] = (int)font->glyphs.size() - 1;

    // Rasterize
    dst = &texData[(glyph->x0 + pad) + (glyph->y0 + pad) * params.width];
    fons__tt_renderGlyphBitmap(&renderFont->font, dst, gw - pad * 2,
        gh - pad * 2, params.width, scale, scale, g);

    // Make sure there is one pixel empty border.
    dst = &texData[glyph->x0 + glyph->y0 * params.width];
    for(y = 0; y < gh; y++)
    {
        dst[y * params.width] = 0;
        dst[gw - 1 + y * params.width] = 0;
    }
    for(x = 0; x < gw; x++)
    {
        dst[x] = 0;
        dst[x + (gh - 1) * params.width] = 0;
    }

    // Debug code to color the glyph background
    /*	unsigned char* fdst = &texData[glyph->x0 + glyph->y0 * params.width];
    for (y = 0; y < gh; y++) {
    for (x = 0; x < gw; x++) {
    int a = (int)fdst[x+y*params.width] + 20;
    if (a > 255) a = 255;
    fdst[x+y*params.width] = a;
    }
    }*/

    // Blur
    if(iblur > 0)
    {
        nscratch = 0;
        bdst = &texData[glyph->x0 + glyph->y0 * params.width];
        fons__blur(bdst, gw, gh, params.width, iblur);
    }

    dirtyRect[0] = fons__mini(dirtyRect[0], glyph->x0);
    dirtyRect[1] = fons__mini(dirtyRect[1], glyph->y0);
    dirtyRect[2] = fons__maxi(dirtyRect[2], glyph->x1);
    dirtyRect[3] = fons__maxi(dirtyRect[3], glyph->y1);

    return glyph;
}

void FONScontext::fons__getQuad(
    FONSfont *font,
    int prevGlyphIndex,
    FONSglyph *glyph,
    float scale,
    float spacing,
    float *x,
    float *y,
    FONSquad *q)
{
    float rx, ry, xoff, yoff, x0, y0, x1, y1;

    if(prevGlyphIndex != -1)
    {
        float adv = fons__tt_getGlyphKernAdvance(&font->font, prevGlyphIndex,
            glyph->index) * scale;
        *x += (int)(adv + spacing + 0.5f);
    }

    // Each glyph has 2px border to allow good interpolation,
    // one pixel to prevent leaking, and one to allow good interpolation for rendering.
    // Inset the texture region by one pixel for correct interpolation.
    xoff = (short)(glyph->xoff + 1);
    yoff = (short)(glyph->yoff + 1);
    x0 = (float)(glyph->x0 + 1);
    y0 = (float)(glyph->y0 + 1);
    x1 = (float)(glyph->x1 - 1);
    y1 = (float)(glyph->y1 - 1);

    if(params.flags & FONS_ZERO_TOPLEFT)
    {
        rx = (float)(int)(*x + xoff);
        ry = (float)(int)(*y + yoff);

        q->x0 = rx;
        q->y0 = ry;
        q->x1 = rx + x1 - x0;
        q->y1 = ry + y1 - y0;

        q->s0 = x0 * itw;
        q->t0 = y0 * ith;
        q->s1 = x1 * itw;
        q->t1 = y1 * ith;
    }
    else
    {
        rx = (float)(int)(*x + xoff);
        ry = (float)(int)(*y - yoff);

        q->x0 = rx;
        q->y0 = ry;
        q->x1 = rx + x1 - x0;
        q->y1 = ry - y1 + y0;

        q->s0 = x0 * itw;
        q->t0 = y0 * ith;
        q->s1 = x1 * itw;
        q->t1 = y1 * ith;
    }

    *x += (int)(glyph->xadv / 10.0f + 0.5f);
}

void FONScontext::flush()
{
    // Flush texture
    if(dirtyRect[0] < dirtyRect[2] && dirtyRect[1] < dirtyRect[3])
    {
        if(params.renderUpdate != NULL)
            params.renderUpdate(params.userPtr, dirtyRect, texData.get());
        // Reset dirty rect
        dirtyRect[0] = params.width;
        dirtyRect[1] = params.height;
        dirtyRect[2] = 0;
        dirtyRect[3] = 0;
    }

    // Flush triangles
    if(!verts.empty())
    {
        if(params.renderDraw != NULL)
            params.renderDraw(params.userPtr, verts.data(), tcoords.data(),
                colors.data(), (int)verts.size());
        verts.clear();
        tcoords.clear();
        colors.clear();
    }
}

void FONScontext::vertex(
    float x,
    float y,
    float s,
    float t,
    unsigned int c)
{
    verts.push_back(x);
    verts.push_back(y);
    tcoords.push_back(s);
    tcoords.push_back(t);
    colors.push_back(c);
}

float FONScontext::getVerticalAlign(FONSfont *font, int align, short isize)
{
    if(params.flags & FONS_ZERO_TOPLEFT)
    {
        if(align & FONS_ALIGN_TOP)
        {
            return font->ascender * (float)isize / 10.0f;
        }
        else if(align & FONS_ALIGN_MIDDLE)
        {
            return (font->ascender + font->descender) / 2.0f * (float)isize /
                10.0f;
        }
        else if(align & FONS_ALIGN_BASELINE)
        {
            return 0.0f;
        }
        else if(align & FONS_ALIGN_BOTTOM)
        {
            return font->descender * (float)isize / 10.0f;
        }
    }
    else
    {
        if(align & FONS_ALIGN_TOP)
        {
            return -font->ascender * (float)isize / 10.0f;
        }
        else if(align & FONS_ALIGN_MIDDLE)
        {
            return -(font->ascender + font->descender) / 2.0f * (float)isize /
                10.0f;
        }
        else if(align & FONS_ALIGN_BASELINE)
        {
            return 0.0f;
        }
        else if(align & FONS_ALIGN_BOTTOM)
        {
            return -font->descender * (float)isize / 10.0f;
        }
    }
    return 0.0;
}

float FONScontext::drawText(
    float x,
    float y,
    std::string_view str)
{
    FONSstate *state = getState();
    unsigned int utf8state = 0;
    FONSglyph *glyph = NULL;
    FONSquad q;
    int prevGlyphIndex = -1;
    short isize = (short)(state->size * 10.0f);
    short iblur = (short)state->blur;
    float scale;
    FONSfont *font;
    float width;

    if(state->font < 0 || state->font >= fonts.size())
        throw std::runtime_error("invalid font index");

    font = &fonts[state->font];
    if(font->data.empty())
        throw std::runtime_error("invalid font data");

    scale = fons__tt_getPixelHeightScale(&font->font, (float)isize / 10.0f);

    // Align horizontally
    if(state->align & FONS_ALIGN_LEFT)
    {
        // empty
    }
    else if(state->align & FONS_ALIGN_RIGHT)
    {
        width = fonsTextBounds(x, y, str, NULL);
        x -= width;
    }
    else if(state->align & FONS_ALIGN_CENTER)
    {
        width = fonsTextBounds(x, y, str, NULL);
        x -= width * 0.5f;
    }
    // Align vertically.
    y += getVerticalAlign(font, state->align, isize);

    for(auto iter = str.begin(); iter != str.end();)
    {
        const auto codepoint = utf8::next(iter, str.end());
        glyph = getGlyph(font, codepoint, isize, iblur);

        if(glyph != NULL)
        {
            fons__getQuad(font, prevGlyphIndex, glyph, scale,
                state->spacing, &x, &y, &q);

            vertex(q.x0, q.y0, q.s0, q.t0, state->color);
            vertex(q.x1, q.y1, q.s1, q.t1, state->color);
            vertex(q.x1, q.y0, q.s1, q.t0, state->color);

            vertex(q.x0, q.y0, q.s0, q.t0, state->color);
            vertex(q.x0, q.y1, q.s0, q.t1, state->color);
            vertex(q.x1, q.y1, q.s1, q.t1, state->color);
        }
        prevGlyphIndex = glyph != NULL ? glyph->index : -1;
    }
    return x;
}

int FONScontext::fonsTextIterInit(
    FONStextIter *iter,
    float x,
    float y,
    const char *str,
    const char *end)
{
    FONSstate *state = getState();
    float width;

    memset(iter, 0, sizeof(*iter));

    if(state->font < 0 || state->font >= fonts.size())
        throw std::
            runtime_error("invalid font index");
    iter->font = &fonts[state->font];
    if(iter->font->data.empty()) return 0;

    iter->isize = (short)(state->size * 10.0f);
    iter->iblur = (short)state->blur;
    iter->scale = fons__tt_getPixelHeightScale(&iter->font->font,
        (float)iter->isize / 10.0f);

    // Align horizontally
    if(state->align & FONS_ALIGN_LEFT)
    {
        // empty
    }
    else if(state->align & FONS_ALIGN_RIGHT)
    {
        width = fonsTextBounds(x, y, str, NULL);
        x -= width;
    }
    else if(state->align & FONS_ALIGN_CENTER)
    {
        width = fonsTextBounds(x, y, str, NULL);
        x -= width * 0.5f;
    }
    // Align vertically.
    y += getVerticalAlign(iter->font, state->align, iter->isize);

    if(end == NULL)
        end = str + strlen(str);

    iter->x = iter->nextx = x;
    iter->y = iter->nexty = y;
    iter->spacing = state->spacing;
    iter->str = str;
    iter->next = str;
    iter->end = end;
    iter->codepoint = 0;
    iter->prevGlyphIndex = -1;

    return 1;
}

int FONScontext::fonsTextIterNext(FONStextIter *iter, FONSquad *quad)
{
    FONSglyph *glyph = NULL;
    const char *str = iter->next;
    iter->str = iter->next;

    if(str == iter->end)
        return 0;

    for(; str != iter->end; str++)
    {
        if(fons__decutf8(&iter->utf8state, &iter->codepoint,
            *(const unsigned char*)str))
            continue;
        str++;
        // Get glyph and quad
        iter->x = iter->nextx;
        iter->y = iter->nexty;
        glyph = getGlyph(iter->font, iter->codepoint, iter->isize,
            iter->iblur);
        if(glyph != NULL)
            fons__getQuad(iter->font, iter->prevGlyphIndex, glyph,
                iter->scale, iter->spacing, &iter->nextx, &iter->nexty, quad);
        iter->prevGlyphIndex = glyph != NULL ? glyph->index : -1;
        break;
    }
    iter->next = str;

    return 1;
}

void FONScontext::fonsDrawDebug(float x, float y)
{
    int i;
    int w = params.width;
    int h = params.height;
    float u = w == 0 ? 0 : (1.0f / w);
    float v = h == 0 ? 0 : (1.0f / h);

    // Draw background
    vertex(x + 0, y + 0, u, v, 0x0fffffff);
    vertex(x + w, y + h, u, v, 0x0fffffff);
    vertex(x + w, y + 0, u, v, 0x0fffffff);

    vertex(x + 0, y + 0, u, v, 0x0fffffff);
    vertex(x + 0, y + h, u, v, 0x0fffffff);
    vertex(x + w, y + h, u, v, 0x0fffffff);

    // Draw texture
    vertex(x + 0, y + 0, 0, 0, 0xffffffff);
    vertex(x + w, y + h, 1, 1, 0xffffffff);
    vertex(x + w, y + 0, 1, 0, 0xffffffff);

    vertex(x + 0, y + 0, 0, 0, 0xffffffff);
    vertex(x + 0, y + h, 0, 1, 0xffffffff);
    vertex(x + w, y + h, 1, 1, 0xffffffff);

    // Drawbug draw atlas
    for(i = 0; i < atlas.nodes.size(); i++)
    {
        FONSatlasNode *n = &atlas.nodes[i];

        vertex(x + n->x + 0, y + n->y + 0, u, v, 0xc00000ff);
        vertex(x + n->x + n->width, y + n->y + 1, u, v,
            0xc00000ff);
        vertex(x + n->x + n->width, y + n->y + 0, u, v,
            0xc00000ff);

        vertex(x + n->x + 0, y + n->y + 0, u, v, 0xc00000ff);
        vertex(x + n->x + 0, y + n->y + 1, u, v, 0xc00000ff);
        vertex(x + n->x + n->width, y + n->y + 1, u, v,
            0xc00000ff);
    }

    // fons__flush(stash);
}

float FONScontext::fonsTextBounds(
    float x,
    float y,
    std::string_view str,
    float *bounds)
{
    FONSstate *state = getState();
    FONSquad q;
    FONSglyph *glyph = NULL;
    int prevGlyphIndex = -1;
    short isize = (short)(state->size * 10.0f);
    short iblur = (short)state->blur;
    float scale;
    FONSfont *font;
    float startx, advance;
    float minx, miny, maxx, maxy;

    if(state->font < 0 || state->font >= fonts.size())
        throw std::runtime_error("invalid font index");
    font = &fonts[state->font];
    if(font->data.empty())
        throw std::runtime_error("invalid font data");

    scale = fons__tt_getPixelHeightScale(&font->font, (float)isize / 10.0f);

    // Align vertically.
    y += getVerticalAlign(font, state->align, isize);

    minx = maxx = x;
    miny = maxy = y;
    startx = x;

    for(auto iter = str.begin(); iter != str.end();)
    {
        const auto codepoint = utf8::next(iter, str.end());

        glyph = getGlyph(font, codepoint, isize, iblur);
        if(glyph != NULL)
        {
            fons__getQuad(font, prevGlyphIndex, glyph, scale,
                state->spacing, &x, &y, &q);
            if(q.x0 < minx) minx = q.x0;
            if(q.x1 > maxx) maxx = q.x1;
            if(params.flags & FONS_ZERO_TOPLEFT)
            {
                if(q.y0 < miny) miny = q.y0;
                if(q.y1 > maxy) maxy = q.y1;
            }
            else
            {
                if(q.y1 < miny) miny = q.y1;
                if(q.y0 > maxy) maxy = q.y0;
            }
        }
        prevGlyphIndex = glyph != NULL ? glyph->index : -1;
    }

    advance = x - startx;

    // Align horizontally
    if(state->align & FONS_ALIGN_LEFT)
    {
        // empty
    }
    else if(state->align & FONS_ALIGN_RIGHT)
    {
        minx -= advance;
        maxx -= advance;
    }
    else if(state->align & FONS_ALIGN_CENTER)
    {
        minx -= advance * 0.5f;
        maxx -= advance * 0.5f;
    }

    if(bounds)
    {
        bounds[0] = minx;
        bounds[1] = miny;
        bounds[2] = maxx;
        bounds[3] = maxy;
    }

    return advance;
}

void FONScontext::fonsVertMetrics(
    float *ascender,
    float *descender,
    float *lineh)
{
    FONSfont *font;
    FONSstate *state = getState();
    short isize;

    if(state->font < 0 || state->font >= fonts.size())
        throw std::
            runtime_error("invalid font index");
    font = &fonts[state->font];
    isize = (short)(state->size * 10.0f);
    if(font->data.empty()) return;

    if(ascender)
        *ascender = font->ascender * isize / 10.0f;
    if(descender)
        *descender = font->descender * isize / 10.0f;
    if(lineh)
        *lineh = font->lineh * isize / 10.0f;
}

void FONScontext::fonsLineBounds(float y, float *miny, float *maxy)
{
    FONSfont *font;
    FONSstate *state = getState();
    short isize;
    if(state->font < 0 || state->font >= fonts.size())
        throw std::
            runtime_error("invalid font index");
    font = &fonts[state->font];
    isize = (short)(state->size * 10.0f);
    if(font->data.empty()) return;

    y += getVerticalAlign(font, state->align, isize);

    if(params.flags & FONS_ZERO_TOPLEFT)
    {
        *miny = y - font->ascender * (float)isize / 10.0f;
        *maxy = *miny + font->lineh * isize / 10.0f;
    }
    else
    {
        *maxy = y + font->descender * (float)isize / 10.0f;
        *miny = *maxy - font->lineh * isize / 10.0f;
    }
}

const unsigned char * FONScontext::fonsGetTextureData(int *width, int *height)
{
    if(width != NULL)
        *width = params.width;
    if(height != NULL)
        *height = params.height;
    return texData.get();
}

int FONScontext::fonsValidateTexture(int *dirty)
{
    if(dirtyRect[0] < dirtyRect[2] && dirtyRect[1] < dirtyRect[3])
    {
        dirty[0] = dirtyRect[0];
        dirty[1] = dirtyRect[1];
        dirty[2] = dirtyRect[2];
        dirty[3] = dirtyRect[3];
        // Reset dirty rect
        dirtyRect[0] = params.width;
        dirtyRect[1] = params.height;
        dirtyRect[2] = 0;
        dirtyRect[3] = 0;
        return 1;
    }
    return 0;
}

FONScontext::~FONScontext()
{
    if(params.renderDelete)
        params.renderDelete(params.userPtr);
}

void FONScontext::fonsSetErrorCallback(
    void (*callback)(void *uptr, int error, int val),
    void *uptr)
{
    handleError = callback;
    errorUptr = uptr;
}

void FONScontext::fonsGetAtlasSize(int *width, int *height)
{
    *width = params.width;
    *height = params.height;
}

int FONScontext::fonsExpandAtlas(int width, int height)
{
    int i, maxy = 0;
    std::unique_ptr<unsigned char[]> newdata;

    width = fons__maxi(width, params.width);
    height = fons__maxi(height, params.height);

    if(width == params.width && height == params.height)
        return 1;

    // Flush pending glyphs.
    flush();

    // Create new texture
    if(params.renderResize != NULL)
    {
        if(params.renderResize(params.userPtr, width, height) == 0)
            return 0;
    }
    // Copy old texture data over.
    newdata.reset(new unsigned char[width * height]);
    for(i = 0; i < params.height; i++)
    {
        unsigned char *dst = &newdata[i * width];
        unsigned char *src = &texData[i * params.width];
        memcpy(dst, src, params.width);
        if(width > params.width)
            memset(dst + params.width, 0, width - params.width);
    }
    if(height > params.height)
        memset(&newdata[params.height * width], 0,
            (height - params.height) * width);

    texData.reset(newdata.release());

    // Increase atlas size
    atlas.fons__atlasExpand(width, height);

    // Add existing data as dirty.
    for(i = 0; i < atlas.nodes.size(); i++)
        maxy = fons__maxi(maxy, atlas.nodes[i].y);
    dirtyRect[0] = 0;
    dirtyRect[1] = 0;
    dirtyRect[2] = params.width;
    dirtyRect[3] = maxy;

    params.width = width;
    params.height = height;
    itw = 1.0f / params.width;
    ith = 1.0f / params.height;

    return 1;
}

int FONScontext::fonsResetAtlas(int width, int height)
{
    int i, j;

    // Flush pending glyphs.
    flush();

    // Create new texture
    if(params.renderResize != NULL)
    {
        if(params.renderResize(params.userPtr, width, height) == 0)
            return 0;
    }

    // Reset atlas
    atlas.fons__atlasReset(width, height);

    // Clear texture data.
    texData.reset(new unsigned char[width * height]);

    // Reset dirty rect
    dirtyRect[0] = width;
    dirtyRect[1] = height;
    dirtyRect[2] = 0;
    dirtyRect[3] = 0;

    // Reset cached glyphs
    for(i = 0; i < fonts.size(); i++)
    {
        FONSfont *font = &fonts[i];
        font->glyphs.clear();
        for(j = 0; j < FONS_HASH_LUT_SIZE; j++)
            font->lut[j] = -1;
    }

    params.width = width;
    params.height = height;
    itw = 1.0f / params.width;
    ith = 1.0f / params.height;

    // Add white rect at 0,0 for debug drawing.
    fons__addWhiteRect(2, 2);

    return 1;
}
