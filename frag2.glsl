#version 330 core

// All of the following variables could be defined in the OpenGL
// program and passed to this shader as uniform variables. This
// would be necessary if their values could change during runtim.
// However, we will not change them and therefore we define them 
// here for simplicity.

vec3 I = vec3(1, 1, 1);          // point light intensity
vec3 Iamb = vec3(0.1, 0.1, 0.1); // ambient light intensity
//vec3 kd = vec3(0.86, 0.11, 0.31);     // diffuse reflectance coefficient
vec3 ka = vec3(0.1, 0.1, 0.1);   // ambient reflectance coefficient
vec3 ks = vec3(0.5, 0.5, 0.5);   // specular reflectance coefficient

uniform vec3 lightPos;
uniform vec3 eyePos;
uniform vec3 kd;

in vec4 fragWorldPos;
flat in vec3 fragWorldNor;

out vec4 fragColor;

void main(void)
{
	fragColor = vec4(1.0, 1.0, 1.0, 1.0); // White color for edges
    return;
}
