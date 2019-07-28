#pragma once

#include <Usagi/Core/Component.hpp>
#include <Usagi/Math/Bound.hpp>

namespace usagi
{
struct Bound2DComponent : Component
{
    AlignedBox2f bound;

    const std::type_info & baseType() override
    {
        return typeid(Bound2DComponent);
    }
};
}
