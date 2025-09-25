#version 330 core 

out vec4 color;

uniform vec3 MatAmb;
uniform vec3 MatDif;
uniform vec3 MatSpec;

//interpolated normal and light vector in camera space
in vec3 fragNor;
in vec3 lightDir;
//position of the vertex in camera space
in vec3 EPos;

void main()
{
	//you will need to work with these for lighting
	vec3 normal = normalize(fragNor);
	vec3 light = normalize(lightDir - EPos);

	float lambertian = max(dot(normal, light), 0.0);
	float specular = 0.0;
	if(lambertian > 0.0) {
		vec3 R = reflect(-1 * light, normal);
		vec3 V = normalize(-1 * EPos);
		float specAngle = max(dot(R, V), 0.0);
		specular = pow(specAngle, 80);
		color = vec4(MatAmb + (lambertian * MatDif) + (specular * MatSpec), 1.0);
	}
	else{
		color = vec4(MatAmb * 10, 1.0);
	}
	
}
