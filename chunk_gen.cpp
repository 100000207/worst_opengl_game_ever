#include "chunk_gen.h"
#include <iostream>
#include <cmath>
#include <glm/glm.hpp>

glm::vec3 chunk_gen::gen_tri_face_normals(glm::vec3 x, glm::vec3 y, glm::vec3 z) {
	glm::vec3 vector1 = x - y;
	glm::vec3 vector2 = x - z;
	glm::vec3 normal = glm::cross(vector1, vector2);
	normal = glm::normalize(normal);
	return normal * (glm::vec3)glm::sign(glm::sign(normal.z) + 0.5);
};


//I vibecoded the eroded noise into c++ but I think Im allowed to do that as I did also implement it into desmos by hand only for it to not work due to desmos being terrible


// ── Eroded terrain (port of Clay John's Shadertoy, MIT licence) ─────────────
// Original: https://www.shadertoy.com/view/... (Buffer A pass)
// CPU port replaces the fract-based GPU hash with an integer hash
// to avoid precision loss at large world coordinates.

// ── helpers ─────────────────────────────────────────────────────────────────

static inline glm::vec2 glsl_fract(glm::vec2 v) {
	return v - glm::floor(v);
}

// Integer hash — robust at any world coordinate.
// Maps (ix, iy) → a vec2 in [-1, 1]²
static glm::vec2 hash2i(int ix, int iy)
{
	// Murmur-inspired integer mix
	unsigned int x = static_cast<unsigned int>(ix) * 1664525u + 1013904223u;
	unsigned int y = static_cast<unsigned int>(iy) * 1664525u + 1013904223u;
	x ^= y * 2246822519u;
	y ^= x * 3266489917u;
	x += y * 1664525u;
	y += x * 1664525u;
	x ^= x >> 16;
	y ^= y >> 16;
	// Map to [-1, 1]
	constexpr float INV = 1.f / 2147483648.f;
	return glm::vec2(
		static_cast<float>(static_cast<int>(x)) * INV,
		static_cast<float>(static_cast<int>(y)) * INV
	);
}

// ── noised ───────────────────────────────────────────────────────────────────
// Port of iq's "Noise - Gradient - 2D - Deriv"
// Returns: .x = noise value, .yz = analytic derivatives
static glm::vec3 noised(glm::vec2 p)
{
	glm::vec2 i = glm::floor(p);
	glm::vec2 f = glm::fract(p);   // same as p - floor(p)

	// Quintic interpolant + derivative
	glm::vec2 u = f * f * f * (f * (f * 6.f - 15.f) + 10.f);
	glm::vec2 du = 30.f * f * f * (f * (f - 2.f) + 1.f);

	int ix = static_cast<int>(i.x), iy = static_cast<int>(i.y);
	glm::vec2 ga = hash2i(ix, iy);
	glm::vec2 gb = hash2i(ix + 1, iy);
	glm::vec2 gc = hash2i(ix, iy + 1);
	glm::vec2 gd = hash2i(ix + 1, iy + 1);

	float va = glm::dot(ga, f - glm::vec2(0.f, 0.f));
	float vb = glm::dot(gb, f - glm::vec2(1.f, 0.f));
	float vc = glm::dot(gc, f - glm::vec2(0.f, 1.f));
	float vd = glm::dot(gd, f - glm::vec2(1.f, 1.f));

	float value = va + u.x * (vb - va) + u.y * (vc - va) + u.x * u.y * (va - vb - vc + vd);

	// Analytic derivative
	glm::vec2 deriv =
		ga
		+ u.x * (gb - ga)
		+ u.y * (gc - ga)
		+ u.x * u.y * (ga - gb - gc + gd)
		+ du * (glm::vec2(u.y, u.x) * (va - vb - vc + vd)
			+ glm::vec2(vb - va, vc - va));

	return glm::vec3(value, deriv.x, deriv.y);
}

// ── erosion ──────────────────────────────────────────────────────────────────
// Port of guil's Gavoronoise, modified by Clay John to return analytic derivs.
static glm::vec3 erosion(glm::vec2 p, glm::vec2 dir)
{
	constexpr float TWO_PI = 6.28318530718f;

	glm::vec2 ip = glm::floor(p);
	glm::vec2 fp = glm::fract(p);

	glm::vec3 va(0.f);
	float wt = 0.f;

	for (int i = -2; i <= 1; ++i) {
		for (int j = -2; j <= 1; ++j) {
			glm::vec2 o(static_cast<float>(i), static_cast<float>(j));
			int hx = static_cast<int>(ip.x) - i;
			int hy = static_cast<int>(ip.y) - j;
			glm::vec2 h = hash2i(hx, hy) * 0.5f;

			glm::vec2 pp = fp + o - h;
			float d = glm::dot(pp, pp);
			float w = std::exp(-d * 2.f);
			wt += w;

			float mag = glm::dot(pp, dir);
			float c = std::cos(mag * TWO_PI);
			float s = std::sin(mag * TWO_PI);

			// .x = value, .yz = derivatives  (matches GLSL vec3(cos, -sin*(pp+dir))*w)
			va += glm::vec3(c, -s * (pp.x + dir.x), -s * (pp.y + dir.y)) * w;
		}
	}
	return va / wt;
}

// ── mountain ─────────────────────────────────────────────────────────────────
// Returns: .x = height [0,1], .yz = surface normal XY derivatives
// s = overall scale factor (0.1 in the original)
static glm::vec3 mountain(glm::vec2 p, float s)
{
	// --- base FBM heightmap with analytic normals ---
	glm::vec3 n(0.f);
	float nf = 1.f, na = 0.6f;
	for (int i = 0; i < 2; ++i) {
		glm::vec3 nd = noised(p * s * nf);
		n += nd * na * glm::vec3(1.f, nf, nf);
		na *= 0.5f;
		nf *= 2.f;
	}

	// curl of the normal → slope-down direction
	glm::vec2 dir = glm::vec2(n.y, -n.z); // n.zy * vec2(1,-1)

	// --- erosion FBM ---
	glm::vec3 h(0.f);
	// smooth out valleys so erosion only appears on slopes
	float t = n.x * 0.5f + 0.5f;
	float a = 0.7f * glm::smoothstep(0.3f, 0.5f, t);
	float f = 1.f;
	for (int i = 0; i < 5; ++i) {
		glm::vec3 e = erosion(p * f, dir + glm::vec2(h.y, -h.z));
		h += e * a * glm::vec3(1.f, f, f);
		a *= 0.4f;
		f *= 2.f;
	}

	// remap height and blend erosion in lightly
	float height = glm::smoothstep(-1.f, 1.f, n.x) + h.x * 0.05f;
	glm::vec2 derivs = (glm::vec2(n.y, n.z) + glm::vec2(h.y, h.z)) * 0.5f + 0.5f;
	return glm::vec3(height, derivs.x, derivs.y);
}

// ── 6.  Terrain function ─────────────────────────────────────────────────
float chunk_gen::terrain_function(float x, float y)
{
	// Match the original: uv*4.0, s=0.1, then the mountain call uses p*s
	// Tweak WORLD_SCALE to taste — controls how zoomed in the features are
	constexpr float WORLD_SCALE = 4.f * 0.1f * 0.015f; // keeps similar frequency to old fbm
	constexpr float HEIGHT_SCALE = 128.f;
	glm::vec3 m = mountain(glm::vec2(x, y) * WORLD_SCALE, 1.f);
	return m.x * HEIGHT_SCALE;
}

// ── Grid generation ──────────────────────────────────────────────────────
// step scales the WORLD-SPACE spacing between vertices, not the vertex
// count. Every chunk always has (chunk_size+1)^2 vertices; a LOD level
// with a larger step simply covers more world-space area with the same
// vertex/triangle budget.

std::vector<float> chunk_gen::gen_x_values(int x, int y, int step) {
	std::vector<float> output_x_values;
	output_x_values.reserve((chunk_size + 1) * (chunk_size + 1));
	for (int i = 0; i <= chunk_size; i++) {
		for (int j = 0; j <= chunk_size; j++) {
			output_x_values.push_back(x + (float)(j * step));
		}
	}
	return output_x_values;
}

std::vector<float> chunk_gen::gen_y_values(int x, int y, int step) {
	std::vector<float> output_y_values;
	output_y_values.reserve((chunk_size + 1) * (chunk_size + 1));
	for (int i = 0; i <= chunk_size; i++) {
		for (int j = 0; j <= chunk_size; j++) {
			output_y_values.push_back(y + (float)(i * step));
		}
	}
	return output_y_values;
}

std::vector<float> chunk_gen::gen_z_values(int x, int y, int step) {
	std::vector<float> output_vector;
	output_vector.reserve((chunk_size + 1) * (chunk_size + 1));
	for (int i = 0; i <= chunk_size; i++) {
		for (int j = 0; j <= chunk_size; j++) {
			output_vector.push_back(terrain_function((float)(x + j * step), (float)(y + i * step)));
		}
	}
	return output_vector;
}

std::vector<glm::vec3> chunk_gen::gen_vertex_position(std::vector<float> x, std::vector<float> y, std::vector<float> z) {
	std::vector<glm::vec3>output_vectors = std::vector<glm::vec3>(std::min({ x.size(),y.size(),z.size() }));
	for (int i = 0; i < std::min({ x.size(),y.size(),z.size() }); i++) {
		output_vectors[i] = glm::vec3(x[i], y[i], z[i]);
	}
	return output_vectors;
}

glm::vec3 chunk_gen::compute_terrain_normal(float x, float y)
{
	constexpr float H = 0.5f; // sample offset — smaller = more accurate, larger = smoother
	float dzdx = (terrain_function(x + H, y) - terrain_function(x - H, y)) / (2.f * H);
	float dzdy = (terrain_function(x, y + H) - terrain_function(x, y - H)) / (2.f * H);
	// your layout is (x, z, y) in world space, so cross accordingly
	return glm::normalize(glm::vec3(-dzdx, -dzdy, 1.f));
}


void chunk_gen::generate_chunk_vectors(int x, int y, int lod_level) {
	//3 positions, 3 normals, 2 texture coordinates;
	// we have pos dont have normals tex is easy to calculate
	std::cout << "starting chunk gen" << std::endl;
	int step = 1 << lod_level;
	std::vector<float> x_values = gen_x_values(x, y, step);
	std::vector<float> y_values = gen_y_values(x, y, step);
	std::vector<float> z_values = gen_z_values(x, y, step);
	std::vector<glm::vec3> vertex_positions = gen_vertex_position(x_values, y_values, z_values);
	std::vector<int> triangle_faces = generate_chunk_EBO(x, y, lod_level);
	std::vector<glm::vec3> vector_normals((chunk_size + 1) * (chunk_size + 1));
	for (int i = 0; i < x_values.size(); i++) {
		vector_normals[i] = compute_terrain_normal((float)x_values[i], (float)y_values[i]);
	}

	std::vector<float> final_vector = std::vector<float>();
	for (int i = 0; i < x_values.size(); i++) {
		final_vector.push_back(x_values[i]);
		final_vector.push_back(z_values[i]);
		final_vector.push_back(y_values[i]);
		final_vector.push_back(vector_normals[i].x);
		final_vector.push_back(vector_normals[i].z);
		final_vector.push_back(vector_normals[i].y);
		final_vector.push_back(x_values[i]);
		final_vector.push_back(y_values[i]);
	}
	vertices = final_vector;
	indices = triangle_faces;
	std::cout << "lod level: " << lod_level << ", step: " << step << std::endl;
	//return final_vector;

};

std::vector<int> chunk_gen::generate_chunk_EBO(int x, int y, int lod_level) {
	// Vertex count is now fixed at (chunk_size+1)^2 regardless of LOD,
	// so the index topology no longer depends on step/lod_level at all.
	std::vector<int> indices;
	indices.reserve(chunk_size * chunk_size * 6);
	//construct left tris
	for (int i = 0; i < chunk_size; i++) {
		for (int j = 0; j < chunk_size; j++) {
			indices.push_back((chunk_size + 1) * i + j);
			indices.push_back((chunk_size + 1) * i + j + 1);
			indices.push_back((chunk_size + 1) * (i + 1) + j);
		}
	}
	//construct right tris
	for (int i = 0; i < chunk_size; i++) {
		for (int j = 0; j < chunk_size; j++) {
			indices.push_back((chunk_size + 1) * (i + 1) + j + 1);
			indices.push_back((chunk_size + 1) * i + j + 1);
			indices.push_back((chunk_size + 1) * (i + 1) + j);
		}
	}
	return indices;
};

void chunk_gen::load_to_gpu() {
	vao = new VAO();
	vao->Bind();    // bind FIRST

	vbo = new VBO(vertices.data(), vertices.size() * sizeof(GLfloat));
	ebo = new EBO(indices.data(), indices.size() * sizeof(GLuint));  // now inside VAO

	vao->LinkAttrib(*vbo, 0, 3, GL_FLOAT, 8 * sizeof(float), (void*)0);
	vao->LinkAttrib(*vbo, 1, 3, GL_FLOAT, 8 * sizeof(float), (void*)(3 * sizeof(float)));
	vao->LinkAttrib(*vbo, 2, 2, GL_FLOAT, 8 * sizeof(float), (void*)(6 * sizeof(float)));

	vao->Unbind();
	vbo->Unbind();
	ebo->Unbind();  // unbind EBO after VAO
}

void chunk_gen::terminate() {
	vao->Delete();
	vbo->Delete();
	ebo->Delete();
	delete vao; vao = nullptr;
	delete vbo; vbo = nullptr;
	delete ebo; ebo = nullptr;
}
