#include "test_complex_3.h"
#include "../renderer.h"

#include <random>

#include "imgui.h"
#include "imgui_impl_glfw.h"
#include "imgui_impl_opengl3.h"

#include "glm/glm.hpp"
#include "glm/gtc/matrix_transform.hpp"
#include "glm/gtc/random.hpp" 
#include "glm/gtx/component_wise.hpp"

#include "constants.h"

extern GLFWwindow * c_window;

namespace test{

    TestComplex3::TestComplex3()
        : m_camera(m_width, m_height, glm::vec3(0.0f, 0.0f, 100.0f)),
        m_noise_intensity(0.0f) {

        //Object distribution grid
        int grid_x = 10;
        int grid_y = 10;
        int grid_z = 10;
        // int grid_x = 32;
        // int grid_y = 32;
        // int grid_z = 32;
        // int grid_x = 100;
        // int grid_y = 100;
        // int grid_z = 100;
        
        float spacing = 1.0f;

        m_instances = grid_x * grid_y * grid_z + 1;

        // Cube vertices
        m_vertices = std::vector<SimpleVertex>(std::begin(CONSTANTS::CUBE_MESH_SIMPLE_VERTICES), std::end(CONSTANTS::CUBE_MESH_SIMPLE_VERTICES));
    
        m_indices = std::vector<unsigned int>(std::begin(CONSTANTS::CUBE_MESH_INDICES), std::end(CONSTANTS::CUBE_MESH_INDICES));

        object_vertices = utils::extractPositions(CONSTANTS::CUBE_MESH_SIMPLE_VERTICES, std::size(CONSTANTS::CUBE_MESH_SIMPLE_VERTICES));
        object_normals = utils::extractNormals  (CONSTANTS::CUBE_MESH_SIMPLE_VERTICES, std::size(CONSTANTS::CUBE_MESH_SIMPLE_VERTICES));
        object_edges = utils::extractEdges    (CONSTANTS::CUBE_MESH_SIMPLE_VERTICES,
                                CONSTANTS::CUBE_MESH_INDICES,
                                std::size(CONSTANTS::CUBE_MESH_INDICES));

        // Initialize m_colors with the number of instances
        m_colors = new std::vector<glm::vec4>(m_instances, glm::vec4(1.0f));

        // Generate random colors for each instance
        std::random_device rd;
        // static std::mt19937 gen(rd());
        static std::mt19937 gen(42);
        std::uniform_real_distribution<float> dist(0.0f, 1.0f);

        for (int i = 0; i < m_instances; i++) {
            m_colors->at(i) = glm::vec4(dist(gen), dist(gen), dist(gen), 1.0f);
        }
        // for (int i = 0; i < m_instances; i++) {
        //     m_colors->at(i) = glm::vec4(0.2f,0.2f,0.2f, 1.0f);
        // }   

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
        glm::vec3 position = glm::vec3(-20.0f, 0.0f, 0.0f);
        
        model = glm::translate(model, position);              // Then translate
        model = glm::scale(model, scale);                     // Scale first
        
        m_model_matrices->at(m_instances - 1) = model;


        float mass = 1.0f;
        float side = 1.0f;

        float inertia = (1.0f / 6.0f) * mass * side * side;
        float inverseInertia = 1.0f / inertia;

        glm::mat3 inverseTensor = glm::mat3(
            1.0f / inertia, 0.0f, 0.0f,
            0.0f, 1.0f / inertia, 0.0f,
            0.0f, 0.0f, 1.0f / inertia
        );

        properties.resize(m_model_matrices->size());


        std::srand(42);
        for (int i = 0; i < m_model_matrices->size(); i++){
            
            auto transform = &m_model_matrices->at(i);

            properties[i].velocity = glm::vec3(0.0f);
            properties[i].acceleration = glm::vec3(0.0f);
            properties[i].angular_velocity = glm::linearRand(glm::vec3(-0.2f), glm::vec3(0.2f));
            // properties[i].angular_velocity = glm::vec3(0.0f);
            properties[i].angular_acceleration = glm::vec3(0.0f);

            // properties[i].inverseMass = i % 2 == 0 ? 0.0f : 1.0f;
            properties[i].inverseMass = 1.0f;
            properties[i].setInverseInertiaTensor(inverseTensor);
            properties[i].friction = 0.0f;
        }

        properties[m_model_matrices->size() - 1].velocity = glm::vec3(5.0f,0.0f,0.0f);
        properties[m_model_matrices->size() - 1].angular_velocity= glm::linearRand(glm::vec3(-0.2f), glm::vec3(0.2f));
        properties[m_model_matrices->size() - 1].inverseMass = 1.0f/1000.0f;
        properties[m_model_matrices->size() - 1].setInverseInertiaTensor(glm::mat3(0.0f));
        properties[m_model_matrices->size() - 1].friction = 0.0f;


        
        m_cube.setData(m_vertices, m_indices, *m_model_matrices, *m_colors, m_instances);
        m_shader.setShader("tex_gpu_renderer.glsl");

        m_simulator = new GpuSimulator(
            m_model_matrices, 
            &m_vertices, 
            &m_indices,
            &object_vertices,
            &object_normals,
            &object_edges,
            &properties
        );

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

    TestComplex3::~TestComplex3(){
        delete m_simulator;
        delete m_model_matrices;

    }

    void TestComplex3::onUpdate(float delta_time){ 
        m_simulator->update(delta_time * m_time_factor, m_gravity);
    }


    void TestComplex3::onRender(){
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


    void TestComplex3::onImGuiRender(){
        ImGui::Text("Lightning test");
        ImGui::Text("FPS counter");
        ImGui::SliderFloat("Sensitivity", &m_camera.m_sensitivity, 10.0f, 100.0f);
        
        ImGui::Text("Lightning color");
        ImGui::SliderFloat4("Light color", &light_color.x, 0.0f, 1.0f);

        ImGui::Text("Physics");
        ImGui::SliderFloat3("Gravity", &m_gravity.x, -1.0f, 1.0f);

        
        ImGui::SliderFloat("Time factor", &m_time_factor, 0.0f, 100.0f);
        ImGui::Text("Application average %.3f ms/frame (%.1f FPS)", 1000.0f / ImGui::GetIO().Framerate, ImGui::GetIO().Framerate);

        ImGui::Text("Texture Settings");
        ImGui::SliderFloat("Noise Intensity", &m_noise_intensity, 0.0f, 1.0f);
    }
}