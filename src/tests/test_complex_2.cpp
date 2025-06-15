#include "test_complex_2.h"
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

    TestComplex2::TestComplex2()
        : m_camera(m_width, m_height, glm::vec3(0.0f, 0.0f, 100.0f)),
        m_noise_intensity(0.0f) {

        //Object distribution grid
        int grid_x = 10;
        int grid_y = 10;
        int grid_z = 10;
        

        m_instances = grid_x * grid_y * grid_z;

        // Cube vertices
        m_vertices = std::vector<SimpleVertex>(std::begin(CONSTANTS::OCTAHEDRON_MESH_SIMPLE_VERTICES), std::end(CONSTANTS::OCTAHEDRON_MESH_SIMPLE_VERTICES));
    
        m_indices = std::vector<unsigned int>(std::begin(CONSTANTS::OCTAHEDRON_MESH_INDICES), std::end(CONSTANTS::OCTAHEDRON_MESH_INDICES));

        object_vertices = utils::extractPositions(CONSTANTS::OCTAHEDRON_MESH_SIMPLE_VERTICES, std::size(CONSTANTS::OCTAHEDRON_MESH_SIMPLE_VERTICES));
        object_normals = utils::extractNormals  (CONSTANTS::OCTAHEDRON_MESH_SIMPLE_VERTICES, std::size(CONSTANTS::OCTAHEDRON_MESH_SIMPLE_VERTICES));
        object_edges = utils::extractEdges    (CONSTANTS::OCTAHEDRON_MESH_SIMPLE_VERTICES,
                                CONSTANTS::OCTAHEDRON_MESH_INDICES,
                                std::size(CONSTANTS::OCTAHEDRON_MESH_INDICES));

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

        // Blob parameters
        int half        = m_instances / 2;
        float blobRadius = 50.0f;
        float minSpacing = 2.5f;          // minimum distance between any two centers
        glm::vec3 leftCenter   = glm::vec3(-60.0f, 0.0f, 0.0f);
        glm::vec3 rightCenter  = glm::vec3( 60.0f, 0.0f, 0.0f);
        float approachSpeed    = 10.0f;

        // Prepare model matrices
        m_model_matrices = new std::vector<glm::mat4>(m_instances);

        // A simple uniform‐in‐sphere sampler
        std::uniform_real_distribution<float> distSphere(-1.0f, 1.0f);
        auto randomInSphere = [&](float radius) {
            glm::vec3 p;
            do {
                p = glm::vec3(distSphere(gen), distSphere(gen), distSphere(gen));
            } while (glm::length2(p) > 1.0f);
            // scale uniformly so distribution is volume‐correct
            float r = std::cbrt(glm::compAdd(glm::abs(p) / radius)) * radius;
            return glm::normalize(p) * r;
        };

        // —— REPLACE YOUR OLD “Scatter left/right blob” LOOPS WITH THIS: ——

        // 1) scatter left blob with strict min‐spacing
        std::vector<glm::vec3> placed;
        placed.reserve(half);
        for (int i = 0; i < half; ++i) {
            glm::vec3 candidate;
            // loop until we find a spot at least minSpacing from all placed
            do {
                candidate = leftCenter + randomInSphere(blobRadius);
            } while (std::any_of(placed.begin(), placed.end(),
                    [&](const glm::vec3 &other) {
                        return glm::distance2(candidate, other) < minSpacing * minSpacing;
                    }));
            placed.push_back(candidate);
            (*m_model_matrices)[i] = glm::translate(glm::mat4(1.0f), candidate);
        }

        // 2) scatter right blob the same way
        placed.clear();
        placed.reserve(m_instances - half);
        for (int i = 0; i < m_instances - half; ++i) {
            glm::vec3 candidate;
            do {
                candidate = rightCenter + randomInSphere(blobRadius);
            } while (std::any_of(placed.begin(), placed.end(),
                    [&](const glm::vec3 &other) {
                        return glm::distance2(candidate, other) < minSpacing * minSpacing;
                    }));
            placed.push_back(candidate);
            (*m_model_matrices)[half + i] = glm::translate(glm::mat4(1.0f), candidate);
        }




        float mass = 1.0f;
        float side = 1.0f;

        float inertia = (1.0f / 6.0f) * mass * side * side;
        float inverseInertia = 1.0f / inertia;

        glm::mat3 inverseTensor = glm::mat3(
            1.0f / inertia, 0.0f, 0.0f,
            0.0f, 1.0f / inertia, 0.0f,
            0.0f, 0.0f, 1.0f / inertia
        );

        // --- now set up your physics properties exactly as before ---
        properties.resize(m_model_matrices->size());
        for (int i = 0; i < properties.size(); ++i) {
            properties[i].velocity              = glm::vec3(0.0f);
            properties[i].acceleration          = glm::vec3(0.0f);
            properties[i].angular_velocity      = glm::linearRand(glm::vec3(-0.2f), glm::vec3(0.2f));
            properties[i].angular_acceleration  = glm::vec3(0.0f);
            properties[i].inverseMass           = 1.0f;
            properties[i].setInverseInertiaTensor(inverseTensor);
            properties[i].friction              = 0.0f;
        }

        // Give the blobs their approach velocities
        for (int i = 0; i < half; ++i) {
            // left blob moves toward right cluster
            glm::vec3 pos = glm::vec3(m_model_matrices->at(i)[3]); // extract translation
            glm::vec3 dir = glm::normalize(rightCenter - leftCenter);
            properties[i].velocity = dir * approachSpeed;
        }

        for (int i = half; i < m_instances; ++i) {
            // right blob moves toward left cluster
            glm::vec3 pos = glm::vec3(m_model_matrices->at(i)[3]);
            glm::vec3 dir = glm::normalize(leftCenter - rightCenter);
            properties[i].velocity = dir * approachSpeed;
        }

        // …then finish by uploading to the GPU, etc., just like before:
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

    TestComplex2::~TestComplex2(){
        delete m_simulator;
        delete m_model_matrices;

    }

    void TestComplex2::onUpdate(float delta_time){ 
        m_simulator->update(delta_time * m_time_factor, m_gravity);
    }


    void TestComplex2::onRender(){
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


    void TestComplex2::onImGuiRender(){
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