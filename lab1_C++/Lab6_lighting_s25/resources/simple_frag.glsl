#version 330 core 

out vec4 color;

uniform vec3 MatAmb;

//interpolated normal and light vector in camera space
in vec3 fragNor;
in vec3 lightDir;
//position of the vertex in camera space
in vec3 EPos;

void main()
{
	//you will need to work with these for lighting
	vec3 normal = normalize(fragNor);
	vec3 light = normalize(lightDir);

	color = vec4(MatAmb*10.0, 1.0);
}
