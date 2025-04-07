#include "camera.h"


const float Camera::C_DEFAULT_SPEED = 0.1f;
const float Camera::C_DEFAULT_SPRINT_SPEED = 2.0f;

Camera::Camera(int width, int height, const glm::vec3& position)
    : m_position(position), m_width(width), m_height(height) {}

void Camera::updateMatrix(float fov, float near, float far){
    //Create the view and projection matrices
    glm::mat4 view = glm::lookAt(m_position, m_position + m_orientation, m_up);
    glm::mat4 proj = glm::perspective(glm::radians(fov), (float)m_width/m_height, near, far);

    //Set the matrix
    m_cam_matrix = proj * view;
    
}

void Camera::matrix(Shader& shader, const char* uniform){
    //Set the matrix in the shader
    shader.bind();
    shader.setUniformMat4f(uniform, m_cam_matrix);
}

void Camera::input(GLFWwindow *window){
    // W -> Move forward
    if (glfwGetKey(window, GLFW_KEY_W) == GLFW_PRESS)
        m_position += m_speed * m_orientation;

    // S -> Move backward
    if (glfwGetKey(window, GLFW_KEY_S) == GLFW_PRESS)
        m_position -= m_speed * m_orientation;

    // A -> Move left
    if (glfwGetKey(window, GLFW_KEY_A) == GLFW_PRESS)
        m_position -= glm::normalize(glm::cross(m_orientation, m_up)) * m_speed;

    // D -> Move right
    if (glfwGetKey(window, GLFW_KEY_D) == GLFW_PRESS)
        m_position += glm::normalize(glm::cross(m_orientation, m_up)) * m_speed;

    // Space -> Move up
    if (glfwGetKey(window, GLFW_KEY_SPACE) == GLFW_PRESS)
        m_position += m_speed * m_up;

    // Left Control -> Move down
    if (glfwGetKey(window, GLFW_KEY_LEFT_CONTROL) == GLFW_PRESS)
        m_position -= m_speed * m_up;

    // Left Shift -> "Sprint"
    if (glfwGetKey(window, GLFW_KEY_LEFT_SHIFT) == GLFW_PRESS)
        m_speed = C_DEFAULT_SPRINT_SPEED;

    // Release Left Shift -> Normal speed
    else if (glfwGetKey(window, GLFW_KEY_LEFT_SHIFT) == GLFW_RELEASE)
        m_speed = C_DEFAULT_SPEED;

    // Right Click -> Rotate camera
    if (glfwGetMouseButton(window, GLFW_MOUSE_BUTTON_3) == GLFW_PRESS){
        //Disable the mouse
        glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_DISABLED);
        
        //Check if its the first click
        if (m_first_click){
            glfwSetCursorPos(window, m_width / 2, m_height / 2);
            m_first_click = false;
        }

        //Get the mouse position
        double mouse_x, mouse_y;
        glfwGetCursorPos(window, &mouse_x, &mouse_y);

        //Calculate the rotation
        float rot_x = m_sensitivity * (float)(mouse_y - (m_height / 2)) / m_height;
        float rot_y = m_sensitivity * (float)(mouse_x - (m_width / 2)) / m_width;

        //Calculate the new camera orientation applying the rotation
        glm::vec3 new_orientation = glm::rotate(m_orientation, glm::radians(-rot_x), glm::normalize(glm::cross(m_orientation, m_up)));

        //Prevent a barrelroll from happening
        if (!(glm::angle(new_orientation, m_up) <= glm::radians(5.0f) || glm::angle(new_orientation, -m_up) <= glm::radians(5.0f)))
            m_orientation = new_orientation;

        //Apply the rotation on the y axis
        m_orientation = glm::rotate(m_orientation, glm::radians(-rot_y), m_up);

        //Center the mouse
        glfwSetCursorPos(window, m_width / 2, m_height / 2);
    }

    // Release Right Click -> Enable the mouse
    else if (glfwGetMouseButton(window, GLFW_MOUSE_BUTTON_3) == GLFW_RELEASE){
        glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_NORMAL);
        m_first_click = true;
    }
}
