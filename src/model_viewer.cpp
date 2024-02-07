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
// Struct for our application context
struct Context {
    int width = 800;
    int height = 800;
    GLFWwindow *window;
    gltf::GLTFAsset asset;
    gltf::DrawableList drawables;
    cg::Trackball trackball;
    GLuint program;
    GLuint emptyVAO;
    float elapsedTime;
    std::string gltfFilename = "armadillo.gltf";  //"cube_rgb.gltf";
    // Add more variables here...
    glm::vec3 ambientColor = glm::vec3(0.5, 0.05, 0.05);
    glm::vec3 diffuseColor = glm::vec3(0.0, 1.0, 0.5);
    glm::vec3 specularColor = glm::vec3(1.0, 1.0, 0.0);
    float specularPower = 80;
    glm::vec3 lightPosition = glm::vec3(-20.0, 20.0, 10.0);
    float fov = 25;

    // Bools for checkboxes
    bool perspective = true;
    bool ambient = true;
    bool diffuse = true;
    bool specular = true;
    bool normals = false;
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
    ctx.program = cg::load_shader_program(shader_dir() + "mesh_part_2_4.vert", shader_dir() + "mesh_part_2_4.frag");

    gltf::load_gltf_asset(ctx.gltfFilename, gltf_dir(), ctx.asset);
    gltf::create_drawables_from_gltf_asset(ctx.drawables, ctx.asset);
}

void draw_scene(Context &ctx)
{
    // Activate shader program
    glUseProgram(ctx.program);

    // Set render state
    glEnable(GL_DEPTH_TEST);  // Enable Z-buffering

    // Define per-scene uniforms
    glUniform1f(glGetUniformLocation(ctx.program, "u_time"), ctx.elapsedTime);




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
    glUniform1f(glGetUniformLocation(ctx.program, "u_normals"), ctx.normals);
 

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

void do_rendering(Context &ctx)
{
    // Clear render states at the start of each frame
    cg::reset_gl_render_state();

    // Clear color and depth buffers
    glClearColor(bgcolor.x, bgcolor.y, bgcolor.z, bgcolor.w);
    //glClearColor(1.0f, 0.5f, 0.0f, 0.0f);  //orange bg
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

    draw_scene(ctx);
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
    ImGui::SliderFloat3("Light position", &ctx.lightPosition[0], -50.0f, 50.0f);

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
