#version 330 core

// Attributes (names match what the C++ binds)
in vec3 vertPos;
in vec3 vertNor;

// Uniforms
uniform mat4 P;
uniform mat4 V;
uniform mat4 M;
uniform vec3 lightPos;

// Varyings to fragment shader
out vec3 vN;     // normal in world space
out vec3 vL;     // light direction (world)
out vec3 vV;     // view direction (world)

void main()
{
    // World-space position
    vec4 wPos = M * vec4(vertPos, 1.0);

    // Proper normal transform
    mat3 Nmat = mat3(transpose(inverse(M)));
    vN = normalize(Nmat * vertNor);

    // Light and view directions in world space
    vec3 worldPos = wPos.xyz;
    vL = normalize(lightPos - worldPos);
    vV = normalize(-worldPos); // camera at (0,0,0) in world for this setup

    gl_Position = P * V * wPos;
}
