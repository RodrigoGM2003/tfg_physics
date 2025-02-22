#ifndef CAMERA_H
#define CAMERA_H

#pragma once

#include <GL/glew.h>
#include <GLFW/glfw3.h>

#include "renderer.h"
#include "glm/glm.hpp"
#include "glm/gtc/matrix_transform.hpp"
#include "glm/gtc/type_ptr.hpp"
#include "glm/gtc/type_ptr.hpp"
#include "glm/gtx/rotate_vector.hpp"
#include "glm/gtx/vector_angle.hpp"

#include "shader.h"


class Camera{
private:
    const static float C_DEFAULT_SPEED; //Default speed
    const static float C_DEFAULT_SPRINT_SPEED; //Default sprint speed


    glm::vec3 m_position; //Camera position
    glm::vec3 m_orientation = glm::vec3(0.0f, 0.0f, -1.0f); //Camera orientation
    glm::vec3 m_up = glm::vec3(0.0f, 1.0f, 0.0f); //Camera up vector
    glm::mat4 m_cam_matrix = glm::mat4(1.0f); //Camera matrix

    bool m_first_click = true; //First click flag
 
    int m_width; //Window width
    int m_height; //Window height

public:
    float m_speed = C_DEFAULT_SPEED; //Camera speed
    float m_sensitivity = 50.0f; //Camera sensitivity


    /**
     * @brief Construct a new Camera object
     * @param width Window width
     * @param height Window height
     * @param position Camera position
     */
    Camera(int width, int height, const glm::vec3& position = glm::vec3(0.0f, 0.0f, 0.0f));

    /**
     * @brief Update the camera matrix
     * @param fov Field of view
     * @param near Near plane
     * @param far Far plane
     */
    void updateMatrix(float fov, float near, float far);

    /**
     * @brief Set the camera matrix in the shader
     * @param shader Shader to set the matrix
     * @param uniform Uniform name
     */
    void matrix(Shader& shader, const char* uniform);


    /**
     * @brief Handle camera input
     * @param window GLFW window
     */
    void input(GLFWwindow* window);

    /**
     * @brief Get the camera position
     * @return glm::vec3 Camera position
     */
    inline glm::vec3 getPosition() const { return m_position; }
};


#endif //CAMERA_H