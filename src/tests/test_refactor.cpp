#include "test_refactor.h"
#include "../renderer.h"

#include "imgui.h"
#include "imgui_impl_glfw.h"
#include "imgui_impl_opengl3.h"

#include "glm/glm.hpp"
#include "glm/gtc/matrix_transform.hpp"

#include "../shader.h"

#include "constants.h"

extern GLFWwindow * c_window;

namespace test{

    TestRefactor::TestRefactor()
        : m_camera(m_width, m_height, glm::vec3(0.0f, 0.0f, 3.0f)) {

        //Cube vertices
        std::vector<Vertex> vertices(std::begin(CONSTANTS::SPHERE_MESH_VERTICES), std::end(CONSTANTS::SPHERE_MESH_VERTICES));
        
        std::vector<unsigned int> indices(std::begin(CONSTANTS::SPHERE_MESH_INDICES), std::end(CONSTANTS::SPHERE_MESH_INDICES));



        // m_cube = Mesh(vertices, indices);
        m_cube.setData(vertices, indices);
        m_cube.setPosition(m_cube_start_pos);

        m_shader.setShader("cube.glsl");
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

        // m_cube = Mesh(vertices, indices);
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

    TestRefactor::~TestRefactor(){
    }


    void TestRefactor::onUpdate(float deltaTime){
        // Calculate the rotation angle based on elapsed time
        float angle = 0.01f;
        // std::cout<<angle<<std::endl;

        // Create a quaternion for the rotation around the y-axis
        glm::vec3 axis = glm::vec3(0.0f, 1.0f, 0.0f);
        glm::quat rotation = glm::angleAxis(angle, axis);

        // Apply the rotation to the cube's orientation
        auto applied_rotation = rotation * m_cube.getOrientation();
        m_cube.setOrientation(applied_rotation);
    }



    void TestRefactor::onRender(){
        Renderer renderer;

        m_camera.input(c_window);
        m_camera.updateMatrix(55.0f, 0.1f, 100.0f);

        //Place the cube 
        m_shader.bind();
        m_shader.setUniformVec4f("u_light_color", light_color);
        m_shader.setUniformVec3f("u_light_pos", m_light.getPosition());
        m_shader.setUniform1f("u_a", 0.05f);
        m_shader.setUniform1f("u_b", 0.01f);

        //Place the light
        m_light_shader.bind();
        m_light_shader.setUniformVec4f("u_light_color", light_color);
        
        //Draw the scene
        renderer.draw(m_cube, m_shader, m_camera);
        renderer.draw(m_light, m_light_shader, m_camera);
    }


    void TestRefactor::onImGuiRender(){
        ImGui::Text("Lightning test");
        ImGui::SliderFloat("Sensitivity", &m_camera.m_sensitivity, 10.0f, 100.0f);

        ImGui::Text("Lightning color");
        ImGui::SliderFloat4("Light color", &light_color.x, 0.0f, 1.0f);

        ImGui::Text("Positions");

        ImGui::SliderFloat3("Light Position", &m_light.getPosition().x, -10.0f, 10.0f);
        ImGui::SliderFloat3("Cube Position", &m_cube.getPosition().x, -10.0f, 10.0f);

        ImGui::Text("FPS counter");
        ImGui::Text("Application average %.3f ms/frame (%.1f FPS)", 1000.0f / ImGui::GetIO().Framerate, ImGui::GetIO().Framerate);
    }
}