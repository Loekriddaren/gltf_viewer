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
uniform bool u_normals; // Toggle between RGB-normals or illumination

// ...

// Vertex inputs (attributes from vertex buffers)
layout(location = 0) in vec4 a_position;
layout(location = 1) in vec3 a_color;
layout(location = 2) in vec3 a_normal;

// Vertex shader outputs
out vec3 v_color;

void main()
{
    // Rotation
    gl_Position =   u_projection * u_view * u_model* a_position; //Apply rotation to the object

    // Choose between displaying RGB-normals or illumination
    if(u_normals)
    {
        v_color = 0.5 * a_normal + 0.5; // maps the normal direction to an RGB color
    }
    else
    {
        // Illumination
         mat4 mv = u_view * u_model; //Model-view matrix
    
        vec3 positionEye = vec3(mv * a_position); // Transform the vertex position to view space (eye coordinates)

        vec3 N = normalize(mat3(mv) * a_normal); // Calculate the view-space normal
    
        vec3 L = normalize(u_lightPosition - positionEye); // Calculate the view-space light direction

        vec3 V = normalize(-positionEye); // Calculate the viewer vector
    
        vec3 H = normalize(L+V); // Calculate the half-way vector
    
        float ambient = 0.5; // The (constant) ambient reflection term
    
        float diffuse = max(0.0, dot(N, L)); // Calculate the diffuse (Lambertian) reflection term

        float specular = pow(max(0.0, dot(N, H)), u_specularPower); // Calculate the specular reflection term

        // Multiply the reflection terms with the base colors and add together
        v_color = ambient * u_ambientColor + diffuse * u_diffuseColor + specular * u_specularColor;
    }

}
