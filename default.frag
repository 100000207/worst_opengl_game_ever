#version 330 core

// Outputs colors in RGBA
out vec4 FragColor;


// Inputs the color from the Vertex Shader
in vec3 Fragcoord;
in vec3 normal;
// Inputs the texture coordinates from the Vertex Shader
in vec2 texCoord;
uniform float time;
uniform vec3 camerapos;
// Gets the Texture Unit from the main function
uniform sampler2D tex0;


void main()
{
	//bill phong shading

	float PI = 3.1415926535897932384626433832795;

	vec3 diffusecolor =	vec3(1.0, 0.97, 0.9);
	vec3 specularcolor = vec3(1.0, 0.98, 0.95);
	vec3 ambientcolor = vec3(0.2, 0.25, 0.35);
	float shineness_exponent = 3.0f;

	vec3 sun = vec3(0.0, cos(time*0.2 - (PI * floor(time*0.2/PI))-PI/2.f), sin(time*0.2));
	vec3 cameradir = normalize(Fragcoord-camerapos);

	vec3 H = normalize(sun + normalize(cameradir));
	vec3 reflection_vector = -normalize(cameradir - 2*(dot(cameradir,normal)*normal));
	
	vec3 diffuse_component = diffusecolor*(dot(normal,sun));
	vec3 specular_component = specularcolor*(pow((dot(H,normal)),shineness_exponent)+pow((dot(cameradir,reflection_vector)),shineness_exponent));
	vec3 ambient_component = ambientcolor;

	diffuse_component = max(diffuse_component,0);
	specular_component = max(specular_component,0);
	ambient_component = max(ambient_component,0);

	vec3 lighting = diffuse_component + specular_component + ambient_component;
	
	FragColor = texture(tex0, texCoord)*vec4(vec3(lighting),1.0f);
}