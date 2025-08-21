#pragma once

#include <iostream>
#include <fstream>
#include <string>
#include <vector>
#include <unordered_map>
#include <map>
#include <unordered_set>
#include <random>
#include <cmath>
#include <cstdint>
#include <cstdlib>

#include <SDL3/SDL.h>

#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/matrix_access.hpp>
#include <glm/gtc/quaternion.hpp>

#define U32_BAD (u32(-1))

using glm::vec4, glm::vec3, glm::vec2, glm::mat4x4, glm::quat;

template<class T> using Set = std::unordered_set<T>;
template<class K, class V> using Map = std::unordered_map<K, V>;
template<class K, class V> using OrderedMap = std::map<K, V>;

inline auto deg_to_rad = glm::radians<float>;

#define IGNORE(something)
#define NO_IMPL(result) { std::cout << "No implementation\n";  return result; }

#define FATALIZE_SDL(expr, also) if (!expr) { std::cout << "SDL Error: " << SDL_GetError() << "\n"; also }

#define BIT_CAST(expr, to) (*(to *)(expr))

#define LENGTHOF(array) (sizeof(array) / sizeof(array[0]))

using byte = uint8_t;
typedef uint32_t u32;

/*
 * Allocates a buffer with new and fills it with the contents of the file.
 * o_size will be set to the file size
 * If there are any errors, returns nullptr and sets o_size to -1
*/
byte *read_whole_file(std::string filename, u32 *o_size);

class RID {
	u32 m_number;

	inline static std::independent_bits_engine<std::default_random_engine, 32, u32> s_engine;
public:
	inline RID() {
		m_number = s_engine();
	}

	inline RID(const u32 &o): m_number(o) {}

	u32 operator*() {
		return m_number;
	}

	operator u32() = delete;

	operator bool() {
		return m_number > 0;
	}
};

inline size_t total_allocs = 0;

inline void *operator new(size_t size) {
	//std::cout << "Allocated " << size << " bytes (" << total_allocs << ")\n";
	total_allocs++;
	return malloc(size);
}

inline void operator delete(void *mem) {
	total_allocs--;
	//std::cout << "Freed allocation (" << total_allocs << ")\n";
	free(mem);
}