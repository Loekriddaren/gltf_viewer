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

uniform sampler2D u_shadowmap; // The index of the shadowmap
uniform float u_shadowBias; // The shadow bias
uniform bool u_useShadowmap; // Toggle shadows on/off

// Fragment shader inputs
in vec3 v_color;
in vec3 N;
in vec3 L;
in vec3 V;
in vec2 v_texcoord_0;
in vec4 v_shadowPos;

// Fragment shader outputs
out vec4 frag_color;

// Function to compare depth with shadowmap
float shadowmap_visibility(sampler2D shadowmap, vec4 shadowPos, float bias)
{
    vec2 delta = vec2(0.5) / textureSize(shadowmap, 0).xy;
    vec2 texcoord = (shadowPos.xy / shadowPos.w) * 0.5 + 0.5;
    float depth = (shadowPos.z / shadowPos.w) * 0.5 + 0.5;
    
    // Sample the shadowmap and compare texels with (depth - bias) to
    // return a visibility value in range [0, 1].
    // Use delta to offset the texture coordinate to get more samples
    // around a point and return the average of all comparisons.
    float average = 0;
    for (int i = -1; i < 2; ++i) {
        for (int j = -1; j < 2; ++j) {
            float texel = texture(shadowmap, vec2(texcoord.x + delta.x*i, texcoord.y + delta.y*j)).r;
            float visibility = float(texel > depth - bias);
            average += visibility;
        }
    }
    
    return average/9;
}

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

        float visibility = 1.0;
        // If shadow map is active, change the visibility of the color accordingly
        if(u_useShadowmap)
        {
            visibility = shadowmap_visibility( u_shadowmap, v_shadowPos, u_shadowBias);
        }
        

        vec3 baseColor;
        // If texture is activated, get the material base color from texture
        if(u_useTexture)
        {
            baseColor = texture(u_texture, v_texcoord_0).rgb * 0.5;
        }
        else
        {
            baseColor = ambient * u_ambientColor;
        }

        // Multiply the reflection terms with the base colors and add together
        vec3 color = baseColor + visibility*(diffuse * u_diffuseColor + normalization_term * specular * u_specularColor);

        frag_color = vec4(color, 1.0);
    }

    // Gamma correction
     if(u_gammaCorrection)
    {
        frag_color = pow(frag_color, vec4(1 / 2.2));
    }
}
