#pragma once

#include <Usagi/Core/Component.hpp>
#include <Usagi/Core/Math.hpp>

namespace usagi
{
struct Position2DComponent : Component
{
    Vector2f pos = Vector2f::Zero();

    const std::type_info & baseType() override
    {
        return typeid(Position2DComponent);
    }
};
}
