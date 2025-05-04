#include "test_compute_shader.h"
#include "../renderer.h"

#include <random>

#include "imgui.h"
#include "imgui_impl_glfw.h"
#include "imgui_impl_opengl3.h"

#include "glm/glm.hpp"
#include "glm/gtc/matrix_transform.hpp"

#include "constants.h"

extern GLFWwindow * c_window;

namespace test{

    TestComputeShader::TestComputeShader()
        : m_camera(m_width, m_height, glm::vec3(0.0f, 0.0f, 100.0f)),
        m_noise_intensity(0.0f) {

        //Object distribution grid
        int grid_x = 45;
        int grid_y = 10;
        int grid_z = 10;
        // int grid_x = 32;
        // int grid_y = 32;
        // int grid_z = 32;
        // int grid_x = 100;
        // int grid_y = 100;
        // int grid_z = 100;
        
        float spacing = 1.0f;

        m_instances = grid_x * grid_y * grid_z + 2;

        // Cube vertices
        m_vertices = std::vector<SimpleVertex>(std::begin(CONSTANTS::CUBE_MESH_SIMPLE_VERTICES), std::end(CONSTANTS::CUBE_MESH_SIMPLE_VERTICES));
    
        m_indices = std::vector<unsigned int>(std::begin(CONSTANTS::CUBE_MESH_INDICES), std::end(CONSTANTS::CUBE_MESH_INDICES));

        // Initialize m_colors with the number of instances
        m_colors = new std::vector<glm::vec4>(m_instances, glm::vec4(1.0f));

        // Generate random colors for each instance
        std::random_device rd;
        // static std::mt19937 gen(rd());
        static std::mt19937 gen(42);
        std::uniform_real_distribution<float> dist(0.0f, 1.0f);

        // for (int i = 0; i < m_instances; i++) {
        //     m_colors->at(i) = glm::vec4(dist(gen), dist(gen), dist(gen), 1.0f);
        // }
        for (int i = 0; i < m_instances; i++) {
            m_colors->at(i) = glm::vec4(0.2f,0.2f,0.2f, 1.0f);
        }   

        m_model_matrices = new std::vector<glm::mat4>(m_instances);
        

        glm::vec3 center_offset = glm::vec3((grid_x - 1) * 0.5f * spacing, 
                                    (grid_y - 1) * 0.5f * spacing, 
                                    -(grid_z - 1) * 0.5f * spacing);

        glm::vec3 global_offset = glm::vec3(0.0f, 0.0f, 0.0f);  // Shift whole grid +5 in x

        for (int x = 0; x < grid_x; x++) {
            for (int y = 0; y < grid_y; y++) {
                for (int z = 0; z < grid_z; z++) {
                    glm::mat4 model = glm::mat4(1.0f);
                    glm::vec3 position = glm::vec3(x * spacing, y * spacing, -z * spacing) - center_offset + global_offset;
                    model = glm::translate(model, position);
                    m_model_matrices->at(x * grid_y * grid_z + y * grid_z + z) = model;
                }
            }
        }

        glm::mat4 model = glm::mat4(1.0f);
        glm::vec3 scale = glm::vec3(5.0f);                   // Uniform scale
        glm::vec3 position = glm::vec3(-50.0f, -50.0f, 0.0f);
        
        model = glm::translate(model, position);              // Then translate
        model = glm::scale(model, scale);                     // Scale first
        
        m_model_matrices->at(m_instances - 2) = model;

        model = glm::mat4(1.0f);
        scale = glm::vec3(50.0f);                   // Uniform scale
        position = glm::vec3(0.0f, -40.0f, 0.0f);
        
        model = glm::translate(model, position);              // Then translate
        model = glm::scale(model, scale);                     // Scale first
        
        m_model_matrices->at(m_instances - 1) = model;
        
        m_cube.setData(m_vertices, m_indices, *m_model_matrices, *m_colors, m_instances);
        m_shader.setShader("tex_gpu_renderer.glsl");

        m_simulator = new GpuSimulator(m_model_matrices, &m_vertices, &m_indices);

        m_light_shader.setShader("light.glsl");

        try {
            m_noiseTexture = std::make_unique<Texture>("noise512.png"); // Ensure you have a noise texture file
        } catch (std::exception& e) {
            std::cerr << "Failed to load noise texture: " << e.what() << std::endl;
            m_noiseTexture = nullptr;
        }

        GLCall(glViewport(0, 0, m_width, m_height));
        GLCall(glClearColor(0.0f, 0.0f, 0.0f, 1.0f));
        GLCall(glEnable(GL_DEPTH_TEST));

        GLCall(glEnable(GL_CULL_FACE));
        GLCall(glCullFace(GL_BACK));
        GLCall(glFrontFace(GL_CCW));
    }

    TestComputeShader::~TestComputeShader(){
        delete m_simulator;
        delete m_model_matrices;

    }

    void TestComputeShader::onUpdate(float delta_time){ 
        m_simulator->update(delta_time * m_time_factor);
    }


    void TestComputeShader::onRender(){
        Renderer renderer;
    
        m_camera.input(c_window);
        m_camera.updateMatrix(55.0f, 0.1f, 10000.0f);
    
        // Bind the noise texture
        if (m_noiseTexture)
            m_noiseTexture->bind(0); // Bind to texture slot 0
    
        // Place the cube 
        m_shader.bind();
        m_shader.setUniformVec4f("u_light_color", light_color);
        m_shader.setUniformVec3f("u_light_pos", m_camera.getPosition());
        m_shader.setUniformVec3f("u_cam_pos", m_camera.getPosition());
        m_shader.setUniform1f("u_a", m_quadriatic);
        m_shader.setUniform1f("u_b", m_linear);
        m_shader.setUniform1i("u_noise_texture", 0); // Set texture unit
        m_shader.setUniform1f("u_noise_intensity", m_noise_intensity); // Add noise intensity control
    
        // Draw the cube
        renderer.instancedDraw(m_cube.getVertexArray(), m_cube.getIndexBuffer(), m_shader, m_camera, m_instances);
        
        // Unbind texture
        if (m_noiseTexture)
            m_noiseTexture->unbind();
    }


    void TestComputeShader::onImGuiRender(){
        ImGui::Text("Lightning test");
        ImGui::Text("FPS counter");
        ImGui::SliderFloat("Sensitivity", &m_camera.m_sensitivity, 10.0f, 100.0f);
        
        ImGui::Text("Lightning color");
        ImGui::SliderFloat4("Light color", &light_color.x, 0.0f, 1.0f);
        
        ImGui::Text("Positions");
        
        ImGui::SliderFloat("quadratic", &m_quadriatic, 0.0f, 0.01f);
        ImGui::SliderFloat("linear", &m_linear, 0.0f, 0.01f);
        
        ImGui::SliderFloat("Time factor", &m_time_factor, 0.0f, 100.0f);
        ImGui::Text("Application average %.3f ms/frame (%.1f FPS)", 1000.0f / ImGui::GetIO().Framerate, ImGui::GetIO().Framerate);

        ImGui::Text("Texture Settings");
        ImGui::SliderFloat("Noise Intensity", &m_noise_intensity, 0.0f, 1.0f);
    }
}