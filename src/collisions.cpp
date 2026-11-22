#include "collisions.h"

#include <algorithm>
#include <glm/vec4.hpp>

AABB AABB_Transform(glm::vec3 bbox_min, glm::vec3 bbox_max, const glm::mat4& model)
{
    AABB result;
    result.min = glm::vec3( 1e30f);
    result.max = glm::vec3(-1e30f);

    // Percorremos os 8 cantos da caixa: cada bit de 'i' escolhe min ou max.
    for (int i = 0; i < 8; ++i)
    {
        glm::vec4 corner(
            (i & 1) ? bbox_max.x : bbox_min.x,
            (i & 2) ? bbox_max.y : bbox_min.y,
            (i & 4) ? bbox_max.z : bbox_min.z,
            1.0f);

        glm::vec4 w = model * corner;

        result.min = glm::vec3(std::min(result.min.x, w.x),
                               std::min(result.min.y, w.y),
                               std::min(result.min.z, w.z));
        result.max = glm::vec3(std::max(result.max.x, w.x),
                               std::max(result.max.y, w.y),
                               std::max(result.max.z, w.z));
    }
    return result;
}

bool AABB_Intersects(const AABB& a, const AABB& b)
{
    return (a.min.x <= b.max.x && a.max.x >= b.min.x) &&
           (a.min.y <= b.max.y && a.max.y >= b.min.y) &&
           (a.min.z <= b.max.z && a.max.z >= b.min.z);
}
