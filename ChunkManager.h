#ifndef CHUNK_MANAGER_CLASS_H
#define CHUNK_MANAGER_CLASS_H

#include "glm/glm.hpp"
#include <vector>

class chunk_manager
{
public:
	std::vector<glm::vec3> world_chunk_pseudo_hashes;
	glm::vec3 chunk_pos_quantised;
	int min_chunk_size = 64;
	int parent_size = pow(2, 10) * 64;
	float distance_factor = 2.0f;
	int max_depth = 10;

	std::vector<glm::vec3> update_chunks();
	void create_quadtree(glm::vec3 chunk_pos, glm::vec3 pos, int depth);









};









#endif
