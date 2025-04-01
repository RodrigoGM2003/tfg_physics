#include "glincludes.h"

#include <cassert>  // 'assert' (enforce preconditions)
#include <cstring>  // 'strlen' (to compile shaders)
#include <iostream> // 'cout' and such
#include <string>
#include <sys/stat.h>
#include <sys/types.h>
#include <direct.h>


#include "glm/glm.hpp"
#include "glm/gtc/matrix_transform.hpp"
#include "imgui.h"
#include "imgui_impl_glfw.h"
#include "imgui_impl_opengl3.h"

#include "renderer.h"
#include "vertex_buffer.h"
#include "index_buffer.h"
#include "vertex_array.h"
#include "vertex_buffer_layout.h"
#include "shader.h"

#include "tests/test_refactor.h"
#include "tests/test_multiple.h"
#include "tests/test_lights.h"
#include "tests/test_compute_shader.h"



//Global variables
GLFWwindow * c_window = nullptr; /*Main Window*/

int
    w_width  = 1920, /*Window width*/
    w_height = 1080 ; /*Window heigth*/

bool terminate_program = false; /*Program termination*/

unsigned int samples = 4; /*Number of samples*/

/**
 * @brief Initializes GLFW
 */
void initGLFW(int argc, char * argv[]){
    //Check for errors initializing GLFW
    if ( !glfwInit() ){
        std::cerr << "Error when initializing GLFW" << std::endl ;
        exit(1);
    }

    glfwWindowHint( GLFW_CONTEXT_VERSION_MAJOR, 4 );
    glfwWindowHint( GLFW_CONTEXT_VERSION_MINOR, 4 );
    glfwWindowHint( GLFW_OPENGL_FORWARD_COMPAT, GLFW_TRUE ); 
    glfwWindowHint( GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE );
    glfwWindowHint( GLFW_SAMPLES, samples );

    //Create window
    c_window = glfwCreateWindow( w_width, w_height, "Hello World", NULL, NULL );
    
    //Check for errors creating window
    if ( !c_window ){
        std::cerr << "Error when creating window" << std::endl ;
        glfwTerminate();
        exit(1);
    }

    //Make the context current
    glfwMakeContextCurrent( c_window );

    glfwSwapInterval(1); //VSync

    //Get the framebuffer size
    glfwGetFramebufferSize(c_window, &w_width, &w_height);

    // CON ESTO GESTION DE EVENTOS
//     glfwSetFramebufferSizeCallback( ventana_glfw, FGE_CambioTamano );
//     glfwSetKeyCallback            ( ventana_glfw, FGE_PulsarLevantarTecla );
//     glfwSetMouseButtonCallback    ( ventana_glfw, FGE_PulsarLevantarBotonRaton);
//     glfwSetCursorPosCallback      ( ventana_glfw, FGE_MovimientoRaton );
//     glfwSetScrollCallback         ( ventana_glfw, FGE_Scroll );
}

/**
 * @brief Initializes GLEW
 */
void initGLEW(){
    // Initialize GLEW
    GLenum codigoError = glewInit();

    // Check for errors
    if ( codigoError != GLEW_OK )
    {
        std::cout << "Imposible to initialize GLEW: " << std::endl
             << (const char *)glewGetErrorString( codigoError ) << std::endl ;
        exit(1);
    }
    else
        std::cout << "GLEW initialized succesfully" << std::endl << std::flush ;
}

/**
 * @brief Initializes OpenGL
 */
void initOpenGL(){
    // Initialize GLEW
    initGLEW();

    //Background color
    GLCall(glClearColor( 0.0, 0.0, 0.0, 1.0 )); 
    
    GLCall(glEnable(GL_MULTISAMPLE));

    // //Enable blending
    // GLCall(glEnable(GL_BLEND));

    //Draw all triangles
    GLCall(glDisable( GL_CULL_FACE ));
}

/**
 * @brief Simulator's main loop
 */
void mainLoop(){
    Renderer renderer;

    ImGui::CreateContext();
    ImGuiIO& io = ImGui::GetIO(); (void)io;
    ImGui_ImplGlfw_InitForOpenGL(c_window, true);
    ImGui_ImplOpenGL3_Init();
    ImGui::StyleColorsDark();

    test::Test* current_test = nullptr;
    test::TestMenu* test_menu = new test::TestMenu(current_test);
    current_test = test_menu;
    test_menu->registerTest<test::TestRefactor>("Refactor");
    test_menu->registerTest<test::TestMultiple>("Multiple");
    test_menu->registerTest<test::TestLights>("Lights");
    test_menu->registerTest<test::TestComputeShader>("ComputeShader");


    double last_frame_time = glfwGetTime();
    float delta_time = 0.0f;

    //Check for program termination
    while ( !terminate_program )
    {
        double current_frame_time = glfwGetTime();
        delta_time = static_cast<float>(current_frame_time - last_frame_time);
        last_frame_time = current_frame_time;

        // Cap delta time to prevent "spiral of death" with very low framerates
        if (delta_time > 0.25f)
            delta_time = 0.25f;

        //Set the viewport
        // GLCall(glViewport(0, 0, w_width, w_height));
        GLCall(glClearColor(0.0f, 0.0f, 0.0f, 1.0f));
        renderer.clear();


        ImGui_ImplGlfw_NewFrame();
        ImGui_ImplOpenGL3_NewFrame();
        ImGui::NewFrame();

        if (current_test){
            current_test->onUpdate(delta_time);
            current_test->onRender();
            ImGui::Begin("Test");
            if (current_test != test_menu && ImGui::Button("<-")){
                delete current_test;
                current_test = test_menu;
            }
            current_test->onImGuiRender();
            ImGui::End();
        }

        ImGui::Render();
        ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData()); // ERROR


        //Swap buffers and poll events
        glfwSwapBuffers( c_window );
        glfwPollEvents();

        //Check for program termination
        terminate_program = glfwWindowShouldClose( c_window ) || terminate_program;
    }

    delete current_test;
    if (current_test != test_menu)
        delete test_menu;
}

int main( int argc, char *argv[] )
{
    //Initialize GLFW
    initGLFW(argc, argv);

    //Initialize OpenGL
    initOpenGL();

    const GLubyte* renderer = glGetString(GL_RENDERER); // Get renderer string
    const GLubyte* version = glGetString(GL_VERSION); // Get version string
    std::cout << "Renderer: " << renderer << std::endl;
    std::cout << "OpenGL version supported: " << version << std::endl;

    //Main loop
    mainLoop();

    //Terminate ImGui
    ImGui_ImplOpenGL3_Shutdown();
    ImGui_ImplGlfw_Shutdown();
    ImGui::DestroyContext();

    //Terminate GLFW
    glfwTerminate();
}
