#ifndef TEST_COMPUTE_SADER_H
#define TEST_COMPUTE_SADER_H

#pragma once

#include "test.h"

#include "../buffers/vertex_buffer.h"
#include "../buffers/index_buffer.h"
#include "../buffers/vertex_array.h"
#include "../buffers/vertex_buffer_layout.h"
#include "../shader.h"
#include "../camera.h"
#include "../meshes/mesh.h"
#include "../simulators/gpu_simulator.h"

namespace test{

    class TestComputeShader : public Test{
    private:
        const unsigned int m_width = 800;
        const unsigned int m_height = 800;

        unsigned int m_instances = 500000;

        //Add cube pos 
        glm::vec3 m_cube_start_pos = glm::vec3(0.0f, 0.0f, 0.0f);
        glm::vec3 m_light_start_pos = glm::vec3(-2.0f, -2.0f, 2.0f);

        std::vector<glm::mat4>* m_model_matrices;

        glm::vec4 light_color = glm::vec4(0.595f, 0.558f, 0.429f, 1.0f);

        Shader m_shader;
        Shader m_light_shader;

        float m_time_factor = 1.0f;

        std::vector<Vertex> m_vertices;
        std::vector<unsigned int> m_indices;
        InstancedMesh m_cube;
        Mesh m_light;

        Camera m_camera;

        std::vector<glm::mat4> m_cube_models;

        Simulable* m_simulator;

        float m_a = 0.01f;
        float m_b = 0.01f;

    public:
        TestComputeShader();
        ~TestComputeShader();

        void onUpdate(float deltaTime) override;
        void onRender() override;
        void onImGuiRender() override;
    
    };
}


#endif // TEST_COMPUTE_SADER_H