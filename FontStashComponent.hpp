#pragma once

#include <Usagi/Core/Component.hpp>

#include "FontStash.hpp"

namespace usagi
{
struct FontStashComponent : Component
{
    int font = 0;
    int align = FONS_ALIGN_LEFT;
    float size = 32;
    unsigned int color = 0xFFFFFFFF;
    float blur = 0;
    float spacing = 0;
    std::string text;

    const std::type_info & baseType() override
    {
        return typeid(FontStashComponent);
    }
};
}
