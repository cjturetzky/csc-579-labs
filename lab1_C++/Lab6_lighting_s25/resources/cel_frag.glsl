#version 330 core
in vec3 vN;
in vec3 vL;
in vec3 vV;

uniform vec3 MatAmb;
uniform vec3 MatDif;
uniform vec3 MatSpec;
uniform float MatShine;

out vec4 FragColor;

float stepBand(float x, float bands) { return floor(x * bands) / bands; }

void main() {
    vec3 N = normalize(vN);
    vec3 L = normalize(vL);
    vec3 V = normalize(vV);
    vec3 H = normalize(L + V);

    // Quantize diffuse into a few bands
    float ndl = max(dot(N, L), 0.0);
    float bands = 3.0;                 // try 2–4
    float dQ = stepBand(ndl, bands);

    // Thresholded specular highlight
    float nsh = pow(max(dot(N, H), 0.0), MatShine);
    float sQ = step(0.5, nsh);         // binary highlight

    // Optional rim (ink near silhouette)
    float rim = pow(1.0 - max(dot(N, V), 0.0), 2.0);
    float rimQ = step(0.6, rim) * 0.25;

    vec3 color = MatAmb + dQ * MatDif + sQ * MatSpec + rimQ * vec3(1.0);
    FragColor = vec4(color, 1.0);
}