#ifndef CHUNK_GEN_CLASS_H
#define CHUNK_GEN_CLASS_H

#include<glad/glad.h>
#include<GLFW/glfw3.h>
#include<glm/glm.hpp>
#include<glm/gtc/matrix_transform.hpp>
#include<glm/gtc/type_ptr.hpp>
#include<glm/gtx/rotate_vector.hpp>
#include<glm/gtx/vector_angle.hpp>
#include<vector>

#include"VAO.h"
#include"VBO.h"
#include"EBO.h"



class chunk_gen
{
	public:
		int chunk_size = 64;

		std::vector<float> vertices;
		std::vector<int> indices;

		VAO* vao = nullptr;
		VBO* vbo = nullptr;
		EBO* ebo = nullptr;


		glm::vec3 gen_tri_face_normals(glm::vec3 x, glm::vec3 y, glm::vec3 z);
		float terrain_function(float x, float y);
		std::vector<float> gen_x_values(int x, int y, int step);
		std::vector<float> gen_y_values(int x, int y, int step);
		std::vector<float> gen_z_values(int x, int y, int step);
		std::vector<glm::vec3> gen_vertex_position(std::vector<float> x, std::vector<float> y, std::vector<float> z);
		glm::vec3 compute_terrain_normal(float x, float y);
		void generate_chunk_vectors(int x, int y, int lod_level);
		std::vector<int> generate_chunk_EBO(int x, int y, int lod_level);
		void load_to_gpu();
		void terminate();



};

















#endif