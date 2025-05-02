#version 440 core // Need atomics and SSBOs
#extension GL_NV_shader_atomic_float : enable

// Include structs (assuming PropertiesStruct and ContactManifold are defined)
#include "common_structs.glsl" 

// --- Uniforms ---
uniform int u_SolverIterations = 1;
uniform float u_Timestep = 1.0 / 71.4;
uniform float u_Beta = 0.0;          // Baumgarte coefficient
uniform float u_RestitutionSlop = 0.01; // How much velocity needed before restitution applies
uniform float u_PenetrationSlop = 0.005; // Allowed penetration

// --- Buffer Definitions ---

// The ContactManifold struct definition remains the same
struct ContactManifold {
    uint indexA;
    uint indexB;
    vec4 normal;          // World space, consistent direction (e.g., A->B)
    float depth;          // Penetration depth
    vec4 pointWorld;      // Contact point in world space
    // uint type;         // Optional: Store collision type (FaceA, FaceB, EdgeEdge)
    // uint featureA;     // Optional: Store feature index on A
    // uint featureB;     // Optional: Store feature index on B
};

// Input Buffers (Read-only or Read/Write via Atomics)
layout(std430, binding = 26) readonly buffer ContactManifoldBuffer {
    ContactManifold manifolds[];
};

layout(std430, binding = 1) readonly buffer TransformBuffer {
    mat4 transforms[];
};

layout(std430, binding = 5) readonly buffer PropertiesBuffer {
    PropertiesStruct properties[]; // Contains invMass, invInertiaTensorLocal, friction, restitution
};

// Warm Starting Buffers (Read/Write - Persistent across frames)
// Size must match max manifolds
layout(std430, binding = 27) buffer AccumulatedNormalImpulseBuffer {
    float accumulatedNormalImpulse[];
};

layout(std430, binding = 28) buffer AccumulatedTangentImpulseBuffer {
    vec3 accumulatedTangentImpulse[]; // Store tangent impulse vector directly
};

// Temporary Delta Impulse Accumulators (Read/Write via Atomics - Reset each frame)
// Size must match max rigid bodies
layout(std430, binding = 29) buffer DeltaLinearImpulseAccumulator {
    vec3 deltaLinearImpulseAccumulator[]; // Stores sum of delta J for each object IN THIS FRAME
};

layout(std430, binding = 30) buffer DeltaAngularImpulseAccumulator {
    vec3 deltaAngularImpulseAccumulator[]; // Stores sum of delta (r x J) for each object IN THIS FRAME
};


// --- Helper Functions ---

// Calculates World Space Inverse Inertia Tensor
mat3 getInverseInertiaTensorWorld(uint objectId) {
    mat3 R = mat3(transforms[objectId]); // Extract rotation part
    mat3 invI_local = properties[objectId].inverseInertiaTensor; // Assuming this is stored
    // Formula: invI_world = R * invI_local * transpose(R)
    return R * invI_local * transpose(R);
}


// --- Main Shader Logic ---

layout(local_size_x = 256, local_size_y = 1, local_size_z = 1) in;

void main() {
    uint contactIndex = gl_GlobalInvocationID.x;
    uint numManifolds = manifolds.length(); // Or get from a uniform/buffer

    if (contactIndex >= numManifolds) {
        return; // Exit if this thread has no contact to process
    }

    // --- Get Contact Data ---
    ContactManifold contact = manifolds[contactIndex];
    uint indexA = contact.indexA;
    uint indexB = contact.indexB;
    vec3 normal = contact.normal.xyz; // Assumed normalized and pointing A->B
    vec3 pointWorld = contact.pointWorld.xyz;
    float penetration = contact.depth;

    // --- Get Object Properties ---
    PropertiesStruct propsA = properties[indexA];
    PropertiesStruct propsB = properties[indexB];

    float invMassA = propsA.inverseMass;
    float invMassB = propsB.inverseMass;
    mat3 invInertiaWorldA = getInverseInertiaTensorWorld(indexA);
    mat3 invInertiaWorldB = getInverseInertiaTensorWorld(indexB);

    // Combine friction & restitution (e.g., average or multiply)
    float frictionCoefficient = (propsA.friction + propsB.friction) * 0.5;
    // float restitution = (propsA.restitution + propsB.restitution) * 0.5;
    float restitution = 0.8f;

    // --- Calculate Contact Point Offsets (rA, rB) ---
    vec3 centerOfMassA = transforms[indexA][3].xyz; // Position from transform matrix
    vec3 centerOfMassB = transforms[indexB][3].xyz;
    vec3 rA = pointWorld - centerOfMassA;
    vec3 rB = pointWorld - centerOfMassB;

    // --- Warm Starting: Load Previous Accumulated Impulses ---
    // These persist across frames for stability
    float jn_accum_prev_frame = accumulatedNormalImpulse[contactIndex];
    vec3 jt_accum_prev_frame = accumulatedTangentImpulse[contactIndex];

    // Initialize accumulators for *this frame's iterations*
    // float jn_accum_current = jn_accum_prev_frame;
    // vec3  jt_accum_current = jt_accum_prev_frame; // Store tangent impulse vector

    float jn_accum_current = 0.0f;
    vec3  jt_accum_current = vec3(0.0f, 0.0f, 0.0f); // Store tangent impulse vector


    // === SOLVER ITERATION LOOP ===
    for (int iter = 0; iter < u_SolverIterations; ++iter) {

        // --- 1. Read CURRENT Velocities ---
        // Crucial: Read velocities *at the start of each iteration*
        // These velocities reflect changes from previous iterations within this frame
        // vec3 linVelA = linearVelocities[indexA];
        // vec3 angVelA = angularVelocities[indexA];
        // vec3 linVelB = linearVelocities[indexB];
        // vec3 angVelB = angularVelocities[indexB];
        vec3 linVelA = propsA.velocity.xyz;
        vec3 angVelA = propsA.angular_velocity.xyz;
        vec3 linVelB = propsB.velocity.xyz;
        vec3 angVelB = propsB.angular_velocity.xyz;

        // --- 2. Calculate Relative Velocity at Contact Point ---
        vec3 vA_contact = linVelA + cross(angVelA, rA);
        vec3 vB_contact = linVelB + cross(angVelB, rB);
        vec3 v_rel = vB_contact - vA_contact;
        float v_rel_normal = dot(v_rel, normal);

        // --- 3. Calculate Normal Impulse (jn) ---

        // Calculate Effective Mass (Normal)
        vec3 rnA = cross(rA, normal);
        vec3 rnB = cross(rB, normal);
        float termA_n = dot(rnA, invInertiaWorldA * rnA);
        float termB_n = dot(rnB, invInertiaWorldB * rnB);
        float inv_eff_mass_normal = invMassA + invMassB + termA_n + termB_n;
        float eff_mass_normal = (inv_eff_mass_normal > 0.0) ? 1.0 / inv_eff_mass_normal : 0.0;

        // Calculate Bias Velocity (Baumgarte)
        float v_bias = (u_Beta / u_Timestep) * max(0.0, penetration - u_PenetrationSlop);

        // Calculate Restitution Velocity
        // Only apply if separating velocity is less than a small threshold (slop)
        float v_restitution = 0.0;
        if (v_rel_normal < -u_RestitutionSlop) { // Check if objects are actually approaching significantly
            v_restitution = -restitution * v_rel_normal;
        }

        // Desired Velocity Change (Normal)
        float delta_v_n = v_restitution + v_bias - v_rel_normal; // Target change needed

        // Calculate Impulse Magnitude required THIS ITERATION to achieve delta_v_n
        float jn = eff_mass_normal * delta_v_n;

        // --- Accumulate & Clamp Normal Impulse ---
        float jn_accum_prev_iter = jn_accum_current; // Store value before adding new jn
        jn_accum_current += jn;                      // Add the calculated impulse
        jn_accum_current = max(0.0, jn_accum_current); // Clamp: Accumulated impulse cannot be negative (pulling)

        // Calculate the IMPULSE TO APPLY in this iteration
        float delta_jn = jn_accum_current - jn_accum_prev_iter;
        vec3 delta_Jn_vec = delta_jn * normal; // Vector impulse change

        // --- 4. Calculate Friction Impulse (Jt) ---

        // Tangential Velocity
        vec3 v_rel_tangent = v_rel - v_rel_normal * normal;
        float tangent_speed = length(v_rel_tangent);
        vec3 tangent_dir = vec3(0.0);
        if (tangent_speed > 1e-6) { // Avoid division by zero / degenerate cases
            tangent_dir = v_rel_tangent / tangent_speed;
        }

        // Calculate Effective Mass (Tangent - using approximation with normal eff mass for simplicity)
        // A more accurate calculation would involve cross(rA, tangent_dir) etc.
        float eff_mass_tangent = eff_mass_normal; // Approximation
        // Or, more accurately (but requires care with choosing tangent axes):
        // vec3 rtA = cross(rA, tangent_dir);
        // vec3 rtB = cross(rB, tangent_dir);
        // float termA_t = dot(rtA, invInertiaWorldA * rtA);
        // float termB_t = dot(rtB, invInertiaWorldB * rtB);
        // float inv_eff_mass_tangent = invMassA + invMassB + termA_t + termB_t;
        // eff_mass_tangent = (inv_eff_mass_tangent > 0.0) ? 1.0 / inv_eff_mass_tangent : 0.0;

        // Calculate Tangential Impulse Magnitude required THIS ITERATION to stop sliding
        float jt = eff_mass_tangent * (-tangent_speed); // Impulse needed to counteract v_rel_tangent

        // --- Accumulate & Clamp Friction Impulse ---
        vec3 jt_accum_prev_iter = jt_accum_current; // Store value before adding new jt
        jt_accum_current += jt * tangent_dir;       // Add the calculated impulse vector

        // Clamp Friction based on Coulomb Limit (using the *total accumulated normal impulse*)
        float max_friction = frictionCoefficient * jn_accum_current;
        float accum_friction_mag = length(jt_accum_current);

        if (accum_friction_mag > max_friction) {
            jt_accum_current = (jt_accum_current / accum_friction_mag) * max_friction; // Clamp magnitude
        }

        // Calculate the TANGENT IMPULSE TO APPLY in this iteration
        vec3 delta_Jt_vec = jt_accum_current - jt_accum_prev_iter;

        // --- 5. Calculate Total Delta Impulse for this Iteration ---
        vec3 delta_J_total = delta_Jn_vec + delta_Jt_vec;

        // --- 6. Atomically Accumulate DELTA Impulses onto Objects ---
        // Apply impulse change to accumulators. Remember J acts on B, -J acts on A.
        // These accumulators are specific to THIS FRAME's solver run.
        // if (invMassA > 0.0) { // Check if object A is movable
        //      atomicAdd(deltaLinearImpulseAccumulator[indexA], -delta_J_total);
        //      vec3 angular_impulseA = -cross(rA, delta_J_total);
        //      atomicAdd(deltaAngularImpulseAccumulator[indexA], invInertiaWorldA * angular_impulseA); // Accumulate delta angular *velocity*
        // }
        // if (invMassB > 0.0) { // Check if object B is movable
        //      atomicAdd(deltaLinearImpulseAccumulator[indexB], delta_J_total);
        //      vec3 angular_impulseB = cross(rB, delta_J_total);
        //      atomicAdd(deltaAngularImpulseAccumulator[indexB], invInertiaWorldB * angular_impulseB); // Accumulate delta angular *velocity*
        // }
        if (invMassA > 0.0) { // Check if object A is movable
            // Break down the vec3 atomic add into component-wise operations
            atomicAdd(deltaLinearImpulseAccumulator[indexA].x, -delta_J_total.x);
            atomicAdd(deltaLinearImpulseAccumulator[indexA].y, -delta_J_total.y);
            atomicAdd(deltaLinearImpulseAccumulator[indexA].z, -delta_J_total.z);
            
            vec3 angular_impulseA = -cross(rA, delta_J_total);
            vec3 angular_delta = invInertiaWorldA * angular_impulseA;
            
            // Component-wise atomic adds for angular impulse
            atomicAdd(deltaAngularImpulseAccumulator[indexA].x, angular_delta.x);
            atomicAdd(deltaAngularImpulseAccumulator[indexA].y, angular_delta.y);
            atomicAdd(deltaAngularImpulseAccumulator[indexA].z, angular_delta.z);
        }

        if (invMassB > 0.0) { // Check if object B is movable
            // Break down the vec3 atomic add into component-wise operations
            atomicAdd(deltaLinearImpulseAccumulator[indexB].x, delta_J_total.x);
            atomicAdd(deltaLinearImpulseAccumulator[indexB].y, delta_J_total.y);
            atomicAdd(deltaLinearImpulseAccumulator[indexB].z, delta_J_total.z);
            
            vec3 angular_impulseB = cross(rB, delta_J_total);
            vec3 angular_delta = invInertiaWorldB * angular_impulseB;
            
            // Component-wise atomic adds for angular impulse
            atomicAdd(deltaAngularImpulseAccumulator[indexB].x, angular_delta.x);
            atomicAdd(deltaAngularImpulseAccumulator[indexB].y, angular_delta.y);
            atomicAdd(deltaAngularImpulseAccumulator[indexB].z, angular_delta.z);
        }

        // --- 7. Synchronization Barrier ---
        // Wait for ALL contacts to finish calculating and accumulating their
        // impulses for THIS iteration before applying velocity updates.
        barrier();
        // memoryBarrierBuffer(); // Ensure writes to buffer accumulators are visible

        // --- 8. Apply Accumulated Velocity Changes (Simplified Approach) ---
        // This part *could* be a separate shader dispatch per object, which can be
        // more efficient if #objects << #contacts. Here, we do it in the same shader.
        // We need *another* barrier if we were to read the velocity changes applied
        // by other threads in the *same* iteration.

        // This simplified approach directly modifies the velocities based on the
        // impulse calculated by *this* thread. This is NOT pure Jacobi anymore,
        // it introduces some Gauss-Seidel like behaviour depending on thread order.
        // A PURE Jacobi requires a separate pass/kernel to apply the accumulated deltas.

        // **Let's stick to the pure Jacobi method described previously:**
        // The ACCUMULATION step (6) stores the *total impulse effect* for the iteration.
        // The application happens *after* the loop OR in a separate kernel.
        // The `barrier()` ensures all accumulations are done *before* the next iteration reads velocities.

        // *** NO VELOCITY APPLICATION INSIDE THE LOOP FOR PURE JACOBI ***
        // Velocities are updated based on the accumulators *after* the loop or in a separate pass.
        // The barrier() above ensures that when iteration i+1 starts, it reads velocities
        // that have implicitly incorporated the effects calculated in iteration i, because
        // all threads will have finished their atomicAdds for iteration i.

        // *** CORRECTION: Re-reading the original description and goal ***
        // The description implies Calculate -> Accumulate -> Apply *within* each iteration.
        // This means we *do* need an apply step here, likely in a separate kernel/pass.
        // Let's simulate that structure by adding the application logic *conceptually*
        // after the barrier, assuming a second kernel would run per-object.

        // (Conceptual step - would be in a separate kernel running per object)
        // KERNEL 2 (Apply Deltas - runs per object 'objID'):
        // {
        //      vec3 totalDeltaLinImpulse = deltaLinearImpulseAccumulator[objID];
        //      vec3 totalDeltaAngVelocity = deltaAngularImpulseAccumulator[objID]; // We accumulated velocity change
        //
        //      if (properties[objID].invMass > 0.0) {
        //          linearVelocities[objID] += totalDeltaLinImpulse * properties[objID].invMass;
        //          angularVelocities[objID] += totalDeltaAngVelocity;
        //      }
        //
        //      // Reset accumulators for next iteration
        //      deltaLinearImpulseAccumulator[objID] = vec3(0.0);
        //      deltaAngularImpulseAccumulator[objID] = vec3(0.0);
        // }
        // // KERNEL 2 finishes, then another barrier if needed before next iteration

        // **Alternative (Less pure Jacobi, common in games): Apply directly after barrier**
        // To avoid a second kernel dispatch *per iteration*, some engines apply the
        // accumulated changes directly. This requires careful synchronization.
        // Let's assume the Apply step happens *outside* this shader *after* all iterations.

    } // === END SOLVER ITERATION LOOP ===


    // --- Store Final Accumulated Impulses for Next Frame's Warm Starting ---
    // This happens *after* all iterations are complete for this contact.
    accumulatedNormalImpulse[contactIndex] = jn_accum_current;
    accumulatedTangentImpulse[contactIndex] = jt_accum_current;

}