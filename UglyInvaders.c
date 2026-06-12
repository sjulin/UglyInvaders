#define _POSIX_C_SOURCE 199309L

#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include <time.h>

#define VECTOR_IMPLEMENTATION

#include "V7Vulkan2.h"
#include "V7VGeometry.h"
#include "V7Util.h"
#include "V7Image.h"

#include "font/sdf_master_breaker.h"
#include "font/sdf_topaz.h"


#define MAX_INVADERS 40
#define THRUST_DEF .00003f
#define THRUST_DEC .000005f
#define THRUST_INC .00001f

float leveling;

Scene *scn;
Model *sky, *spaceship, *afterburner, *flyx, *wasper, *invader[MAX_INVADERS];
Model *logo, *txt1, *txt2;

V7Image *img;



typedef struct {
    char *name;
    enum type;
} LoadList;

LoadList *asslist = {
    "Spaceship2", 1,
    "Flyx", 1,
    "Wasper", 1
};

Model *LoadGLB(V7V *v7v, const char *name) {
    size_t glbsize; void *glb; Model *mod;
    glb = V7Load(&glbsize, "data/%s.glb", name);
    mod = V7VGLB(v7v, name, glb, glbsize);
    mod->pipeline = v7v->vc->pbrPipeline;  // Use PBR pipeline
    return mod;
}

int LoadAssets() {




}

static void Setup(V7V *v7v) {

    scn = v7v->scn = V7VNewScn(v7v, "Stage 01");

    vec_reserve(&v7v->scn->vmod, 128); // Up to 128 Models in scene

    spaceship = LoadGLB(v7v, "Spaceship2");
//    afterburner = LoadGLB(v7v, "Afterburner");
//    V7VParent(afterburner, spaceship);

    txt1 = V7NewTxt(v7v, "TitleTxt", &sdf_topaz);
	V7Txt(txt1, "\x02""UglyInvaders\n");
    V7VMove(txt1, 0,20,20);

    V7VScale(spaceship, .05f);

    V7VMove(spaceship, .0f,-1.f,-2.f);

    logo = LoadGLB(v7v, "UglyInvaders");

    flyx = LoadGLB(v7v, "Flyx");
    wasper = LoadGLB(v7v, "Wasper");
    
    V7VMove(flyx, -4.0f,-10.f,-2.f);
    V7VMove(wasper, 4.0f,-10.f,2.f);

    for(int i=0; i<MAX_INVADERS; i++) {
        char name[16]; sprintf(name, "Invader%02d", i);
        invader[i] = V7VNewMod(v7v, name);
        if((i/8)&~0x1)
            invader[i]->msh = flyx->msh;
        else invader[i]->msh = wasper->msh;

        V7VScale(invader[i], .5f);
        V7VMove(invader[i], (i%8)/2.f-2.f, (i/8)/4.f+2.f, 10.f );

    }

    // Create skybox
    sky = V7VNewMod(v7v, "sky");
    sky->msh = V7VCreateSkyBox(v7v, 1000.0f);
    V7VUploadMesh(v7v, sky->msh);
    sky->pipeline = v7v->vc->skyPipeline;

    scn->ubo.cloudCoverage = 0.6f;
    scn->ubo.cloudSharpness = 0.7f;

}

// Thrust adds force in the direction plane is pointing
float pitch, roll, thrust = THRUST_DEF;
// Motion vector - independent of plane orientation
vec3 velocity = {0.0f, 0.0f, 0.0f};

static void Loop(V7V *v7v) {
    
    float speedo = 0.05f;

    if (v7v->key[_ESC_]) {
        v7v->quit=1;
    }
    
    // Thrust: W/S
    if (v7v->key[_W_]||v7v->key[_S_]) {
        if (v7v->key[_W_]) thrust+=THRUST_INC;
        if (v7v->key[_S_]) thrust-=THRUST_INC;
    } else {
        if(thrust>THRUST_DEF+THRUST_INC) { thrust-=THRUST_DEC; } else
        if(thrust<THRUST_DEF+THRUST_INC) { thrust+=THRUST_DEC; }
    }

    // Get plane's current orientation vectors first (before applying new rotations)
    vec3 upVec, forwardVec, rightVec;
    glm_quat_rotatev(spaceship->r, xaxis, rightVec);
    glm_quat_rotatev(spaceship->r, yaxis, upVec);
    glm_quat_rotatev(spaceship->r, zaxis, forwardVec);
    
    // Auto homing: gradually point towards origin
    if (v7v->key[_A_]) {
        vec3 origin = {0.0f, 4.0f, 10.0f};
        vec3 toOrigin;
        glm_vec3_sub(origin, spaceship->t, toOrigin);
        
        float distToOrigin = glm_vec3_norm(toOrigin);
        if (distToOrigin > 0.1f) {
            // Normalize direction to origin
            glm_vec3_normalize(toOrigin);
            
            // How much is target to the right/left of plane's forward direction?
            float rightError = glm_vec3_dot(toOrigin, rightVec);
            // Roll to bank into the turn (negative to turn toward target)
            roll -= rightError * 0.001f;
            
            // How much is target above/below plane's forward direction?
            float upError = glm_vec3_dot(toOrigin, upVec);
            // Pitch to aim up/down (negative to pitch toward target)
            pitch -= upError * 0.0008f;
            
            // How aligned are we with target? (forward dot toOrigin)
            float alignment = glm_vec3_dot(forwardVec, toOrigin);
            // If not well aligned, increase roll to turn faster
            if (alignment < 0.9f) {
                roll -= rightError * 0.0015f;
            }
        }
    }

    // Steering: I/J/K/L
    if (v7v->key[_J_]||v7v->key[_K_]||v7v->key[_L_]||v7v->key[_I_]) {
        if (v7v->key[_J_]) { roll+=.002f;  }
        if (v7v->key[_L_]) { roll-=.002f;  }
        if (v7v->key[_I_]) pitch+=.001f;
        if (v7v->key[_K_]) pitch-=.001f;
        leveling = 0.f;
    } else {
        if(leveling<0.005) leveling +=0.0001;
    }

    if (v7v->key[_LEFT_]) {
        spaceship->t[0] -= speedo;
    }
    if (v7v->key[_RIGHT_]) {
        spaceship->t[0] += speedo;
    }

    // Fly the spaceship - SIMPLE REALISTIC PHYSICS MODEL
    
    // Control inputs
    if (pitch != 0.0f) {
        V7VRotate(spaceship, xaxis, pitch);
        pitch *= 0.95f;
    }
    
    if (roll != 0.0f) {
        V7VRotate(spaceship, zaxis, roll);
        roll *= 0.95f;
    }
    
    // Recalculate orientation vectors after rotations
    glm_quat_rotatev(spaceship->r, xaxis, rightVec);
    glm_quat_rotatev(spaceship->r, yaxis, upVec);
    glm_quat_rotatev(spaceship->r, zaxis, forwardVec);
    
    // Auto-leveling: slowly correct banking and pitch toward level flight
    // rightVec[1] tells us bank: 0 = level, >0 = banked right, <0 = banked left
    float currentBank = rightVec[1];
    // forwardVec[1] tells us pitch: 0 = level, >0 = nose up, <0 = nose down
    float currentPitch = forwardVec[1];

    if(leveling<0.005) leveling +=0.0001;
    
    // Correct roll toward level
    float levelingRoll = -currentBank * leveling;
    V7VRotate(spaceship, zaxis, levelingRoll);
    
    // Correct pitch toward level
    float levelingPitch = -currentPitch * leveling;
    V7VRotate(spaceship, xaxis, levelingPitch);
    
    // Recalculate orientation vectors after leveling corrections
    glm_quat_rotatev(spaceship->r, yaxis, upVec);
    glm_quat_rotatev(spaceship->r, zaxis, forwardVec);
    
    vec3 thrustVec;
    glm_vec3_scale(forwardVec, thrust, thrustVec);
    glm_vec3_add(velocity, thrustVec, velocity);
    
    // Gravity - constant downward force
    float gravity = 0.001f;
    velocity[1] -= gravity;
    
    // Lift calculation based on angle of attack and airspeed
    float airspeed = glm_vec3_norm(velocity);
    if (airspeed > 0.001f) {
        // Calculate velocity direction
        vec3 velocityDir;
        glm_vec3_normalize_to(velocity, velocityDir);
        
        // Simple approach: lift pushes up when nose is above velocity vector
        // forwardVec[1] > velocityDir[1] means nose is pitched up
        float angleOfAttack = forwardVec[1] - velocityDir[1];
        
        // Wing orientation factor: how horizontal are the wings?
        // upVec[1] = 1.0 when level (100% lift)
        // upVec[1] = 0.0 when banked 90° (0% lift)
        // upVec[1] = -1.0 when inverted (50% lift)
        float wingFactor = (upVec[1] > 0.0f) ? upVec[1] : fabs(upVec[1]) * 0.5f;
        
        // Lift proportional to airspeed squared, angle of attack, and wing orientation
        float liftCoefficient = 0.8f;
        float lift = airspeed * airspeed * liftCoefficient * angleOfAttack * wingFactor;
        velocity[1] += lift;
    }
    
    // Moderate drag - allows speed to build up
    glm_vec3_scale(velocity, 0.99f, velocity);  // 1% drag per frame
    
    // Cap maximum speed to keep things sane
    if (airspeed > 0.05f) {
        glm_vec3_normalize(velocity);
        glm_vec3_scale(velocity, 0.05f, velocity);
    }
    
    // Update position based on velocity vector
    glm_vec3_add(spaceship->t, velocity, spaceship->t);
    
    glm_vec3_copy(spaceship->t, scn->ubo.center);
    
    if (v7v->key[_UP_]) {
    }
    if (v7v->key[_DOWN_]) {
    }

    if (v7v->key[_Z_]) { scn->ubo.cloudCoverage -= 0.02f; }
    if (v7v->key[_X_]) { scn->ubo.cloudCoverage += 0.02f; }
    if (v7v->key[_C_]) { scn->ubo.cloudSharpness -= 0.02f; }
    if (v7v->key[_V_]) { scn->ubo.cloudSharpness += 0.02f; }

    // Animate day/night cycle
    static float time = 0.0f;
    time += 0.001f;
    float blend = (sin(time) + 1.0f) * 0.5f; // Oscillate between 0 and 1
    
    // Day colors - 5 step gradient
    vec3 dayZenith = {0.1f, 0.3f, 0.6f};      // Deep blue at top
    vec3 dayMidHigh = {0.3f, 0.5f, 0.8f};     // Medium blue
    vec3 dayHorizon = {0.4f, 0.6f, 0.9f};     // Light blue at horizon
    vec3 dayMidLow = {0.04f, 0.04f, 0.10f};      // Slightly darker
    vec3 dayNadir = {0.08f, 0.05f, 0.06f};      // Ground
    
    // Night colors - 5 step gradient
    vec3 nightZenith = {0.01f, 0.01f, 0.05f};   // Nearly black at top
    vec3 nightMidHigh = {0.02f, 0.02f, 0.08f};  // Very dark blue
    vec3 nightHorizon = {0.05f, 0.05f, 0.15f};  // Dark blue at horizon
    vec3 nightMidLow = {0.03f, 0.03f, 0.10f};   // Slightly lighter
    vec3 nightNadir = {0.02f, 0.02f, 0.03f};    // Dark ground
    
    // Blend all 5 colors
    vec3 zenith, midHigh, horizon, midLow, nadir;
    glm_vec3_lerp(nightZenith, dayZenith, blend, scn->ubo.zenithColor);
    glm_vec3_lerp(nightMidHigh, dayMidHigh, blend, scn->ubo.midHighColor);
    glm_vec3_lerp(nightHorizon, dayHorizon, blend, scn->ubo.horizonColor);
    glm_vec3_lerp(nightMidLow, dayMidLow, blend, scn->ubo.midLowColor);
    glm_vec3_lerp(nightNadir, dayNadir, blend, scn->ubo.nadirColor);

    // Animate clouds
    scn->ubo.cloudOffset[0] = time * 0.02f;
    scn->ubo.cloudOffset[1] = time * -0.1f;
    scn->ubo.cloudOffset[2] = time * 0.01f;  // Slower drift in Z

}

static void Destroy(V7V *v7v) {


}

int main(int argc, char **argv) {

    V7V *v7v = V7VCreate("Ugly Invaders", 640, 360, Setup, Loop, Destroy, 0);

    while(!v7v->quit) {
        V7VRender(v7v);
    }

    V7VDestroy(v7v);

    struct timespec ts = {0, 10000 * 1000};
    nanosleep(&ts, NULL);
    printf("EXIT\n");

    return 0;
}

