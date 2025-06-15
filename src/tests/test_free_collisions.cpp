#include "test_free_collisions.h"
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

    TestFreeCollisions::TestFreeCollisions()
        : m_camera(m_width, m_height, glm::vec3(0.0f, 0.0f, 100.0f)),
        m_noise_intensity(0.0f) {

        //Object distribution grid
        int grid_x = 40;
        int grid_y = 40;
        int grid_z = 40;
        // int grid_x = 100;
        // int grid_y = 100;
        // int grid_z = 100;
        
        float spacing = 5.0f;

        m_instances = grid_x * grid_y * grid_z;


        // Cube vertices
        m_vertices = std::vector<SimpleVertex>(std::begin(CONSTANTS::DODECAHEDRON_MESH_SIMPLE_VERTICES), std::end(CONSTANTS::DODECAHEDRON_MESH_SIMPLE_VERTICES));
    
        m_indices = std::vector<unsigned int>(std::begin(CONSTANTS::DODECAHEDRON_MESH_INDICES), std::end(CONSTANTS::DODECAHEDRON_MESH_INDICES));

        object_vertices = utils::extractPositions(CONSTANTS::DODECAHEDRON_MESH_SIMPLE_VERTICES, std::size(CONSTANTS::DODECAHEDRON_MESH_SIMPLE_VERTICES));
        object_normals = utils::extractNormals  (CONSTANTS::DODECAHEDRON_MESH_SIMPLE_VERTICES, std::size(CONSTANTS::DODECAHEDRON_MESH_SIMPLE_VERTICES));
        object_edges = utils::extractEdges    (CONSTANTS::DODECAHEDRON_MESH_SIMPLE_VERTICES,
                                CONSTANTS::DODECAHEDRON_MESH_INDICES,
                                std::size(CONSTANTS::DODECAHEDRON_MESH_INDICES));

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

        float mass = 1.0f;
        float side = 1.0f;

        // float inertia = (1.0f / 6.0f) * mass * side * side;
        // float inverseInertia = 1.0f / inertia;
        // glm::mat3 inverseTensor = glm::mat3(inverseInertia);

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

            properties[i].velocity = glm::linearRand(glm::vec3(-0.05f), glm::vec3(0.05f));
            properties[i].acceleration = glm::vec3(0.0f);
            properties[i].angular_velocity = glm::linearRand(glm::vec3(-0.1f), glm::vec3(0.1f));
            properties[i].angular_acceleration = glm::vec3(0.0f);

            properties[i].inverseMass = 1.0f;
            properties[i].setInverseInertiaTensor(inverseTensor);
            properties[i].friction = 0.0f;
        }
        
        m_cube.setData(m_vertices, m_indices, *m_model_matrices, *m_colors, m_instances);
        m_shader.setShader("tex_gpu_renderer.glsl");


        m_simulator = new CollisionDetector(
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

    TestFreeCollisions::~TestFreeCollisions(){
        delete m_simulator;
        delete m_model_matrices;

    }

    void TestFreeCollisions::onUpdate(float delta_time){ 
        m_simulator->update(delta_time * m_time_factor);
    }


    void TestFreeCollisions::onRender(){
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


    void TestFreeCollisions::onImGuiRender(){
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