#version 330
#extension GL_ARB_explicit_attrib_location : require

// Uniform constants
// ...
uniform vec3 u_ambientColor; // The ambient surface color of the model
uniform vec3 u_diffuseColor; // The diffuse surface color of the model
uniform vec3 u_specularColor; // The specular surface color of the model
uniform float u_specularPower; // The specular power
uniform bool u_normals;
uniform bool u_gammaCorrection; // Toggle gamma correction
uniform samplerCube u_cubemap; // Index of the cubemap
uniform bool u_useCubemap; // Toggle cubemap on/off
uniform sampler2D u_texture; // The index of the texture
uniform bool u_useTexCoord; // Toggle texture coordinate visualisation on/off
uniform bool u_useTexture; // Toggle texture use on/off

// Fragment shader inputs
in vec3 v_color;
in vec3 N;
in vec3 L;
in vec3 V;
in vec2 v_texcoord_0;

// Fragment shader outputs
out vec4 frag_color;

void main()
{
    if(u_normals)
    {
        frag_color = vec4(v_color, 1.0);
    }
    else if(u_useCubemap)
    {
       vec3 R = reflect(-V, N); // Calculate the reflection vector

       vec3 color = texture(u_cubemap, R).rgb; // Get corresponding color from cubemap

       frag_color = vec4(color, 1.0);  
    }
    else if(u_useTexCoord)
    {
        // Visualise texture coordinates with rgb colors
        vec3 color = vec3 (v_texcoord_0, 0);
        frag_color = vec4(color, 1.0);  
    }
    // Standard Blinn-Phong shading
    else {
        vec3 H = normalize(L+V); // Calculate the half-way vector
    
        float ambient = 0.1*0.5; // Calculate the ambient reflection term
    
        float diffuse = 0.5*max(0.0, dot(N, L)); // Calculate the diffuse (Lambertian) reflection term

        float specular = 0.04 * pow(max(0.0, dot(N, H)), u_specularPower); // Calculate the specular reflection term

        float normalization_term = (u_specularPower + 8)/8;

        // If texture is activated, get the material base color from texture
        vec3 ambientColor;
        if(u_useTexture)
        {
            ambientColor = texture(u_texture, v_texcoord_0).rgb * 0.5;
        }
        else
        {
            ambientColor = ambient * u_ambientColor;
        }

        // Multiply the reflection terms with the base colors and add together
        vec3 color = ambientColor + diffuse * u_diffuseColor +  normalization_term * specular * u_specularColor;

        frag_color = vec4(color, 1.0);
    }

    // Gamma correction
     if(u_gammaCorrection)
    {
        frag_color = pow(frag_color, vec4(1 / 2.2));
    }
}
