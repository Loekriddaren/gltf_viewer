#version 330
#extension GL_ARB_explicit_attrib_location : require

// Uniform constants
uniform float u_time;
int phase = int(u_time/5);
// ...

// Vertex inputs (attributes from vertex buffers)
layout(location = 0) in vec4 a_position;
// ...

// Vertex shader outputs
// ...

void main()
{
  if(phase%2==0)
    gl_Position = vec4(a_position.x, a_position.y, a_position.z, max(sin(u_time),0.01)); //deforming scale phase
  else 
    gl_Position = vec4(a_position.x+(cos(u_time)/2), a_position.y+(sin(u_time)/2), a_position.z, 1.0); //moving in circles phase
}
