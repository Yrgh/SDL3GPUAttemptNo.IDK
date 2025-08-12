#pragma once

#include <iostream>
#include <fstream>
#include <string>
#include <vector>
#include <cmath>
#include <cstdint>
#include <cstdlib>

#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/matrix_access.hpp>
#include <glm/gtc/quaternion.hpp>

using glm::vec4, glm::vec3, glm::vec2, glm::mat4x4, glm::quat;

#define IGNORE(something)
#define NO_IMPL(result) { std::cout << "No implementation\n";  return result; }

#define FATALIZE_SDL(expr, also) if (!expr) { std::cout << "SDL Error: " << SDL_GetError() << "\n"; also }

#define BIT_CAST(expr, to) (*(to *)(expr))

using byte = uint8_t;

/*
 * Allocates a buffer with new and fills it with the contents of the file.
 * o_size will be set to the file size
 * If there are any errors, returns nullptr and sets o_size to -1
*/
byte *read_whole_file(std::string filename, size_t &o_size);