#ifndef TEST_LIGHTS_H
#define TEST_LIGHTS_H

#pragma once

#include "test.h"

#include "../buffers/vertex_buffer.h"
#include "../buffers/index_buffer.h"
#include "../buffers/vertex_array.h"
#include "../buffers/vertex_buffer_layout.h"
#include "../shader.h"
#include "../camera.h"
#include "../meshes/mesh.h"
#include "../meshes/instanced_mesh.h"

namespace test{

    class TestLights : public Test{
    private:
        const unsigned int m_width = 800;
        const unsigned int m_height = 800;

        unsigned int m_instances = 1000000;

        //Add cube pos 
        glm::vec3 m_cube_start_pos = glm::vec3(0.0f, 0.0f, 0.0f);
        glm::vec3 m_light_start_pos = glm::vec3(-2.0f, -2.0f, 2.0f);

        std::vector<glm::mat4>* m_model_matrices;

        glm::vec4 light_color = glm::vec4(1.0f, 1.0f, 1.0f, 1.0f);

        Shader m_shader;
        Shader m_light_shader;

        InstancedMesh m_cube;
        Mesh m_light;

        Camera m_camera;

        std::vector<glm::mat4> m_cube_models;

        float m_a = 0.01f;
        float m_b = 0.01f;

    public:
        TestLights();
        ~TestLights();

        void onUpdate(float deltaTime) override;
        void onRender() override;
        void onImGuiRender() override;
    
    };
}


#endif // TEST_LIGHTS_H