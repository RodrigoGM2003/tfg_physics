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
        : m_camera(m_width, m_height, glm::vec3(0.0f, 0.0f, 3.0f)) {

        // Cube vertices
        m_vertices = std::vector<SimpleVertex>(std::begin(CONSTANTS::CUBE_MESH_SIMPLE_VERTICES), std::end(CONSTANTS::CUBE_MESH_SIMPLE_VERTICES));
    
        m_indices = std::vector<unsigned int>(std::begin(CONSTANTS::CUBE_MESH_INDICES), std::end(CONSTANTS::CUBE_MESH_INDICES));

        // Initialize m_colors with the number of instances
        m_colors = new std::vector<glm::vec4>(m_instances);

        // Generate random colors for each instance
        std::random_device rd;
        std::mt19937 gen(rd());
        std::uniform_real_distribution<float> dist(0.0f, 1.0f);

        for (int i = 0; i < m_instances; i++) {
            m_colors->at(i) = glm::vec4(dist(gen), dist(gen), dist(gen), 1.0f);
        }
            
            
        // Pass these colors to your GpuMesh class
        // std::vector<glm::mat4> model_matrices(m_instances);
        m_model_matrices = new std::vector<glm::mat4>(m_instances);
        
        int grid_x = 100; // Example: 2,000,000 objects
        int grid_y = 100;
        int grid_z = 100;
        
        float spacing = 4.0f;
        
        for (int x = 0; x < grid_x; x++) {
            for (int y = 0; y < grid_y; y++) {
                for (int z = 0; z < grid_z; z++) {
                    glm::mat4 model = glm::mat4(1.0f);
                    model = glm::translate(model, glm::vec3(x * spacing, y * spacing, -z * spacing));
                    m_model_matrices->at(x * grid_y * grid_z + y * grid_z + z) = model;
                }
            }
        }
        
        m_cube.setData(m_vertices, m_indices, *m_model_matrices, *m_colors, m_instances);
        m_shader.setShader("gpu_renderer.glsl");


        m_simulator = new GpuSimulator(m_model_matrices, &m_vertices, &m_indices);

        m_light_shader.setShader("light.glsl");

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
        // const glm::vec3 translation(0.01f, 0.0f, 0.0f);

        // for (int i = 0; i < m_instances; i++) 
        //     m_model_matrices->operator[](i) = glm::translate(m_model_matrices->operator[](i), translation);
        

        m_simulator->update(delta_time * m_time_factor);
        // m_cube.updateModelMatrices();
    }


    void TestComputeShader::onRender(){
        Renderer renderer;

        m_camera.input(c_window);
        m_camera.updateMatrix(55.0f, 0.1f, 1000.0f);

        //Place the cube 
        m_shader.bind();
        m_shader.setUniformVec4f("u_light_color", light_color);
        m_shader.setUniformVec3f("u_light_pos", m_camera.getPosition());
        m_shader.setUniform1f("u_a", m_a);
        m_shader.setUniform1f("u_b", m_b);

        //Place the light
        m_light_shader.bind();
        m_light_shader.setUniformVec4f("u_light_color", light_color);

        //Draw the cube
        renderer.instancedDraw(m_cube.getVertexArray(), m_cube.getIndexBuffer(), m_shader, m_camera, m_instances);
    }


    void TestComputeShader::onImGuiRender(){
        ImGui::Text("Lightning test");
        ImGui::Text("FPS counter");
        ImGui::Text("Application average %.3f ms/frame (%.1f FPS)", 1000.0f / ImGui::GetIO().Framerate, ImGui::GetIO().Framerate);
        ImGui::SliderFloat("Sensitivity", &m_camera.m_sensitivity, 10.0f, 100.0f);

        ImGui::Text("Lightning color");
        ImGui::SliderFloat4("Light color", &light_color.x, 0.0f, 1.0f);

        ImGui::Text("Positions");

        ImGui::SliderFloat("quadratic", &m_a, 0.0f, 0.01f);
        ImGui::SliderFloat("linear", &m_b, 0.0f, 0.01f);

        ImGui::SliderFloat("Time factor", &m_time_factor, 0.0f, 100.0f);
    }
}