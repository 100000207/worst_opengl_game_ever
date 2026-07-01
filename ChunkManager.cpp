#include "ChunkManager.h"


std::vector<glm::vec3> chunk_manager::update_chunks() {
	return std::vector<glm::vec3> {glm::vec3(0, 0, 0)};
};



void chunk_manager::create_quadtree(glm::vec3 chunk_pos, glm::vec3 pos, int depth) {

	int chunk_size = parent_size / pow(2, depth);

	glm::vec3 center = glm::vec3(chunk_pos.x + chunk_size/2, chunk_pos.y + chunk_size/2,0);

	float distance_from_center = glm::length(center - chunk_pos_quantised);
	float distance_threshold = distance_factor * (float)(parent_size / pow(2, depth));
	if (depth >= max_depth) {
		world_chunk_pseudo_hashes.push_back(chunk_pos + glm::vec3(0, 0 , max_depth - depth));
	}
	else if (distance_from_center > distance_threshold) {
		world_chunk_pseudo_hashes.push_back(chunk_pos + glm::vec3(0, 0, max_depth - depth));
	}
	else {
		chunk_manager::create_quadtree(chunk_pos, pos, depth + 1);
		chunk_manager::create_quadtree(chunk_pos + glm::vec3(chunk_size / 2, 0, 0), pos, depth + 1);
		chunk_manager::create_quadtree(chunk_pos + glm::vec3(0, chunk_size / 2, 0), pos, depth + 1);
		chunk_manager::create_quadtree(chunk_pos + glm::vec3(chunk_size / 2, chunk_size / 2, 0), pos, depth + 1);
	}

}
