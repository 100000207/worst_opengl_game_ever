//------- Ignore this ----------
#include<filesystem>
namespace fs = std::filesystem;
//------------------------------

#include<iostream>
#include<glad/glad.h>
#include<GLFW/glfw3.h>
#include<stb/stb_image.h>
#include<glm/glm.hpp>
#include<glm/gtc/matrix_transform.hpp>
#include<glm/gtc/type_ptr.hpp>
//#include<vector>

#include"Texture.h"
#include"shaderClass.h"
#include"VAO.h"
#include"VBO.h"
#include"EBO.h"
#include"Camera.h"
#include"chunk_gen.h"
#include"ChunkManager.h"
//#include"utils.h"



const unsigned int width = 800;
const unsigned int height = 800;



// Vertices coordinates
// position, normal, texture 
void framebuffer_size_callback(GLFWwindow* window, int width, int height)
{
	glViewport(0, 0, width, height);
}


int main()
{
	chunk_manager world;
	world.create_quadtree(glm::vec3(0, 0, 0), glm::vec3(0, 100, 0), 0);
	std::cout << "number of chunks: " << world.world_chunk_pseudo_hashes.size() << std::endl;
	std::vector<glm::vec3> chunks = world.world_chunk_pseudo_hashes;

	// Initialize GLFW
	glfwInit();

	// Tell GLFW what version of OpenGL we are using 
	// In this case we are using OpenGL 3.3
	glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
	glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
	// Tell GLFW we are using the CORE profile
	// So that means we only have the modern functions
	glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);

	// Create a GLFWwindow object of 800 by 800 pixels, naming it "YoutubeOpenGL"
	GLFWwindow* window = glfwCreateWindow(width, height, "YoutubeOpenGL", NULL, NULL);
	// Error check if the window fails to create
	if (window == NULL)
	{
		std::cout << "Failed to create GLFW window" << std::endl;
		glfwTerminate();
		return -1;
	}
	// Introduce the window into the current context
	glfwMakeContextCurrent(window);
	glfwSetFramebufferSizeCallback(window, framebuffer_size_callback);

	//Load GLAD so it configures OpenGL
	gladLoadGL();
	// Specify the viewport of OpenGL in the Window
	// In this case the viewport goes from x = 0, y = 0, to x = 800, y = 800
	glViewport(0, 0, width, height);

	
	// Generates Shader object using shaders default.vert and default.frag
	Shader triangle_rasterization("default.vert", "default.frag");
	//Shader shaderP("render_buffer.vert", "render_buffer.frag");


	// Generates Vertex Array Object and binds it

	



	/*
	* I'm doing this relative path thing in order to centralize all the resources into one folder and not
	* duplicate them between tutorial folders. You can just copy paste the resources from the 'Resources'
	* folder and then give a relative path from this folder to whatever resource you want to get to.
	* Also note that this requires C++17, so go to Project Properties, C/C++, Language, and select C++17
	*/
	std::string parentDir = (fs::current_path().fs::path::parent_path()).string();
	std::string texPath = "/Resources/YoutubeOpenGL 0 - Install/";

	// Texture
	Texture brickTex("C:/Users/Hecti/Downloads/grass 17 - 128x128.png", GL_TEXTURE_2D, GL_TEXTURE0, GL_RGB, GL_UNSIGNED_BYTE);
	brickTex.texUnit(triangle_rasterization, "tex0", 0);



	// Original code from the tutorial
	/*Texture brickTex("brick.png", GL_TEXTURE_2D, GL_TEXTURE0, GL_RGBA, GL_UNSIGNED_BYTE);
	brickTex.texUnit(shaderProgram, "tex0", 0);*/



	// Enables the Depth Buffer
	glEnable(GL_DEPTH_TEST);
	glfwSwapInterval(0);
	



	// Creates camera object
	Camera camera(width, height, glm::vec3(0.0f, 0.0f, 2.0f));

	//finally build some chunks

	int radius = 25;
	int num_chunks = (int)world.world_chunk_pseudo_hashes.size();
	std::vector<chunk_gen> chunk(num_chunks);

	for (int i = 0; i < num_chunks; i++) {
		glm::vec3 hash = world.world_chunk_pseudo_hashes[i];
		int chunk_x = (int)hash.x;
		int chunk_y = (int)hash.y;
		int lod = (int)hash.z;

		chunk[i].generate_chunk_vectors(chunk_x, chunk_y, lod);
		chunk[i].load_to_gpu();
	}
	
	

	// Main while loop
	int current_frame = 0;
	float time = 0.0f;
	while (!glfwWindowShouldClose(window))
	{
		std::cout << "position: " << camera.Position.y << std::endl;
		current_frame++;
		if ((int)time != (int)glfwGetTime()) {
			glfwSetWindowTitle(window, ("YoutubeOpenGL | FPS: " + std::to_string(current_frame)).c_str());
			current_frame = 0;
		}
		time = glfwGetTime();
		
		// Specify the color of the background
		glClearColor(0.07f, 0.13f, 0.17f, 1.0f);
		// Clean the back buffer and depth buffer
		glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
		// Tell OpenGL which Shader Program we want to use
		triangle_rasterization.Activate();
		triangle_rasterization.setFloat("time", glfwGetTime());
		triangle_rasterization.setVec3("camerapos", camera.Position);

		// Handles camera inputs
		camera.Inputs(window);
		// Updates and exports the camera matrix to the Vertex Shader
		camera.Matrix(45.0f, 0.1f, -1.0f, triangle_rasterization, "camMatrix");
		camera.Position;

		// Binds texture so that is appears in rendering
		brickTex.Bind();
		// Bind the VAO so OpenGL knows to use it
		for (int i = 0; i < num_chunks; i++) {
			chunk[i].vao->Bind();
			// Draw primitives, number of indices, datatype of indices, index of indices

			//chunk.vao->Bind();
			glDrawElements(GL_TRIANGLES, (GLsizei)chunk[i].indices.size(), GL_UNSIGNED_INT, 0);
		}
		// Swap the back buffer with the front buffer
		glfwSwapBuffers(window);
		// Take care of all GLFW events
		glfwPollEvents();
	}



	// Delete all the objects we've created
	//VAO1.Delete();
	//VBO1.Delete();
	//EBO1.Delete();
	for (int i = 0; i < num_chunks; i++) { chunk[i].terminate(); }
	
	brickTex.Delete();
	triangle_rasterization.Delete();
	// Delete window before ending the program
	glfwDestroyWindow(window);
	// Terminate GLFW before ending the program
	glfwTerminate();
	return 0;
}