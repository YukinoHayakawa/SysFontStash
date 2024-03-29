﻿#pragma once

#include <Usagi/Core/Component.hpp>

#include "FontStash.hpp"

namespace usagi
{
struct FontStashComponent : Component
{
    int font = 0;
    int align = FONS_ALIGN_LEFT | FONS_ALIGN_BASELINE;
    float size = 48;
    unsigned int color = 0xFFFFFFFF;
    float blur = 0;
    float spacing = 0;
    float line_spacing = 5;

    float transition_begin = 0;
    float transition_end = 0;

    std::u32string uft32_text;

    const std::type_info & baseType() override
    {
        return typeid(FontStashComponent);
    }
};
}
