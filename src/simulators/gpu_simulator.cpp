#include "gpu_simulator.h"

#include <iostream>
#include <chrono>

#include "glm/gtc/matrix_transform.hpp" 
#include "glm/gtx/transform.hpp"
#include "glm/glm.hpp"
#include "glm/gtc/quaternion.hpp"
#include "glm/gtc/random.hpp" 
#include "glm/gtx/component_wise.hpp"
#include "../constants.h"
#include "../utils.h"


GpuSimulator::GpuSimulator(
    std::vector<glm::mat4>* transforms, 
    const std::vector<SimpleVertex>* static_vertices, 
    const std::vector<unsigned int>* static_indices,
    const std::vector<glm::vec4>* object_vertices,
    const std::vector<glm::vec4>* object_normals, 
    const std::vector<glm::vec4>* object_edges,
    std::vector<physics::Properties>* properties
) : sim_transforms(transforms), 
    sim_static_vertices(static_vertices), 
    sim_static_indices(static_indices),
    m_object_vertices(object_vertices),
    m_object_normals(object_normals),
    m_object_edges(object_edges),
    sim_properties(properties)
    {

    initializeData();

    std::cout<<"number of objects: "<<sim_transforms->size()<<std::endl;
    //Transform update shader
    m_transform_shader.setShader("sphere_transforms.glsl");
    m_transform_shader.useTimer(false);
    m_transform_shader.bind();
    
    //Broad phase shader
    m_broad_phase_shader.setShader("collision_naive.glsl");
    m_broad_phase_shader.useTimer(false);
    m_broad_phase_shader.bind();

    //Narrow phase shader
    m_narrow_phase_shader.setShader("narrow_working.glsl");
    // m_narrow_phase_shader.setShader("narrow_rotation.glsl");
    m_narrow_phase_shader.useTimer(false);
    m_narrow_phase_shader.bind();
    
    //Resolution phase shaders
    m_impulse_phase_shader.setShader("jacobi_friction_impulse.glsl");
    // m_impulse_phase_shader.setShader("jacobi_rotation_impulse.glsl");
    // m_impulse_phase_shader.setShader("constraint_rotation.glsl");
    m_impulse_phase_shader.useTimer(false);
    m_impulse_phase_shader.bind();
    
    m_accumulation_phase_shader.setShader("accumulator.glsl");
    // m_accumulation_phase_shader.setShader("accumulator_rotation.glsl");
    m_accumulation_phase_shader.useTimer(false);
    m_accumulation_phase_shader.bind();


    //SSBOs
    m_transform_ssbo.setBuffer(sim_transforms->data(), sim_transforms->size() * sizeof(glm::mat4), GL_DYNAMIC_DRAW);
    m_transform_ssbo.unbind();

    m_properties_ssbo.setBuffer(sim_properties->data(), sim_properties->size() * sizeof(physics::Properties), GL_DYNAMIC_DRAW);
    m_properties_ssbo.unbind();

    m_results_ssbo.setBuffer(nullptr, sim_transforms->size() * sizeof(int), GL_DYNAMIC_DRAW);
    m_results_ssbo.unbind();

    m_spheres_ssbo.setBuffer(sim_spheres.data(), sim_spheres.size() * sizeof(glm::vec4), GL_DYNAMIC_DRAW);
    m_spheres_ssbo.unbind();

    m_collision_pair_ssbo.setBuffer(nullptr, sim_spheres.size() * sim_spheres.size() * sizeof(glm::ivec2) , GL_DYNAMIC_DRAW);
    m_collision_pair_ssbo.unbind();

    m_collision_count_ssbo.setBuffer(nullptr, sizeof(unsigned int), GL_DYNAMIC_DRAW);
    m_collision_count_ssbo.unbind();

    m_second_results_ssbo.setBuffer(nullptr, sim_transforms->size() * sizeof(int), GL_DYNAMIC_DRAW);
    m_second_results_ssbo.unbind();

    m_object_vertices_ssbo.setBuffer(m_object_vertices->data(), m_object_vertices->size() * sizeof(glm::vec4), GL_STATIC_DRAW);
    m_object_vertices_ssbo.unbind();

    m_object_normals_ssbo.setBuffer(m_object_normals->data(), m_object_normals->size() * sizeof(glm::vec4), GL_STATIC_DRAW);
    m_object_normals_ssbo.unbind();

    m_object_edges_ssbo.setBuffer(m_object_edges->data(), m_object_edges->size() * sizeof(glm::vec4), GL_STATIC_DRAW);
    // m_object_edges_ssbo.setBuffer(m_object_edges.data(), m_object_edges.size() * sizeof(unsigned int), GL_STATIC_DRAW);
    m_object_edges_ssbo.unbind();

    std::vector<physics::ContactManifold> manifolds(sim_spheres.size() * sim_spheres.size(), physics::ContactManifold());
    std::cout << "sizeof(ContactManifold): " << sizeof(physics::ContactManifold) << std::endl;
    m_contact_manifolds_ssbo.setBuffer(manifolds.data(), sim_spheres.size() * sim_spheres.size() * sizeof(physics::ContactManifold), GL_DYNAMIC_DRAW);
    m_contact_manifolds_ssbo.unbind();
    
    // //---------------------------------
    // m_normal_impulses_ssbo.setBuffer(m_normal_impulses.data(), sim_spheres.size() * sizeof(float) * 2, GL_DYNAMIC_DRAW);
    // m_normal_impulses_ssbo.unbind();

    // m_tangent_impulses_ssbo.setBuffer(m_tangent_impulses.data(), sim_spheres.size() * sizeof(glm::vec3) * 2, GL_DYNAMIC_DRAW);
    // m_tangent_impulses_ssbo.unbind();

    std::vector<glm::vec4> deltaV(sim_transforms->size(), glm::vec4(0.0f));
    m_deltaV_ssbo.setBuffer(deltaV.data(), sim_transforms->size() * sizeof(glm::vec4), GL_DYNAMIC_DRAW);
    m_deltaV_ssbo.unbind();

    std::vector<glm::vec4> deltaW(sim_transforms->size(), glm::vec4(0.0f));
    m_deltaW_ssbo.setBuffer(deltaW.data(), sim_transforms->size() * sizeof(glm::vec4), GL_DYNAMIC_DRAW);
    m_deltaW_ssbo.unbind();

    std::vector<float> lambdas(sim_transforms->size(), 0.0f);
    m_lambdas_ssbo.setBuffer(lambdas.data(), sim_transforms->size() * sizeof(float), GL_DYNAMIC_DRAW);
    m_lambdas_ssbo.unbind();

    std::vector<float> new_lambdas(sim_transforms->size(), 0.0f);
    m_new_lambdas_ssbo.setBuffer(new_lambdas.data(), sim_transforms->size() * sizeof(float), GL_DYNAMIC_DRAW);
    m_new_lambdas_ssbo.unbind();

    // m_delta_angular_impulses_ssbo.setBuffer(m_delta_angular_impulses.data(), sim_transforms->size() * sizeof(glm::vec3), GL_DYNAMIC_DRAW);
    // m_delta_angular_impulses_ssbo.unbind();
    //---------------------------------


    m_transform_ssbo.bindToBindingPoint(1);
    m_properties_ssbo.bindToBindingPoint(5);
    m_results_ssbo.bindToBindingPoint(4);
    m_spheres_ssbo.bindToBindingPoint(7);

    m_collision_pair_ssbo.bindToBindingPoint(20);
    m_collision_count_ssbo.bindToBindingPoint(21);
    m_second_results_ssbo.bindToBindingPoint(22);

    m_object_vertices_ssbo.bindToBindingPoint(23);
    m_object_normals_ssbo.bindToBindingPoint(24);
    m_object_edges_ssbo.bindToBindingPoint(25);
    m_contact_manifolds_ssbo.bindToBindingPoint(26);

    m_normal_impulses_ssbo.bindToBindingPoint(27);
    m_tangent_impulses_ssbo.bindToBindingPoint(28);
    m_deltaV_ssbo.bindToBindingPoint(29);
    m_deltaW_ssbo.bindToBindingPoint(30);
    m_lambdas_ssbo.bindToBindingPoint(31);
    m_new_lambdas_ssbo.bindToBindingPoint(32);
}

GpuSimulator::~GpuSimulator(){
    sim_transforms = nullptr;
    sim_static_vertices = nullptr;
    sim_static_indices = nullptr;
}

void GpuSimulator::update(float delta_time, glm::vec3 gravity){
    //Transform phase
    int work_groups = (sim_transforms->size() + 256 - 1) / 256;
    m_transform_shader.use();
    m_transform_shader.setUniform1f("delta_time", delta_time);
    m_transform_shader.setUniform3f("gravity", gravity.x, gravity.y, gravity.z);
    m_transform_shader.dispatch(work_groups, 1, 1);
    m_transform_shader.waitForCompletion(GL_SHADER_STORAGE_BARRIER_BIT);

    //Broad phase
    // SPLITED LOAD
    work_groups = (sim_transforms->size() + 8 - 1) / 8;
    m_broad_phase_shader.use();
    m_broad_phase_shader.dispatch(work_groups * 64, 1, 1);
    m_broad_phase_shader.waitForCompletion(GL_SHADER_STORAGE_BARRIER_BIT);

    // SIMPLE LOAD
    // work_groups = (sim_transforms->size() + 256 - 1) / 256;
    // m_broad_phase_shader.use();
    // m_broad_phase_shader.dispatch(work_groups, 1, 1);
    // m_broad_phase_shader.waitForCompletion(GL_SHADER_STORAGE_BARRIER_BIT);

    glMemoryBarrier(GL_SHADER_STORAGE_BARRIER_BIT | GL_BUFFER_UPDATE_BARRIER_BIT);
    m_collision_count_ssbo.bind();
    unsigned int collision_counter = *(unsigned int *)m_collision_count_ssbo.readData();
    m_collision_count_ssbo.unmapBuffer();
    
    // Narrow phase and resolution
    if(collision_counter > 0){
        work_groups = (collision_counter + 512 - 1) / 512;
        m_narrow_phase_shader.use();
        m_narrow_phase_shader.dispatch(work_groups, 1, 1);
        
        m_narrow_phase_shader.waitForCompletion(GL_SHADER_STORAGE_BARRIER_BIT);
        glMemoryBarrier(GL_SHADER_STORAGE_BARRIER_BIT | GL_BUFFER_UPDATE_BARRIER_BIT);
        m_collision_count_ssbo.bind();
        collision_counter = *(unsigned int *)m_collision_count_ssbo.readData();
        // std::cout<<"Max count: "<<sim_spheres.size() * sim_spheres.size() <<" actual count: "<<collision_counter<<" Work Groups: "<<work_groups<<std::endl;
        m_collision_count_ssbo.unmapBuffer();
        

        for(int i = 0; i < 10 && collision_counter > 0; i++){

            // // ------------------------------------------------------------------//
            // std::cout<<std::endl;
            // std::cout<<"--------------------------------------------------------------------"<<std::endl;
            // std::cout<<"MANIFOLDS: ";
            // m_contact_manifolds_ssbo.bind();
            // physics::ContactManifold* obj3 = (physics::ContactManifold *)m_contact_manifolds_ssbo.readData();
            // for(int i = 0; i < collision_counter; i++){
            //     glm::vec3 normal = obj3[i].normal;
            //     glm::vec3 contactPointA = obj3[i].rAWorld;
            //     glm::vec3 contactPointB = obj3[i].rBWorld;
            //     unsigned int indexA = obj3[i].indexA;
            //     unsigned int indexB = obj3[i].indexB;
            //     float depth = obj3[i].depth;
            //     std::cout<<"Objects "<<indexA<< " and "<< indexB <<std::endl;
            //     std::cout<<"Normal: "<<normal.x<<" "<<normal.y<<" "<<normal.z<<" | "<<std::endl;;
            //     std::cout<<"Depth: "<<depth<<" | "<<std::endl;;
            //     std::cout<<"Contact Point "<<indexA<<": "<<contactPointA.x<<" "<<contactPointA.y<<" "<<contactPointA.z<<" | "<<std::endl;;
            //     std::cout<<"Contact Point "<<indexB<<": "<<contactPointB.x<<" "<<contactPointB.y<<" "<<contactPointB.z<<" | ";

            //     std::cout<<std::endl;
            // }
            // std::cout<<"delta_time: "<<delta_time<<std::endl;
            // m_contact_manifolds_ssbo.unmapBuffer();
            // glMemoryBarrier(GL_SHADER_STORAGE_BARRIER_BIT | GL_BUFFER_UPDATE_BARRIER_BIT);
            // // ------------------------------------------------------------------//


            work_groups = (collision_counter + 256 - 1) / 256;
            m_impulse_phase_shader.use();
            m_impulse_phase_shader.setUniform1f("delta_time", delta_time);
            m_impulse_phase_shader.dispatch(work_groups, 1, 1);
            
            glMemoryBarrier(GL_SHADER_STORAGE_BARRIER_BIT | GL_BUFFER_UPDATE_BARRIER_BIT); // Essential for SSBO read-after-write
            
            // // ------------------------------------------------------------------//
            // std::cout<<"VELOCITIES: "<<std::endl;
            // m_properties_ssbo.bind();
            // physics::Properties* obj2 = (physics::Properties *)m_properties_ssbo.readData();
            // for(int i = 0; i < sim_spheres.size(); i++){
            //     glm::vec3 velocity = obj2[i].velocity;
            //     glm::vec3 ang = obj2[i].angular_velocity;
            //     float invMass =  obj2[i].inverseMass;
            //     std::cout<<"Object velocity "<<i <<": "<<velocity.x<<" "<<velocity.y<<" "<<velocity.z
            //     <<" angualr: "<<ang.x<<" "<<ang.y<<" "<<ang.z<<" InvMass :" <<invMass<<" | "<<std::endl;
            // }
            // m_properties_ssbo.unmapBuffer();
            // glMemoryBarrier(GL_SHADER_STORAGE_BARRIER_BIT | GL_BUFFER_UPDATE_BARRIER_BIT);
            // std::cout<<std::endl;

            // m_deltaV_ssbo.bind();
            // glm::vec4 * obj = (glm::vec4 *)m_deltaV_ssbo.readData();
            // for(int i = 0; i < sim_spheres.size(); i++){
            //     glm::vec4 velocity = obj[i];
            //     std::cout<<"Object Linear Impulse "<<i <<": "<<velocity.x<<" "<<velocity.y<<" "<<velocity.z<<" | ";
            // }
            // m_deltaV_ssbo.unmapBuffer();
            // glMemoryBarrier(GL_SHADER_STORAGE_BARRIER_BIT | GL_BUFFER_UPDATE_BARRIER_BIT);
            // std::cout<<std::endl;

            // m_deltaW_ssbo.bind();
            // obj = (glm::vec4 *)m_deltaW_ssbo.readData();
            // for(int i = 0; i < sim_spheres.size(); i++){
            //     glm::vec4 ang = obj[i];
            //     std::cout<<"Object Angular Impulse "<<i <<": "<<ang.x<<" "<<ang.y<<" "<<ang.z<<" | ";
            // }
            // m_deltaW_ssbo.unmapBuffer();
            // glMemoryBarrier(GL_SHADER_STORAGE_BARRIER_BIT | GL_BUFFER_UPDATE_BARRIER_BIT);
            // std::cout<<std::endl;
            // std::cout<<std::endl;
            // // ------------------------------------------------------------------//

            
            work_groups = (sim_transforms->size() + 256 - 1) / 256;
            // std::cout<<"Work: "<<work_groups<<" collision: "<<collision_counter<<std::endl;
            m_accumulation_phase_shader.use();
            m_accumulation_phase_shader.dispatch(work_groups, 1, 1);

            glMemoryBarrier(GL_SHADER_STORAGE_BARRIER_BIT | GL_BUFFER_UPDATE_BARRIER_BIT);

            // //------------------------------------------------------------------//

            // m_deltaV_ssbo.bind();
            // obj = (glm::vec4 *)m_deltaV_ssbo.readData();
            // for(int i = 0; i < sim_spheres.size(); i++){
            //     glm::vec4 velocity = obj[i];
            //     std::cout<<"Object Linear Impulse "<<i <<": "<<velocity.x<<" "<<velocity.y<<" "<<velocity.z<<" | ";
            // }
            // m_deltaV_ssbo.unmapBuffer();
            // glMemoryBarrier(GL_SHADER_STORAGE_BARRIER_BIT | GL_BUFFER_UPDATE_BARRIER_BIT);
            // std::cout<<std::endl;
            
            // m_deltaW_ssbo.bind();
            // obj = (glm::vec4 *)m_deltaW_ssbo.readData();
            // for(int i = 0; i < sim_spheres.size(); i++){
            //     glm::vec4 ang = obj[i];
            //     std::cout<<"Object Angular Impulse "<<i <<": "<<ang.x<<" "<<ang.y<<" "<<ang.z<<" | ";
            // }
            // m_deltaW_ssbo.unmapBuffer();
            // glMemoryBarrier(GL_SHADER_STORAGE_BARRIER_BIT | GL_BUFFER_UPDATE_BARRIER_BIT);
            // std::cout<<std::endl;

            // std::cout<<"VELOCITIES: "<<std::endl;
            // m_properties_ssbo.bind();
            // obj2 = (physics::Properties *)m_properties_ssbo.readData();
            // for(int i = 0; i < sim_spheres.size(); i++){
            //     glm::vec3 velocity = obj2[i].velocity;
            //     glm::vec3 ang = obj2[i].angular_velocity;
            //     float invMass =  obj2[i].inverseMass;
            //     std::cout<<"Object velocity "<<i <<": "<<velocity.x<<" "<<velocity.y<<" "<<velocity.z
            //     <<" angualr: "<<ang.x<<" "<<ang.y<<" "<<ang.z<<" InvMass :" <<invMass<<" | "<<std::endl;
            // }
            // m_properties_ssbo.unmapBuffer();
            // glMemoryBarrier(GL_SHADER_STORAGE_BARRIER_BIT | GL_BUFFER_UPDATE_BARRIER_BIT);
            // std::cout<<std::endl<<"--------------------------------------------------------------------"<<std::endl;

            // //------------------------------------------------------------------//
        }
    }
}


void GpuSimulator::initializeData(){
    //Create AABBs
    float base_radius = utils::calculateRadius(*sim_static_vertices);
    std::cout<<base_radius<<std::endl;
    sim_spheres.resize(sim_transforms->size(), glm::vec4(0.0f, 0.0f, 0.0f, base_radius));


    float mass = 1.0f;
    float side = 1.0f;

    // float inertia = (1.0f / 6.0f) * mass * side * side;
    // float inverseInertia = 1.0f / inertia;
    // glm::mat3 inverseTensor = glm::mat3(inverseInertia);

    float inertia = (1.0f / 6.0f) * mass * side * side;
    float inverseInertia = 1.0f / inertia;

    glm::mat3 inverseTensor = glm::mat3(
        1.0f / inertia, 0.0f, 0.0f,
        0.0f, 1.0f / inertia, 0.0f,
        0.0f, 0.0f, 1.0f / inertia
    );

    // for (int i = 0; i < 3; i++) {
    //     for (int j = 0; j < 3; j++) {
    //         std::cout << inverseTensor[i][j] << " ";
    //     }
    //     std::cout << std::endl; // Move to the next row
    // }
    std::srand(42);
    for (int i = 0; i < sim_transforms->size(); i++){
        
        auto transform = &sim_transforms->at(i);
        
        glm::vec3 scale = utils::scaleFromTransform(*transform); 
        sim_spheres[i].w *= glm::max(scale.x, glm::max(scale.y, scale.z));

        sim_spheres[i][0] = (*transform)[3][0];
        sim_spheres[i][1] = (*transform)[3][1];
        sim_spheres[i][2] = (*transform)[3][2];

    }


    // m_normal_impulses.resize(sim_spheres.size() * sim_spheres.size() , 0);
    // m_tangent_impulses.resize(sim_spheres.size() * sim_spheres.size() , glm::vec3(0.0f));
    // m_delta_linear_impulses.resize(sim_transforms->size(), glm::vec3(0.0f));
    // m_delta_angular_impulses.resize(sim_transforms->size(), glm::vec3(0.0f));
}