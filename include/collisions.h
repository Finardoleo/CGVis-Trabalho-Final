#ifndef COLLISIONS_H
#define COLLISIONS_H

#include <glm/vec3.hpp>
#include <glm/mat4x4.hpp>

// Axis-Aligned Bounding Box (AABB) em coordenadas do mundo.
struct AABB
{
    glm::vec3 min;
    glm::vec3 max;
};

// Transforma a AABB de um modelo (bbox_min/bbox_max, em coordenadas do modelo)
// pela matriz Model e devolve a AABB resultante em coordenadas do mundo.
// Os 8 cantos da caixa são transformados e depois se toma o mínimo/máximo
// em cada eixo, o que funciona para qualquer rotação (nas paredes, 90 graus).
AABB AABB_Transform(glm::vec3 bbox_min, glm::vec3 bbox_max, const glm::mat4& model);

// Teste de intersecção AABB x AABB (sobreposição nos 3 eixos).
bool AABB_Intersects(const AABB& a, const AABB& b);

#endif // COLLISIONS_H
