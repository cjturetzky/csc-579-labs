#version 330 core
precision highp float;

in vec3 vN;
in vec3 vL;
in vec3 vV;

out vec4 fragColor;

// Gooch uniforms
uniform vec3 SurfaceColor; // base color (k_d)
uniform vec3 WarmColor;    // warm tint
uniform vec3 CoolColor;    // cool tint
uniform float Alpha;       // α
uniform float Beta;        // β

// Specular (Blinn-Phong)
uniform vec3  MatSpec;
uniform float MatShine;

void main()
{
    vec3 N = normalize(vN);
    vec3 L = normalize(vL);
    vec3 V = normalize(vV);

    // Gooch diffuse: mix cool->warm using remapped N·L in [-1,1] -> [0,1]
    float NdL = clamp(dot(N, L), -1.0, 1.0);
    float t   = 0.5 * (NdL + 1.0);

    vec3 kcool = CoolColor + Beta * SurfaceColor;
    vec3 kwarm = WarmColor + Alpha * SurfaceColor;
    vec3 gooch = mix(kcool, kwarm, t);

    // Blinn-Phong specular on top
    vec3 H = normalize(L + V);
    float spec = pow(max(dot(N, H), 0.0), MatShine);
    vec3 specCol = MatSpec * spec;

    fragColor = vec4(gooch + specCol, 1.0);
}
