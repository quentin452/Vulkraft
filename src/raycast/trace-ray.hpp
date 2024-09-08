#pragma once

#include <functional>
#include <glm/glm.hpp>

using TraceRayCallback = std::function<bool(glm::ivec3)>;

class TraceRay {
public:
  static bool trace(TraceRayCallback callback, glm::vec3 origin,
                    glm::vec3 direction, int limit, glm::ivec3 &hitPos,
                    glm::vec3 &hitNorm);
};