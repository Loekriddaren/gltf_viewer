#version 330
#extension GL_ARB_explicit_attrib_location : require

// Uniform constants
// ...
uniform vec3 u_ambientColor; // The ambient surface color of the model
uniform vec3 u_diffuseColor; // The diffuse surface color of the model
uniform vec3 u_specularColor; // The specular surface color of the model
uniform float u_specularPower; // The specular power
uniform bool u_normals;

// Fragment shader inputs
in vec3 v_color;
in vec3 N;
in vec3 L;
in vec3 V;

// Fragment shader outputs
out vec4 frag_color;

void main()
{
    if(u_normals)
        {
            frag_color = vec4(v_color, 1.0);
        }
    else {
            vec3 H = normalize(L+V); // Calculate the half-way vector
    
            float ambient = 0.1; // Calculate the ambient reflection term
    
            float diffuse = max(0.0, dot(N, L)); // Calculate the diffuse (Lambertian) reflection term

            float specular = 0.04*pow(max(0.0, dot(N, H)), u_specularPower); // Calculate the specular reflection term

            float normalization_term = (u_specularPower + 8)/8;

            // Multiply the reflection terms with the base colors and add together
            vec3 color = ambient * u_ambientColor + diffuse * u_diffuseColor +  normalization_term * specular * u_specularColor;
            
            // Gamma correction
            color = pow(color, vec3(1 / 2.2));
            frag_color = vec4(color, 1.0);
         }
}