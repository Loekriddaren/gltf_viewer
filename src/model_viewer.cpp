// Model viewer code for the assignments in Computer Graphics 1TD388/1MD150.
//
// Modify this and other source files according to the tasks in the instructions.
//

#include "gltf_io.h"
#include "gltf_scene.h"
#include "gltf_render.h"
#include "cg_utils.h"
#include "cg_trackball.h"

#include <GL/gl3w.h>
#include <GLFW/glfw3.h>

#include <imgui.h>
#include <imgui_impl_glfw.h>
#include <imgui_impl_opengl3.h>

#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>

#include <cstdlib>
#include <iostream>


static ImVec4 bgcolor = ImVec4(0.0f / 255.0f, 0.0f / 255.0f, 0.0f / 255.0f, 255.0f / 255.0f);

// Struct for representing a shadow casting point light
struct ShadowCastingLight {
    glm::vec3 position;      // Light source position
    glm::mat4 shadowMatrix;  // Camera matrix for shadowmap
    GLuint shadowmap;        // Depth texture
    GLuint shadowFBO;        // Depth framebuffer
    float shadowBias;        // Bias for depth comparison
};

// Struct for our application context
struct Context {
    int width = 800;
    int height = 800;
    int active_cubemap;
    GLFWwindow *window;
    gltf::GLTFAsset asset;
    gltf::DrawableList drawables;
    cg::Trackball trackball;
    GLuint program;
    GLuint emptyVAO;
    GLuint cubemap;
    gltf::TextureList textures;
    float elapsedTime;
    std::string gltfFilename = "lpshead.gltf";  //"cube_rgb.gltf";
    // Add more variables here...
    glm::vec3 ambientColor = glm::vec3(0.5, 0.05, 0.05);
    glm::vec3 diffuseColor = glm::vec3(0.0, 1.0, 0.5);
    glm::vec3 specularColor = glm::vec3(1.0, 1.0, 0.0);
    float specularPower = 80;
    glm::vec3 lightPosition = glm::vec3(-3.3, 2.5, 0.0);
    float fov = 25;

    // For shadow mapping
    ShadowCastingLight light;
    GLuint shadowProgram;
    bool showShadowmap = false;
    bool useShadowmap = false;
    glm::mat4 shadowFromView;

    // Bools for checkboxes
    bool perspective = true;
    bool ambient = true;
    bool diffuse = true;
    bool specular = true;
    bool normals = false;
    bool gammaCorrection = false;
    bool useCubemap = false;
    bool useTexCoord = false;
    bool useTexture = false;
};

// Returns the absolute path to the src/shader directory
std::string shader_dir(void)
{
    std::string rootDir = cg::get_env_var("MODEL_VIEWER_ROOT");
    if (rootDir.empty()) {
        std::cout << "Error: MODEL_VIEWER_ROOT is not set." << std::endl;
        std::exit(EXIT_FAILURE);
    }
    return rootDir + "/src/shaders/";
}

std::string cubemap_dir(void)
{
    std::string rootDir = cg::get_env_var("MODEL_VIEWER_ROOT");
    if (rootDir.empty()) {
        std::cout << "Error: MODEL_VIEWER_ROOT is not set." << std::endl;
        std::exit(EXIT_FAILURE);
    }
    return rootDir + "/assets/cubemaps/";
}

// Returns the absolute path to the assets/gltf directory
std::string gltf_dir(void)
{
    std::string rootDir = cg::get_env_var("MODEL_VIEWER_ROOT");
    if (rootDir.empty()) {
        std::cout << "Error: MODEL_VIEWER_ROOT is not set." << std::endl;
        std::exit(EXIT_FAILURE);
    }
    return rootDir + "/assets/gltf/";
}

void do_initialization(Context &ctx)
{
    ctx.program = cg::load_shader_program(shader_dir() + "mesh_part_3_4.vert", shader_dir() + "mesh_part_3_4.frag");


    //load the textures for reflectifve cubemaps
    std::string textt = "RomeChurch";
    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_CUBE_MAP,
                  cg::load_cubemap(cubemap_dir() + "/"+textt+"/prefiltered/0.125/"));
    glActiveTexture(GL_TEXTURE0 + 1);
    glBindTexture(GL_TEXTURE_CUBE_MAP,
                  cg::load_cubemap(cubemap_dir() + "/" + textt + "/prefiltered/0.5/"));
    glActiveTexture(GL_TEXTURE0 + 2);
    glBindTexture(GL_TEXTURE_CUBE_MAP,
                  cg::load_cubemap(cubemap_dir() + "/" + textt + "/prefiltered/2/"));
    glActiveTexture(GL_TEXTURE0 + 3);
    glBindTexture(GL_TEXTURE_CUBE_MAP,
                  cg::load_cubemap(cubemap_dir() + "/" + textt + "/prefiltered/8/"));
    glActiveTexture(GL_TEXTURE0 + 4);
    glBindTexture(GL_TEXTURE_CUBE_MAP,
                  cg::load_cubemap(cubemap_dir() + "/" + textt + "/prefiltered/32/"));
    glActiveTexture(GL_TEXTURE0 + 5);
    glBindTexture(GL_TEXTURE_CUBE_MAP,
                  cg::load_cubemap(cubemap_dir() + "/" + textt + "/prefiltered/128/"));
    glActiveTexture(GL_TEXTURE0 + 6);
    glBindTexture(GL_TEXTURE_CUBE_MAP,
                  cg::load_cubemap(cubemap_dir() + "/" + textt + "/prefiltered/512/"));
    glActiveTexture(GL_TEXTURE0 + 7);
    glBindTexture(GL_TEXTURE_CUBE_MAP,
                  cg::load_cubemap(cubemap_dir() + "/" + textt + "/prefiltered/2048/"));


    gltf::load_gltf_asset(ctx.gltfFilename, gltf_dir(), ctx.asset);
    gltf::create_drawables_from_gltf_asset(ctx.drawables, ctx.asset);
    gltf::create_textures_from_gltf_asset(ctx.textures, ctx.asset);

    // For shadow mapping
    ctx.shadowProgram = cg::load_shader_program(shader_dir() + "shadow.vert", shader_dir() + "shadow.frag");
    ctx.light.shadowmap = cg::create_depth_texture(1024, 1024);
    ctx.light.shadowFBO = cg::create_depth_framebuffer(ctx.light.shadowmap);
    ctx.light.position = ctx.lightPosition;
    ctx.light.shadowBias = 0.001f;
    ctx.light.shadowMatrix = glm::mat4(1.0f);
}



void draw_scene(Context &ctx)
{
    // Activate shader program
    glUseProgram(ctx.program);

    // Set render state
    glEnable(GL_DEPTH_TEST);  // Enable Z-buffering

    // Define per-scene uniforms
    glUniform1f(glGetUniformLocation(ctx.program, "u_time"), ctx.elapsedTime);
    

    glUniform1i(glGetUniformLocation(ctx.program, "u_cubemap"), ctx.active_cubemap);

    // Pass on uniforms for different render modes
    glUniform1i(glGetUniformLocation(ctx.program, "u_useCubemap"), ctx.useCubemap);
    glUniform1f(glGetUniformLocation(ctx.program, "u_normals"), ctx.normals);
    glUniform1f(glGetUniformLocation(ctx.program, "u_useTexCoord"), ctx.useTexCoord);
    glUniform1f(glGetUniformLocation(ctx.program, "u_useTexture"), ctx.useTexture);
    glUniform1f(glGetUniformLocation(ctx.program, "u_gammaCorrection"), ctx.gammaCorrection);
    glUniform1f(glGetUniformLocation(ctx.program, "u_useShadowmap"), ctx.useShadowmap);


    // Define trackball matrix to move the view
    glm::mat4 view = glm::mat4(ctx.trackball.orient);
    //Define camera
    glm::mat4 camera = glm::lookAt(glm::vec3(0.0f, .0f, 10.0f), glm::vec3(0.0f, 0.0f, 0.0f),glm::vec3(0.0f, 1.0f, 0.0f));

    //Apply trackball to camera
    view = camera * view;

    glUniformMatrix4fv(glGetUniformLocation(ctx.program, "u_view"), 1, GL_FALSE, &view[0][0]);

    //define perspective and orthogonal projection matrix
    float ratio = ctx.width / ctx.height;
    glm::mat4 projection;
    if (ctx.perspective){
        projection = glm::perspective(glm::radians(ctx.fov), ratio, 1.0f, 50.0f);
    } 
    else{
        projection = glm::ortho(-2.0f, 2.0f, -2.0f, 2.0f, 0.0f, 20.0f);
    }

    glUniformMatrix4fv(glGetUniformLocation(ctx.program, "u_projection"), 1, GL_FALSE, &projection[0][0]);

    //Pass light uniforms to shader (if light is toggled on)
    glm::vec3 ambient = glm::vec3(0.0f, 0.0f, 0.0f);
    glm::vec3 diffuse = glm::vec3(0.0, 0.0, 0.0);
    glm::vec3 specular = glm::vec3(0.0, 0.0, 0.0);
    if (ctx.ambient) { ambient = ctx.ambientColor; }
    if (ctx.diffuse) { diffuse = ctx.diffuseColor; }
    if (ctx.specular) { specular = ctx.specularColor; }

    glUniform3fv(glGetUniformLocation(ctx.program, "u_ambientColor"), 1, &ambient[0]);
    glUniform3fv(glGetUniformLocation(ctx.program, "u_diffuseColor"), 1, &diffuse[0]);
    glUniform3fv(glGetUniformLocation(ctx.program, "u_specularColor"), 1, &specular[0]);
    glUniform3fv(glGetUniformLocation(ctx.program, "u_lightPosition"), 1, &ctx.lightPosition[0]);
    glUniform1f(glGetUniformLocation(ctx.program, "u_specularPower"), ctx.specularPower);

    // Pass matrix for shadow calculations
    glUniformMatrix4fv(glGetUniformLocation(ctx.program, "u_shadowFromView"), 1, GL_FALSE, &ctx.light.shadowMatrix[0][0]);
    glActiveTexture(GL_TEXTURE9);
    glBindTexture(GL_TEXTURE_2D, ctx.light.shadowmap);
    glUniform1i(glGetUniformLocation(ctx.program, "u_shadowmap"), 9);
    glUniform1f(glGetUniformLocation(ctx.program, "u_shadowBias"), ctx.light.shadowBias);

 

    // Draw scene
    for (unsigned i = 0; i < ctx.asset.nodes.size(); ++i) {
        const gltf::Node &node = ctx.asset.nodes[i];
        const gltf::Drawable &drawable = ctx.drawables[node.mesh];

        // Define per-object uniforms
        // ...
        // Define model matrix
        glm::mat4 model = glm::mat4(1.0f);
        model = glm::translate(model, node.translation);
        model = glm::scale(model, node.scale);
        glm::mat4 rot = glm::mat4_cast(node.rotation);
        model = model * rot;
        glUniformMatrix4fv(glGetUniformLocation(ctx.program, "u_model"), 1, GL_FALSE, &model[0][0]);

        // Define per-object texture
        const gltf::Mesh &mesh = ctx.asset.meshes[node.mesh];
        if (mesh.primitives[0].hasMaterial) {
            const gltf::Primitive &primitive = mesh.primitives[0];
            const gltf::Material &material = ctx.asset.materials[primitive.material];
            const gltf::PBRMetallicRoughness &pbr = material.pbrMetallicRoughness;

            // Define material textures and uniforms
            if (pbr.hasBaseColorTexture) {
                GLuint texture_id = ctx.textures[pbr.baseColorTexture.index];
                glActiveTexture(GL_TEXTURE8);
                glBindTexture(GL_TEXTURE_2D, texture_id);
                glUniform1i(glGetUniformLocation(ctx.program, "u_texture"), 8);

            } else {
                // If no texture available, always disable the use of textures in shader
                ctx.useTexCoord = false;
                ctx.useTexture = false;
            }
        }

        // Draw object
        glBindVertexArray(drawable.vao);
        glDrawElements(GL_TRIANGLES, drawable.indexCount, drawable.indexType,
                       (GLvoid *)(intptr_t)drawable.indexByteOffset);
        glBindVertexArray(0);
    }

    // Clean up
    cg::reset_gl_render_state();
    glUseProgram(0);
}

// Update the shadowmap and shadow matrix for a light source
void update_shadowmap(Context &ctx, ShadowCastingLight &light, GLuint shadowFBO)
{
    // Set up rendering to shadowmap framebuffer
    glBindFramebuffer(GL_DRAW_FRAMEBUFFER, shadowFBO);
    if (shadowFBO) glViewport(0, 0, 1024, 1024);  // TODO Set viewport to shadowmap size
    glClear(GL_DEPTH_BUFFER_BIT);                 // Clear depth values to 1.0

    // Set up pipeline
    glUseProgram(ctx.shadowProgram);
    glEnable(GL_DEPTH_TEST);  // Enable Z-buffering

    // Matrices for the shadowmap camera.
    // The view matrix is a lookAt-matrix computed from the light source position
    // The projection matrix is a frustum that covers the parts of the scene that recieves shadows
    light.position = ctx.lightPosition;
    glm::mat4 view =
        glm::lookAt(ctx.lightPosition, glm::vec3(0.0f, 0.0f, 0.0f), glm::vec3(0.0f, 1.0f, 0.0f));

    glm::mat4 proj = glm::perspective(glm::radians(30.0f), 1.0f, 1.0f, 500.0f);

    glUniformMatrix4fv(glGetUniformLocation(ctx.shadowProgram, "u_view"), 1, GL_FALSE, &view[0][0]);
    glUniformMatrix4fv(glGetUniformLocation(ctx.shadowProgram, "u_proj"), 1, GL_FALSE, &proj[0][0]);

    // Store updated shadow matrix for use in draw_scene()
    light.shadowMatrix = proj * view;

    // Draw scene
    for (unsigned i = 0; i < ctx.asset.nodes.size(); ++i) {
        const gltf::Node &node = ctx.asset.nodes[i];
        const gltf::Drawable &drawable = ctx.drawables[node.mesh];

        // Define the model matrix for the drawable
        glm::mat4 model = glm::mat4(1.0f);
        model = glm::translate(model, node.translation);
        model = glm::scale(model, node.scale);
        glm::mat4 rot = glm::mat4_cast(node.rotation);
        model = model * rot;
        glUniformMatrix4fv(glGetUniformLocation(ctx.shadowProgram, "u_model"), 1, GL_FALSE,
                           &model[0][0]);

        // Draw object
        glBindVertexArray(drawable.vao);
        glDrawElements(GL_TRIANGLES, drawable.indexCount, drawable.indexType,
                       (GLvoid *)(intptr_t)drawable.indexByteOffset);
        glBindVertexArray(0);
    }

    // Clean up
    cg::reset_gl_render_state();
    glUseProgram(0);
    glViewport(0, 0, ctx.width, ctx.height);
    glBindFramebuffer(GL_DRAW_FRAMEBUFFER, 0);
}

void do_rendering(Context &ctx)
{
    // Clear render states at the start of each frame
    cg::reset_gl_render_state();

    // Clear color and depth buffers
    glClearColor(bgcolor.x, bgcolor.y, bgcolor.z, bgcolor.w);
    //glClearColor(1.0f, 0.5f, 0.0f, 0.0f);  //orange bg
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

    update_shadowmap(ctx, ctx.light, ctx.light.shadowFBO);
    draw_scene(ctx);

    if (ctx.showShadowmap) {
        // Draw shadowmap on default screen framebuffer
        glClearColor(1.0f, 1.0f, 1.0f, 1.0f);
        glClear(GL_COLOR_BUFFER_BIT);
        update_shadowmap(ctx, ctx.light, 0);
    }
}

void reload_shaders(Context *ctx)
{
    glDeleteProgram(ctx->program);
    ctx->program = cg::load_shader_program(shader_dir() + "mesh.vert", shader_dir() + "mesh.frag");
}

void error_callback(int /*error*/, const char *description)
{
    std::cerr << description << std::endl;
}

void key_callback(GLFWwindow *window, int key, int scancode, int action, int mods)
{
    // Forward event to ImGui
    ImGui_ImplGlfw_KeyCallback(window, key, scancode, action, mods);
    if (ImGui::GetIO().WantCaptureKeyboard) return;

    Context *ctx = static_cast<Context *>(glfwGetWindowUserPointer(window));
    if (key == GLFW_KEY_R && action == GLFW_PRESS) { reload_shaders(ctx); }
}

void char_callback(GLFWwindow *window, unsigned int codepoint)
{
    // Forward event to ImGui
    ImGui_ImplGlfw_CharCallback(window, codepoint);
    if (ImGui::GetIO().WantTextInput) return;
}

void mouse_button_callback(GLFWwindow *window, int button, int action, int mods)
{
    // Forward event to ImGui
    ImGui_ImplGlfw_MouseButtonCallback(window, button, action, mods);
    if (ImGui::GetIO().WantCaptureMouse) return;

    double x, y;
    glfwGetCursorPos(window, &x, &y);

    Context *ctx = static_cast<Context *>(glfwGetWindowUserPointer(window));
    if (button == GLFW_MOUSE_BUTTON_LEFT) {
        ctx->trackball.center = glm::vec2(x, y);
        ctx->trackball.tracking = (action == GLFW_PRESS);
    }
}

void cursor_pos_callback(GLFWwindow *window, double x, double y)
{
    // Forward event to ImGui
    if (ImGui::GetIO().WantCaptureMouse) return;

    Context *ctx = static_cast<Context *>(glfwGetWindowUserPointer(window));
    cg::trackball_move(ctx->trackball, float(x), float(y));
}

void scroll_callback(GLFWwindow *window, double x, double y)
{
    // Forward event to ImGui
    ImGui_ImplGlfw_ScrollCallback(window, x, y);
    if (ImGui::GetIO().WantCaptureMouse) return;

    Context *ctx = static_cast<Context *>(glfwGetWindowUserPointer(window));
    ctx->fov -= y;
}

void resize_callback(GLFWwindow *window, int width, int height)
{
    // Update window size and viewport rectangle
    Context *ctx = static_cast<Context *>(glfwGetWindowUserPointer(window));
    ctx->width = width;
    ctx->height = height;
    glViewport(0, 0, width, height);
}

//CustomGui
void DrawGui(Context &ctx)
{

    //ImGui::SetNextWindowSize(ImVec2(250, 250));
    ImGui::Begin("color menu");

    ImGui::ColorEdit3("Background color", (float *)&bgcolor);
    ImGui::ColorEdit3("Ambient color", &ctx.ambientColor[0]);
    ImGui::ColorEdit3("Diffuse color", &ctx.diffuseColor[0]);
    ImGui::ColorEdit3("Specular color", &ctx.specularColor[0]);
    ImGui::SliderFloat("Specular power", &ctx.specularPower, 0.0f, 150.0f);
    ImGui::SliderFloat3("Light position", &ctx.lightPosition[0], -20.0f, 20.0f);
    ImGui::SliderInt("Active Cubemap", &ctx.active_cubemap, 0, 7);
    ImGui::SliderFloat("Shadow bias", &ctx.light.shadowBias, 0.0f, 0.1f);

    // Combobox for perspective/orthogonal projection
    const char *items[] = {"Perspective projection", "Orthogonal projection"};
    static const char *current_item = items[0];
    if (ImGui::BeginCombo("Projection", current_item))
    {
        for (int n = 0; n < IM_ARRAYSIZE(items); n++) {
            bool is_selected = (current_item == items[n]);
            if (ImGui::Selectable(items[n], is_selected)) 
            { 
                current_item = items[n]; 
            }
        }

        ImGui::EndCombo();
    }
    
    if (current_item == items[0]) { 
        ctx.perspective = true; 
    } 
    else {
        ctx.perspective = false;
    }

    //Checkboxes
    ImGui::Checkbox("Ambient light", &ctx.ambient);
    ImGui::Checkbox("Diffuse light", &ctx.diffuse);
    ImGui::Checkbox("Specular light", &ctx.specular);
    ImGui::Checkbox("Show normals", &ctx.normals);
    ImGui::Checkbox("Gamma correction", &ctx.gammaCorrection);
    ImGui::Checkbox("Use cubemap", &ctx.useCubemap);
    ImGui::Checkbox("Show texture coordinates", &ctx.useTexCoord);
    ImGui::Checkbox("Use texture", &ctx.useTexture);
    ImGui::Checkbox("Show shadowmap", &ctx.showShadowmap);
    ImGui::Checkbox("Use shadowmap", &ctx.useShadowmap);

    
    



    ImGui::End();


}


int main(int argc, char *argv[])
{
    Context ctx = Context();
    if (argc > 1) { ctx.gltfFilename = std::string(argv[1]); }

    // Create a GLFW window
    glfwSetErrorCallback(error_callback);
    glfwInit();
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
    glfwWindowHint(GLFW_OPENGL_FORWARD_COMPAT, GL_TRUE);
    ctx.window = glfwCreateWindow(ctx.width, ctx.height, "Model viewer", nullptr, nullptr);
    glfwMakeContextCurrent(ctx.window);
    glfwSetWindowUserPointer(ctx.window, &ctx);
    glfwSetKeyCallback(ctx.window, key_callback);
    glfwSetCharCallback(ctx.window, char_callback);
    glfwSetMouseButtonCallback(ctx.window, mouse_button_callback);
    glfwSetCursorPosCallback(ctx.window, cursor_pos_callback);
    glfwSetScrollCallback(ctx.window, scroll_callback);
    glfwSetFramebufferSizeCallback(ctx.window, resize_callback);

    // Load OpenGL functions
    if (gl3wInit() || !gl3wIsSupported(3, 3) /*check OpenGL version*/) {
        std::cerr << "Error: failed to initialize OpenGL" << std::endl;
        std::exit(EXIT_FAILURE);
    }
    std::cout << "OpenGL version: " << glGetString(GL_VERSION) << std::endl;

    // Initialize ImGui
    ImGui::CreateContext();
    ImGui_ImplGlfw_InitForOpenGL(ctx.window, false /*do not install callbacks*/);
    ImGui_ImplOpenGL3_Init("#version 330" /*GLSL version*/);

    // Initialize rendering
    glGenVertexArrays(1, &ctx.emptyVAO);
    glBindVertexArray(ctx.emptyVAO);
    glEnable(GL_TEXTURE_CUBE_MAP_SEAMLESS);
    do_initialization(ctx);

    // Start rendering loop
    while (!glfwWindowShouldClose(ctx.window)) {
        glfwPollEvents();
        ctx.elapsedTime = glfwGetTime();

        ImGui_ImplOpenGL3_NewFrame();
        ImGui_ImplGlfw_NewFrame();
        ImGui::NewFrame();
        DrawGui(ctx);
        //ImGui::ShowDemoWindow();
        do_rendering(ctx);
        ImGui::Render();
        ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());

        glfwSwapBuffers(ctx.window);
    }

    // Shutdown
    ImGui_ImplOpenGL3_Shutdown();
    ImGui_ImplGlfw_Shutdown();
    ImGui::DestroyContext();
    glfwDestroyWindow(ctx.window);
    glfwTerminate();
    std::exit(EXIT_SUCCESS);
}
