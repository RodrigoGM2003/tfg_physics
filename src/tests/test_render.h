#ifndef TEST_RENDER_H
#define TEST_RENDER_H

#pragma once

#include <memory>

#include "test.h"

#include "../buffers/vertex_buffer.h"
#include "../buffers/index_buffer.h"
#include "../buffers/vertex_array.h"
#include "../buffers/vertex_buffer_layout.h"
#include "../shader.h"
#include "../camera.h"
#include "../meshes/mesh.h"
#include "../meshes/gpu_mesh.h"
#include "../simulators/gpu_simulator.h"
#include "../texture.h"

namespace test{

    class TestRender : public Test{
    private:
        const unsigned int m_width = 1920;
        const unsigned int m_height= 1080;

        unsigned int m_instances;
        // unsigned int m_instances = 1000;

        //Add cube pos 
        glm::vec3 m_cube_start_pos = glm::vec3(0.0f, 0.0f, 0.0f);
        glm::vec3 m_light_start_pos = glm::vec3(-2.0f, -2.0f, 2.0f);

        std::vector<glm::mat4>* m_model_matrices;
        std::vector<glm::vec4>* m_colors;

        glm::vec4 light_color = glm::vec4(1.0f ,1.0f, 1.0f, 1.0f);

        Shader m_shader;
        Shader m_light_shader;

        float m_time_factor = 1.0f;
        glm::vec3 m_gravity = glm::vec3(0.0f, -0.1f, 0.0f); 

        std::vector<SimpleVertex> m_vertices;
        std::vector<unsigned int> m_indices;
        std::vector<physics::Properties> properties;
        std::vector<glm::vec4> object_vertices;
        std::vector<glm::vec4> object_normals;
        std::vector<glm::vec4> object_edges;
        GpuMesh m_cube;

        Camera m_camera;

        ShaderStorageBuffer m_transform_ssbo;

        std::vector<glm::mat4> m_cube_models;

        std::unique_ptr<Texture> m_noiseTexture;
        float m_noise_intensity;

        float m_quadriatic = 0.00f;
        float m_linear = 0.00f;

    public:
        TestRender();
        ~TestRender();

        void onUpdate(float deltaTime) override;
        void onRender() override;
        void onImGuiRender() override;
    
    };
}


#endif // TEST_RENDER_H