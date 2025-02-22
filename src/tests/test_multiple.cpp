#include "test_multiple.h"
#include "../renderer.h"

#include "imgui.h"
#include "imgui_impl_glfw.h"
#include "imgui_impl_opengl3.h"

#include "glm/glm.hpp"
#include "glm/gtc/matrix_transform.hpp"

#include <omp.h>

extern GLFWwindow * c_window;

namespace test{

    TestMultiple::TestMultiple()
        : m_camera(m_width, m_height, glm::vec3(0.0f, 0.0f, 3.0f)) {

        //Cube vertices
        std::vector<Vertex> vertices = {
            {{-0.5f, -0.5f, -0.5f},     {1, 1, 1, 1},      {0, -1, 0}}, // - Y face
            {{ 0.5f, -0.5f, -0.5f},     {1, 1, 1, 1},      {0, -1, 0}},
            {{ 0.5f, -0.5f,  0.5f},     {1, 1, 1, 1},      {0, -1, 0}},
            {{-0.5f, -0.5f,  0.5f},     {1, 1, 1, 1},      {0, -1, 0}},

            {{-0.5f,  0.5f, -0.5f},     {1, 1, 1, 1},      {0, 1, 0}},  // + Y face
            {{ 0.5f,  0.5f, -0.5f},     {1, 1, 1, 1},      {0, 1, 0}},
            {{ 0.5f,  0.5f,  0.5f},     {1, 1, 1, 1},      {0, 1, 0}},
            {{-0.5f,  0.5f,  0.5f},     {1, 1, 1, 1},      {0, 1, 0}},

            {{-0.5f, -0.5f, -0.5f},     {1, 1, 1, 1},      {-1, 0, 0}}, // - X face
            {{-0.5f,  0.5f, -0.5f},     {1, 1, 1, 1},      {-1, 0, 0}},
            {{-0.5f,  0.5f,  0.5f},     {1, 1, 1, 1},      {-1, 0, 0}},
            {{-0.5f, -0.5f,  0.5f},     {1, 1, 1, 1},      {-1, 0, 0}},

            {{0.5f, -0.5f, -0.5f},      {1, 1, 1, 1},      {1, 0, 0}}, // + X face
            {{0.5f,  0.5f, -0.5f},      {1, 1, 1, 1},      {1, 0, 0}},
            {{0.5f,  0.5f,  0.5f},      {1, 1, 1, 1},      {1, 0, 0}},
            {{0.5f, -0.5f,  0.5f},      {1, 1, 1, 1},      {1, 0, 0}},

            {{-0.5f, -0.5f, -0.5f},     {1, 1, 1, 1},      {0, 0, -1}}, // - Z face
            {{-0.5f,  0.5f, -0.5f},     {1, 1, 1, 1},      {0, 0, -1}},
            {{0.5f,  0.5f, -0.5f},      {1, 1, 1, 1},      {0, 0, -1}},
            {{0.5f, -0.5f, -0.5f},      {1, 1, 1, 1},      {0, 0, -1}},

            {{-0.5f, -0.5f,  0.5f},     {1, 1, 1, 1},      {0, 0, 1}}, // + Z face
            {{-0.5f,  0.5f,  0.5f},     {1, 1, 1, 1},      {0, 0, 1}},
            {{ 0.5f,  0.5f,  0.5f},     {1, 1, 1, 1},      {0, 0, 1}},
            {{ 0.5f, -0.5f,  0.5f},     {1, 1, 1, 1},      {0, 0, 1}}
        };
        std::vector<unsigned int> indices = { 
            0, 1, 2, 2, 3, 0, // - Y face
            4, 6, 5, 4, 7, 6, // + Y face
            8, 10, 9, 8, 11, 10, // - X face
            12, 13, 14, 12, 14, 15, // + X face
            16, 17, 18, 16, 18, 19, // - Z face
            20, 22, 21, 20, 23, 22  // + Z face
        };

        m_instances = 1000000;
        // std::vector<glm::mat4> model_matrices(m_instances);
        m_model_matrices = new std::vector<glm::mat4>(m_instances);

        // Make the cubes appear in a 3D grid
        for (int x = 0; x < 100; x++) {
            for (int y = 0; y < 100; y++) {
                for (int z = 0; z < 100; z++) {
                    glm::mat4 model = glm::mat4(1.0f);
                    model = glm::translate(model, glm::vec3(x * 4.0f, y * 4.0f, -z * 4.0f));
                    m_model_matrices->at(x * 10000 + y * 100 + z) = model;
                }
            }
        }
        
        m_cube.setData(vertices, indices, *m_model_matrices, m_instances);

        m_shader.setShader("multiple_cube.glsl");
        m_shader.bind();
        m_shader.setUniformVec4f("u_light_color", light_color);
        m_shader.unbind();

        std::vector<Vertex> light_vertices = {
            {{-0.1f, -0.1f, -0.1f},     {0, 0, 0, 0},      {0, 0, 0}},   //0
            {{ 0.1f, -0.1f, -0.1f},     {0, 0, 0, 0},      {0, 0, 0}},   //1
            {{ 0.1f, -0.1f,  0.1f},     {0, 0, 0, 0},      {0, 0, 0}},   //0
            {{-0.1f, -0.1f,  0.1f},     {0, 0, 0, 0},      {0, 0, 0}},   //0

            {{-0.1f,  0.1f, -0.1f},     {0, 0, 0, 0},      {0, 0, 0}},   //0
            {{ 0.1f,  0.1f, -0.1f},     {0, 0, 0, 0},      {0, 0, 0}},   //0
            {{ 0.1f,  0.1f,  0.1f,},    {0, 0, 0, 0},      {0, 0, 0}},   //0
            {{-0.1f,  0.1f,  0.1f},     {0, 0, 0, 0},      {0, 0, 0}}   //0
        };
        std::vector<unsigned int> light_indices = { 
            0, 1, 2,
            2, 3, 0,
            0, 5, 1,
            0, 4, 5,
            1, 6, 2,
            1, 5, 6,
            2, 6, 3,
            3, 6, 7,
            0, 7, 4,
            0, 3, 7,
            4, 6, 5,
            4, 7, 6
        };


        m_light.setData(light_vertices, light_indices);
        m_light.setPosition(m_light_start_pos);

        m_light_shader.setShader("light.glsl");
        m_light_shader.bind();
        m_light_shader.setUniformVec4f("u_light_color", light_color);

        GLCall(glViewport(0, 0, m_width, m_height));
        GLCall(glClearColor(0.0f, 0.0f, 0.0f, 1.0f));
        GLCall(glEnable(GL_DEPTH_TEST));

        GLCall(glEnable(GL_CULL_FACE));
        GLCall(glCullFace(GL_BACK));
        GLCall(glFrontFace(GL_CCW));

    }

    TestMultiple::~TestMultiple(){
    }


    void TestMultiple::onUpdate(float deltaTime){
        // const glm::vec3 translation(0.01f, 0.0f, 0.0f);
        // int size = m_model_matrices->size();

        //Move all the cubes to the right
        // #pragma omp parallel for
        // for (int i = 0; i < size; i++) {
        //     m_model_matrices->operator[](i) = glm::translate(m_model_matrices->operator[](i), translation);
        // }
        // for (int i = 0; i < 1000000; i++) {
        //     m_model_matrices->operator[](i) = glm::translate(m_model_matrices->operator[](i), translation);
        // }

        // m_cube.updateModelMatrices(*m_model_matrices);
    }



    void TestMultiple::onRender(){
        Renderer renderer;

        m_camera.input(c_window);
        m_camera.updateMatrix(55.0f, 0.1f, 1000.0f);

        //Place the cube 
        m_shader.bind();
        m_shader.setUniformVec4f("u_light_color", light_color);
        m_shader.setUniformVec3f("u_light_pos", m_light.getPosition());
        m_shader.setUniform1f("u_a", m_a);
        m_shader.setUniform1f("u_b", m_b);

        //Place the light
        m_light_shader.bind();
        m_light_shader.setUniformVec4f("u_light_color", light_color);

        //Draw the scene
        renderer.instancedDraw(m_cube, m_shader, m_camera, m_instances);
        renderer.draw(m_light, m_light_shader, m_camera);
    }


    void TestMultiple::onImGuiRender(){
        ImGui::Text("Lightning test");
        ImGui::Text("FPS counter");
        ImGui::Text("Application average %.3f ms/frame (%.1f FPS)", 1000.0f / ImGui::GetIO().Framerate, ImGui::GetIO().Framerate);
        ImGui::SliderFloat("Sensitivity", &m_camera.m_sensitivity, 10.0f, 100.0f);

        ImGui::Text("Lightning color");
        ImGui::SliderFloat4("Light color", &light_color.x, 0.0f, 1.0f);

        ImGui::Text("Positions");

        ImGui::SliderFloat3("Light Position", &m_light.getPosition().x, -10.0f, 150.0f);

        ImGui::SliderFloat("a", &m_a, 0.0f, 0.01f);
        ImGui::SliderFloat("b", &m_b, 0.0f, 0.01f);
    }
}