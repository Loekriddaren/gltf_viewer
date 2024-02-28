#version 330
#extension GL_ARB_explicit_attrib_location : require

// Uniform constants
uniform float u_time;
uniform mat4 u_view;
uniform mat4 u_model;
uniform mat4 u_projection;
uniform vec3 u_ambientColor; // The ambient surface color of the model
uniform vec3 u_diffuseColor; // The diffuse surface color of the model
uniform vec3 u_specularColor; // The specular surface color of the model
uniform float u_specularPower; // The specular power
uniform vec3 u_lightPosition; // The position of your light source
uniform bool u_normals;

// ...

// Vertex inputs (attributes from vertex buffers)
layout(location = 0) in vec4 a_position;
layout(location = 1) in vec3 a_color;
layout(location = 2) in vec3 a_normal;

// Vertex shader outputs
out vec3 v_color;
out vec3 N;
out vec3 L;
out vec3 V;

void main()
{
    // Rotation
    gl_Position =   u_projection * u_view * u_model* a_position; //Apply rotation to the object

    if(u_normals)
    {
        v_color = 0.5 * a_normal + 0.5; // maps the normal direction to an RGB color
    }
    else
    {
        // Illumination
         mat4 mv = u_view * u_model; //Model-view matrix
    
        vec3 positionEye = vec3(mv * a_position); // Transform the vertex position to view space (eye coordinates)

        vec3 positionLight = vec3(u_view * vec4(u_lightPosition, 1.0)); //Transform the light position to view-space

        N = normalize(mat3(mv) * a_normal); // Calculate the view-space normal
   
        L = normalize(positionLight - positionEye); // Calculate the view-space light direction

        V = normalize(-positionEye); // Calculate the viewer vector
    
    }

}
