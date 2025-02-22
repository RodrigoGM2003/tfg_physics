#ifndef TEST_REFACTOR_H
#define TEST_REFACTOR_H

#pragma once

#include "test.h"

#include "../buffers/vertex_buffer.h"
#include "../buffers/index_buffer.h"
#include "../buffers/vertex_array.h"
#include "../buffers/vertex_buffer_layout.h"
#include "../shader.h"
#include "../camera.h"
#include "../meshes/mesh.h"

namespace test{

    class TestRefactor : public Test{
    private:
        const unsigned int m_width = 800;
        const unsigned int m_height = 800;

        //Add cube pos 
        glm::vec3 m_cube_start_pos = glm::vec3(0.0f, 0.0f, 0.0f);
        glm::vec3 m_light_start_pos = glm::vec3(1.2f, 2.0f, 2.0f);

        glm::vec4 light_color = glm::vec4(1.0f, 1.0f, 1.0f, 1.0f);

        Shader m_shader;
        Shader m_light_shader;

        Mesh m_cube;
        Mesh m_light;

        Camera m_camera;

    public:
        TestRefactor();
        ~TestRefactor();

        void onUpdate(float deltaTime) override;
        void onRender() override;
        void onImGuiRender() override;
    
    };
}


#endif // TEST_REFACTOR_H