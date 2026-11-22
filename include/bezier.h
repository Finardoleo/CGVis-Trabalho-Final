#ifndef BEZIER_H
#define BEZIER_H

#include <vector>
#include <glm/vec3.hpp>

// Curva de Bézier cúbica definida por 4 pontos de controle:
//   B(t) = (1-t)^3 P0 + 3(1-t)^2 t P1 + 3(1-t) t^2 P2 + t^3 P3,  t em [0,1].
// A curva começa em P0, termina em P3, e P1/P2 "puxam" a curva, definindo as
// tangentes nas pontas: B'(0) = 3(P1-P0) e B'(1) = 3(P3-P2).
struct CubicBezier
{
    glm::vec3 p0, p1, p2, p3;

    glm::vec3 Point(float t) const
    {
        const float s = 1.0f - t;
        return (s*s*s) * p0 + (3.0f*s*s*t) * p1 + (3.0f*s*t*t) * p2 + (t*t*t) * p3;
    }

    // Derivada B'(t): a direção (e a "velocidade") da curva em t.
    glm::vec3 Tangent(float t) const
    {
        const float s = 1.0f - t;
        return (3.0f*s*s) * (p1 - p0) + (6.0f*s*t) * (p2 - p1) + (3.0f*t*t) * (p3 - p2);
    }
};

// Monta uma sequência de curvas de Bézier cúbicas que passa por todos os
// pontos 'w' (os pontos de passagem da ronda), de forma suave (com tangente
// contínua). Para cada trecho w[i] -> w[i+1], os pontos de controle internos
// são escolhidos a partir dos vizinhos (construção de Catmull-Rom):
//   P1 = w[i]   + (w[i+1] - w[i-1]) / 6
//   P2 = w[i+1] - (w[i+2] - w[i])   / 6
// Se 'closed', a curva é fechada (o último ponto liga no primeiro); senão, os
// extremos são repetidos.
inline std::vector<CubicBezier> BuildBezierPath(const std::vector<glm::vec3>& w, bool closed)
{
    std::vector<CubicBezier> path;
    const int n = (int)w.size();
    const int segments = closed ? n : n - 1;
    auto at = [&](int i) -> glm::vec3 {
        if (closed) return w[((i % n) + n) % n];
        if (i < 0) return w[0];
        if (i >= n) return w[n - 1];
        return w[i];
    };
    for (int i = 0; i < segments; ++i)
    {
        CubicBezier b;
        b.p0 = at(i);
        b.p3 = at(i + 1);
        b.p1 = b.p0 + (at(i + 1) - at(i - 1)) / 6.0f;
        b.p2 = b.p3 - (at(i + 2) - at(i)) / 6.0f;
        path.push_back(b);
    }
    return path;
}

#endif // BEZIER_H
