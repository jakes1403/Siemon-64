#include <libdragon.h>
#include <GL/gl.h>
#include <GL/glu.h>
#include <GL/gl_integration.h>
#include <malloc.h>
#include <math.h>

#include "cube.h"
#include "sphere.h"
#include "plane.h"
#include "prim_test.h"

#include <math.h>

#include "perlin_noise.h"
#include "maze_gen.h"


// Set this to 1 to enable rdpq debug output.
// The demo will only run for a single frame and stop.
#define DEBUG_RDP 0

#define PI 3.14159265

#define CHANNEL_SFX1    0
#define CHANNEL_VOICE    1
#define CHANNEL_MUSIC   2

static wav64_t sfx_monosample, kill_sample;

static struct controller_data pressed;
static struct controller_data down;

const float floor_y = 2.5f;
static float camera_x, camera_y, camera_z;
static float camera_pitch, camera_yaw;

static uint32_t game_time;
static uint32_t global_time;

const float look_sensitivity = 0.1f;


static surface_t zbuffer;

static bool fog_enabled = true;

static const GLfloat environment_color[] = { 0.1f, 0.03f, 0.2f, 1.f };

#define SPRITE_COUNT 31

static const char *texture_path[SPRITE_COUNT] = {
    "rom:/wallTex.sprite",
    "rom:/floor.sprite",
    "rom:/pentagon0.sprite",
    "rom:/siemon.sprite",
    "rom:/AI_1.sprite",
    "rom:/AI_2.sprite",
    "rom:/E_1.sprite",
    "rom:/E_2.sprite",
    "rom:/etc_1.sprite",
    "rom:/etc_2.sprite",
    "rom:/FV_1.sprite",
    "rom:/FV_2.sprite",
    "rom:/L_1.sprite",
    "rom:/L_2.sprite",
    "rom:/MBP_1.sprite",
    "rom:/MBP_2.sprite",
    "rom:/O_1.sprite",
    "rom:/O_2.sprite",
    "rom:/rest_1.sprite",
    "rom:/rest_2.sprite",
    "rom:/U_1.sprite",
    "rom:/U_2.sprite",
    "rom:/WQ_1.sprite",
    "rom:/WQ_2.sprite",
    "rom:/S.sprite",
    "rom:/I.sprite",
    "rom:/E.sprite",
    "rom:/M.sprite",
    "rom:/O.sprite",
    "rom:/N.sprite",
    "rom:/start.sprite"
};

static GLuint textures[SPRITE_COUNT];

static sprite_t *sprites[SPRITE_COUNT];

// The gravitational constant in m/s^2
#define GRAVITY 15.0
// The time at which the jump reaches its peak in seconds, decreased to make the jump quicker
#define TIME_TO_PEAK 0.4
// The peak height of the jump in meters, decreased to make the jump shorter
#define PEAK_HEIGHT 0.7

float jump_height(float time) {
    // Calculate the initial velocity needed to reach the peak height
    float initial_velocity = GRAVITY * TIME_TO_PEAK;

    // The jump height at the given time is determined by the kinematic equation:
    // h(t) = v_0 * t - 0.5 * g * t^2
    // where v_0 is the initial velocity, g is the acceleration due to gravity, 
    // and t is the time since the start of the jump
    float height = initial_velocity * time - 0.5 * GRAVITY * pow(time, 2);

    return height;
}

void load_texture(GLenum target, sprite_t *sprite)
{
    for (uint32_t i = 0; i < 7; i++)
    {
        surface_t surf = sprite_get_lod_pixels(sprite, i);
        if (!surf.buffer) break;

        glTexImageN64(target, i, &surf);
    }
}



void setup_renderer()
{
    zbuffer = surface_alloc(FMT_RGBA16, display_get_width(), display_get_height());

    for (uint32_t i = 0; i < SPRITE_COUNT; i++)
    {
        sprites[i] = sprite_load(texture_path[i]);
    }

    setup_sphere();
    make_sphere_mesh();

    setup_cube();

    setup_plane();
    make_plane_mesh();

    glEnable(GL_DEPTH_TEST);
    glEnable(GL_CULL_FACE);
    glEnable(GL_NORMALIZE);

    glMatrixMode(GL_PROJECTION);
    glLoadIdentity();

    glMatrixMode(GL_MODELVIEW);
    glLoadIdentity();

    GLfloat mat_diffuse[] = { 1.0f, 1.0f, 1.0f, 1.0f };
    glMaterialfv(GL_FRONT_AND_BACK, GL_AMBIENT_AND_DIFFUSE, mat_diffuse);

    glFogf(GL_FOG_START, 2);
    glFogf(GL_FOG_END, 30);
    glFogfv(GL_FOG_COLOR, environment_color);

    glGenTextures(SPRITE_COUNT, textures);

    #if 0
    GLenum min_filter = GL_LINEAR_MIPMAP_LINEAR;
    #else
    GLenum min_filter = GL_LINEAR;
    #endif


    for (uint32_t i = 0; i < SPRITE_COUNT; i++)
    {
        if (i < 4)
        {
            glBindTexture(GL_TEXTURE_2D, textures[i]);

            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, min_filter);

            load_texture(GL_TEXTURE_2D, sprites[i]);
        }
        else
        {
            glBindTexture(GL_TEXTURE_2D, textures[i]);

            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);

            load_texture(GL_TEXTURE_2D, sprites[i]);
        }
    }
}


void draw_quad()
{
    int animation_frame = 4 + ((game_time / 10) % 10) * 2;
    // Original plane
    // Top left with textures[4]
    glBindTexture(GL_TEXTURE_2D, textures[animation_frame]);
    glBegin(GL_TRIANGLE_STRIP);
        glNormal3f(0, 1, 0);
        glTexCoord2f(0, 0);
        glVertex3f(-1, 1, 0);
        glTexCoord2f(0, 1);
        glVertex3f(-1, 0, 0);
        glTexCoord2f(1, 0);
        glVertex3f(0, 1, 0);
        glTexCoord2f(1, 1);
        glVertex3f(0, 0, 0);
    glEnd();

    // Mirrored plane
    // Top right with textures[4]
    glBegin(GL_TRIANGLE_STRIP);
        glNormal3f(0, 1, 0);
        glTexCoord2f(1, 0); // Reverse texture mapping
        glVertex3f(0, 1, 0);
        glTexCoord2f(1, 1); // Reverse texture mapping
        glVertex3f(0, 0, 0);
        glTexCoord2f(0, 0); // Reverse texture mapping
        glVertex3f(1, 1, 0);
        glTexCoord2f(0, 1); // Reverse texture mapping
        glVertex3f(1, 0, 0);
    glEnd();

    // Bottom left with textures[5]
    glBindTexture(GL_TEXTURE_2D, textures[animation_frame + 1]);
    glBegin(GL_TRIANGLE_STRIP);
        glNormal3f(0, 1, 0);
        glTexCoord2f(0, 0);
        glVertex3f(-1, 0, 0);
        glTexCoord2f(0, 1);
        glVertex3f(-1, -1, 0);
        glTexCoord2f(1, 0);
        glVertex3f(0, 0, 0);
        glTexCoord2f(1, 1);
        glVertex3f(0, -1, 0);
    glEnd();

    // Bottom right with textures[5]
    glBegin(GL_TRIANGLE_STRIP);
        glNormal3f(0, 1, 0);
        glTexCoord2f(1, 0); // Reverse texture mapping
        glVertex3f(0, 0, 0);
        glTexCoord2f(1, 1); // Reverse texture mapping
        glVertex3f(0, -1, 0);
        glTexCoord2f(0, 0); // Reverse texture mapping
        glVertex3f(1, 0, 0);
        glTexCoord2f(0, 1); // Reverse texture mapping
        glVertex3f(1, -1, 0);
    glEnd();
}

float calculate_distance(float p1_x, float p1_y, float p2_x, float p2_y) {
    double x_diff = p1_x - p2_x;
    double y_diff = p1_y - p2_y;

    return sqrt(x_diff * x_diff + y_diff * y_diff);
}


int check_collision(float r, float x, float y, float z, float s, float cx, float cy, float cz) {
    float half_s = s / 2;
    // Calculate the distance from the sphere center to the closest point in the cube
    float dist_sq = 0;
    
    // Adjust the sphere's coordinates relative to the cube's center
    x -= cx;
    y -= cy;
    z -= cz;

    if (x < -half_s) dist_sq += pow(x + half_s, 2);
    else if (x > half_s) dist_sq += pow(x - half_s, 2);

    if (y < -half_s) dist_sq += pow(y + half_s, 2);
    else if (y > half_s) dist_sq += pow(y - half_s, 2);

    if (z < -half_s) dist_sq += pow(z + half_s, 2);
    else if (z > half_s) dist_sq += pow(z - half_s, 2);

    return dist_sq <= r * r;
}

#define MAX_LENGTH (cube_size * 6)

struct Maze_Segment {
    float x, y;
};

static uint32_t number_segments;

struct Maze_Segment maze_segments[(MAX_LENGTH * 2) * (MAX_LENGTH * 2)];

typedef struct Collision_Info {
    float x, y;
    bool hadCollision;
} Collision_Info;

Collision_Info check_for_maze_collision()
{
    Collision_Info inf;
    inf.hadCollision = false;
    for (uint32_t i = 0; i < number_segments; i++)
    {
        if (check_collision(2.0f, camera_x, camera_y, camera_z, cube_size * 2, maze_segments[i].x, cube_size, maze_segments[i].y))
        {
            inf.x = maze_segments[i].x;
            inf.y = maze_segments[i].y;
            inf.hadCollision = true;
        }
    }
    return inf;
}

void drawCube (float x, float y)
{
    glBindTexture(GL_TEXTURE_2D, textures[0]);
    if (calculate_distance(x, y, camera_x, camera_z) < MAX_LENGTH)
    {
        glTranslatef(x, cube_size,y);
        rdpq_debug_log_msg("Cube");
        draw_cube();
        glTranslatef(-x, -cube_size,-y);
    }
    maze_segments[number_segments].x = x;
    maze_segments[number_segments].y = y;

    number_segments++;
    
}


void drawFloor (float x, float y)
{
    glBindTexture(GL_TEXTURE_2D, textures[1]);
    if (calculate_distance(x, y, camera_x, camera_z) < MAX_LENGTH)
    {
        glTranslatef(x, 0,y);
        draw_plane();
        glTranslatef(-x, 0,-y);
    }
    
}

#define WHITE drawFloor(x * cube_size * 2, y * cube_size * 2)
#define BLACK drawCube(x * cube_size * 2, y * cube_size * 2)
#define RED

#define max(a,b) (((a) > (b)) ? (a) : (b))
#define min(a,b) (((a) < (b)) ? (a) : (b))

void renderMaze(int xspecial, int yspecial){
	int x, y;
    int width=(xsize-1)*2-1;
    int height=(ysize-1)*2-1;

    int adjustedCameraX = (int)(camera_x / cube_size / 2);
    int adjustedCameraY = (int)(camera_y / cube_size / 2);

    int lowerBoundX = max(0, adjustedCameraX - (int)MAX_LENGTH);
    int upperBoundX = min(width - 1, adjustedCameraX + (int)MAX_LENGTH);

    int lowerBoundY = max(0, adjustedCameraY - (int)MAX_LENGTH);
    int upperBoundY = min(height - 1, adjustedCameraY + (int)MAX_LENGTH);
    number_segments = 0;

    //Actual writing of data begins here:
    for(y = lowerBoundY; y <= upperBoundY; y++){
        for(x = lowerBoundX; x <= upperBoundX; x++){
            if(x%2 == 1 && y%2 == 1){
                if(x/2+1 == xspecial && y/2+1 == yspecial) RED;
                else{
                    if(MAZE[x/2+1][y/2+1].in) WHITE; else BLACK;
                }
            }else if(x%2 == 0 && y%2 == 0){
                BLACK;
            }else if(x%2 == 0 && y%2 == 1){
                if(MAZE[x/2+1][y/2+1].left) BLACK; else WHITE;
            }else if(x%2 == 1 && y%2 == 0){
                if(MAZE[x/2+1][y/2+1].up) BLACK; else WHITE;
            }
        }
    }
	return;
}

static float simon_x, simon_y, simon_z;

static float rotation;

static float current_rotation = 0.0f;

float distance3D(float x1, float y1, float z1, float x2, float y2, float z2) {
    float dx = x2 - x1;
    float dy = y2 - y1;
    float dz = z2 - z1;
    
    return sqrt(dx*dx + dy*dy + dz*dz);
}

void drawCubeBetweenPoints(float x1, float y1, float z1, float x2, float y2, float z2) {
    float dx = x2 - x1;
    float dy = y2 - y1;
    float dz = z2 - z1;

    float cx = (x1 + x2) / 2;
    float cy = (y1 + y2) / 2;
    float cz = (z1 + z2) / 2;

    float d = sqrt(dx * dx + dy * dy + dz * dz); // Distance between points
    float ax = 57.2957795*acos( dz/d ); // The angle in the x-y plane 
    float ay = 57.2957795*atan2( dy, dx ); // The angle in the x-z plane 

    glPushMatrix();

    // Translate the cube to center
    glTranslatef(cx, cy, cz);

    // Rotate the cube
    glRotatef(ax, -1.0, 0.0, 0.0);
    glRotatef(ay, 0.0, 1.0, 0.0);

    // Scale the cube
    glScalef(2, d, 2); // Assuming you want to keep the width and depth as 2

    // Draw the cube
    draw_cube();

    glPopMatrix();
}

static const float distance_cutoff = 25.0f;

void render_game()
{

    gl_context_begin();

    glClearColor(environment_color[0], environment_color[1], environment_color[2], environment_color[3]);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

    glMatrixMode(GL_PROJECTION);
    glLoadIdentity();
    glOrtho(0.0, display_get_width(), display_get_height(), 0.0, -1.0, 1.0);
    glMatrixMode(GL_MODELVIEW);
    glLoadIdentity();

    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    
    float distance_from_simon = distance3D(simon_x, simon_y, simon_z, camera_x, camera_y, camera_z);

    float red_value = 1.0f;

    if (distance_from_simon < distance_cutoff)
    {
        red_value = distance_from_simon / distance_cutoff;
    }

    // The color you want to tint with (RGBA)
    float red = 1.0f;   // range: 0-1
    float green = red_value; // range: 0-1
    float blue = red_value;  // range: 0-1
    float alpha = 1.0f; // range: 0-1, 0 means fully transparent, 1 means fully opaque

    glColor4f(red, green, blue, alpha);
    glBegin(GL_QUADS);
    glVertex2i(0, 0);
    glVertex2i(display_get_width(), 0);
    glVertex2i(display_get_width(), display_get_height());
    glVertex2i(0, display_get_height());
    glEnd();

    glDisable(GL_BLEND);

    float aspect_ratio = (float)display_get_width() / (float)display_get_height();
    float near_plane = 1.0f;
    float far_plane = 50.0f;

    glMatrixMode(GL_PROJECTION);
    glLoadIdentity();
    glFrustum(-near_plane*aspect_ratio, near_plane*aspect_ratio, -near_plane, near_plane, near_plane, far_plane);

    float look_x = sin(camera_yaw), look_y = tan(-camera_pitch), look_z = cos(camera_yaw);

    glMatrixMode(GL_MODELVIEW);
    glLoadIdentity();
    gluLookAt(camera_x, camera_y, camera_z,
              camera_x + look_x, camera_y + look_y, camera_z + look_z,
              0.0f, 1.0f, 0.0f);

    glPushMatrix();

    glRotatef(rotation*5.43f, 0, 1, 0);

    // for (uint32_t i = 0; i < 8; i++)
    // {
    //     glLightfv(GL_LIGHT0 + i, GL_POSITION, light_pos[i]);
    // }

    glPopMatrix();

    //glBindTexture(GL_TEXTURE_2D, textures[texture_index]);

    //glEnable(GL_LIGHTING);
    glEnable(GL_TEXTURE_2D);

    //glEnable(GL_COLOR_MATERIAL);
    glPushMatrix();

    rdpq_debug_log_msg("Plane");
    //draw_plane();

    // for (uint32_t i = 0; i < 10; i++)
    // {
    //     glTranslatef(i,sin(rotation * 0.23f * (i + 1)),i * sin(rotation * 0.13f));
    //     rdpq_debug_log_msg("Cube");
    //     draw_cube();
    // }

    renderMaze(0, 0);

    glPopMatrix();

    glPushMatrix();


	//$Simon.global_position.x += cos($Timer.time_left * 2.0) * 8.0 * _noise2.get_noise_1d($Timer.time_left)
	//$Simon.global_position.y += sin($Timer.time_left * 2.0) * 4.0 * _noise3.get_noise_1d($Timer.time_left)

    glTranslatef(simon_x, simon_y, simon_z);

    // float dx = camera_x - simon_x;
    // float dz = camera_z - simon_z;
    
    glRotatef(current_rotation, 0, 1, 0);

    //glRotatef(90.0f, 1, 0, 0);

    glScalef(10, 10, 10);
    //glColor4f(1.0f, 0.4f, 0.2f, 1.0f);
    //glDepthFunc(GL_EQUAL);
    //glDepthMask(GL_FALSE);
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    //glDisable(GL_LIGHTING);
    rdpq_debug_log_msg("Decal");

    draw_quad();
    //glDepthMask(GL_TRUE);
    //glDepthFunc(GL_LESS);
    glDisable(GL_BLEND);
    //glEnable(GL_LIGHTING);
    glPopMatrix();

    //glDisable(GL_COLOR_MATERIAL);

    // glPushMatrix();

    // glRotatef(rotation*0.23f, 1, 0, 0);
    // glRotatef(rotation*0.98f, 0, 0, 1);
    // glRotatef(rotation*1.71f, 0, 1, 0);

    // glBindTexture(GL_TEXTURE_2D, textures[(texture_index + 1)%4]);

    // glCullFace(GL_FRONT);
    // rdpq_debug_log_msg("Sphere");
    // draw_sphere();
    // glCullFace(GL_BACK);

    // glPopMatrix();

    //glBindTexture(GL_TEXTURE_2D, textures[2]);

    //drawCubeBetweenPoints(simon_x, simon_y, simon_z, camera_x, camera_y - 1.0, camera_z);


    gl_context_end();
}

float clamp(float val, float min, float max) {
    return val < min ? min : val > max ? max : val;
}

static bool hasDone = false;

static float prev_camera_x;
static float prev_camera_z;

void step_through_game()
{
    static int jumpFrameCount = 0;

    static bool doingJump = false;

    game_time++;

    if (down.c[0].A && !doingJump) {
        jumpFrameCount = 0;
        doingJump = true;
    }

    if (doingJump)
    {
        float jump_val = jump_height(jumpFrameCount++ / 60.0f);
        

        if (jump_val < 0.0f)
        {
            doingJump = false;
        }
        else
        {
            camera_y = floor_y + jump_val;
        }
    }

    if (down.c[0].L) {
        fog_enabled = !fog_enabled;
        if (fog_enabled) {
            glEnable(GL_FOG);
        } else {
            glDisable(GL_FOG);
        }
    }

    float y = pressed.c[0].y / 128.f;
    float x = pressed.c[0].x / 128.f;
    float mag = x*x + y*y;

    if (fabsf(mag) > 0.01f) {
        //distance += y * 0.2f;
        //cam_rotate = cam_rotate - x * 1.2f;
        // Yaw is left and right
        camera_yaw -= x * look_sensitivity;
        camera_pitch -= y * look_sensitivity;

        //debugf("%f\n", camera_pitch);
        camera_pitch = clamp(camera_pitch, -1.5f, 1.5f);
    }

    static float movement_speed = 0.3f;

    if (pressed.c[0].C_up)
    {
        prev_camera_x = camera_x;
        prev_camera_z = camera_z;

        camera_x += sin(camera_yaw) * movement_speed;
        camera_z += cos(camera_yaw) * movement_speed;

        Collision_Info collision = check_for_maze_collision();

        // Check if there's been a collision
        if (collision.hadCollision)
        {
            debugf("Collision! At x: %f y: %f\n", collision.x, collision.y);

            // If there's been a collision, move player back to the previous position
            camera_x = prev_camera_x;
            camera_z = prev_camera_z;
        }
    }
    if (pressed.c[0].C_left)
    {
        prev_camera_x = camera_x;
        prev_camera_z = camera_z;
        
        camera_x += cos(camera_yaw) * movement_speed;
        camera_z -= sin(camera_yaw) * movement_speed;

        Collision_Info collision = check_for_maze_collision();

        // Check if there's been a collision
        if (collision.hadCollision)
        {
            debugf("Collision! At x: %f y: %f\n", collision.x, collision.y);

            // If there's been a collision, move player back to the previous position
            camera_x = prev_camera_x;
            camera_z = prev_camera_z;
        }
    }
    if (pressed.c[0].C_right)
    {
        prev_camera_x = camera_x;
        prev_camera_z = camera_z;

        camera_x -= cos(camera_yaw) * movement_speed;
        camera_z += sin(camera_yaw) * movement_speed;

        Collision_Info collision = check_for_maze_collision();

        // Check if there's been a collision
        if (collision.hadCollision)
        {
            debugf("Collision! At x: %f y: %f\n", collision.x, collision.y);

            // If there's been a collision, move player back to the previous position
            camera_x = prev_camera_x;
            camera_z = prev_camera_z;
        }
    }
    if (pressed.c[0].C_down)
    {
        prev_camera_x = camera_x;
        prev_camera_z = camera_z;
        
        camera_x -= sin(camera_yaw) * movement_speed;
        camera_z -= cos(camera_yaw) * movement_speed;

        Collision_Info collision = check_for_maze_collision();

        // Check if there's been a collision
        if (collision.hadCollision)
        {
            debugf("Collision! At x: %f y: %f\n", collision.x, collision.y);

            // If there's been a collision, move player back to the previous position
            camera_x = prev_camera_x;
            camera_z = prev_camera_z;
        }
    }

    simon_x += (camera_x - simon_x) * 0.005f * perlin2d(simon_x, simon_y, 0.1, 4) * 2.0f;
    simon_y += (camera_y - simon_y) * 0.005f * perlin2d(simon_y, simon_x, 0.1, 4) * 1.5f;
    simon_z += (camera_z - simon_z) * 0.005f * perlin2d(simon_x, simon_y, 0.2, 4) * 1.8f;

    //debugf("%f\n", perlin2d(simon_x, simon_y, 0.2, 4));

    rotation = game_time * 0.5f;

    simon_x += sin(rotation*0.01f) * 0.05f;
    simon_y += fabs(sin(rotation*0.01f) * 0.05f);
    simon_z += cos(rotation*0.01f) * 0.05f;

    float dx = camera_x - simon_x;
    float dz = camera_z - simon_z;

    float target_rotation = atan2(dz, dx) * -(180.0 / PI) + 90.0f;

    // calculate the difference and make sure it stays in the -180 to 180 range
    float rotation_difference = fmodf((target_rotation - current_rotation + 180.0f), 360.0f) - 180.0f;
    current_rotation += rotation_difference * 0.05f * perlin2d(simon_x, simon_y, 0.1, 4);

    if (distance3D(simon_x, simon_y, simon_z, camera_x, camera_y, camera_z) < distance_cutoff)
    {
        if (!hasDone)
        {
            hasDone = true;
            if (rand() % 100 < 30)
            {
                wav64_play(&kill_sample, CHANNEL_VOICE);
            }
        }
        
    }
    else
    {
        hasDone = false;
    }
}

void setup_game()
{
    game_time = 3283;
    camera_x = cube_size * 2, camera_y = floor_y, camera_z = cube_size * 2;

    camera_pitch = 0.0f, camera_yaw = 0.0f;

    simon_x = 0.0f, simon_y = 1.5f, simon_z = 60.0f;

    srand(global_time); //seed random number generator with system time
	initialize_maze_gen();      //initialize the maze
	generate_maze();        //generate the maze
}

// RANDN(n): generate a random number from 0 to n-1
#define RANDN(n) ({ \
	__builtin_constant_p((n)) ? \
		(rand()%(n)) : \
		(uint32_t)(((uint64_t)rand() * (n)) >> 32); \
})


int main(void)
{
	debug_init_isviewer();
	debug_init_usblog();

    int ret = dfs_init(DFS_DEFAULT_LOCATION);
	assert(ret == DFS_ESUCCESS);

	audio_init(44100, 4);
	mixer_init(16);

    display_init(RESOLUTION_320x240, DEPTH_16_BPP, 3, GAMMA_NONE, ANTIALIAS_RESAMPLE_FETCH_ALWAYS);

    wav64_open(&sfx_monosample, "rom:/dungeon_music.wav64");
	wav64_set_loop(&sfx_monosample, true);

    wav64_open(&kill_sample, "rom:/simon_kills.wav64");

    rdpq_init();
    gl_init();

    global_time = 0;

    setup_renderer();

    controller_init();

    glEnable(GL_FOG);

    wav64_play(&sfx_monosample, CHANNEL_MUSIC);

    static bool is_paused = false;

    //setup_game();

    bool is_main_menu = true;

    debugf("Demo by jakes1403. Modified from the libdragon demo.\n");

    rdpq_font_t *fnt1 = rdpq_font_load("rom:/Pacifico.font64");

    sprite_t* tiles_sprite = sprite_load("rom:/tiles.sprite");


    surface_t tiles_surf = sprite_get_pixels(tiles_sprite);

    // Create a block for the background, so that we can replay it later.
    rspq_block_begin();

    // Check if the sprite was compiled with a paletted format. Normally
    // we should know this beforehand, but for this demo we pretend we don't
    // know. This also shows how rdpq can transparently work in both modes.
    bool tlut = false;
    tex_format_t tiles_format = sprite_get_format(tiles_sprite);
    if (tiles_format == FMT_CI4 || tiles_format == FMT_CI8) {
        // If the sprite is paletted, turn on palette mode and load the
        // palette in TMEM. We use the mode stack for demonstration,
        // so that we show how a block can temporarily change the current
        // render mode, and then restore it at the end.
        rdpq_mode_push();
        rdpq_mode_tlut(TLUT_RGBA16);
        rdpq_tex_upload_tlut(sprite_get_palette(tiles_sprite), 0, 16);
        tlut = true;
    }
    uint32_t tile_width = tiles_sprite->width / tiles_sprite->hslices;
    uint32_t tile_height = tiles_sprite->height / tiles_sprite->vslices;

    tile_width *= 2;
    tile_height *= 2;
 
    for (uint32_t ty = 0; ty < display_get_height(); ty += tile_height)
    {
        for (uint32_t tx = 0; tx < display_get_width(); tx += tile_width)
        {
            // Load a random tile among the 4 available in the texture,
            // and draw it as a rectangle.
            // Notice that this code is agnostic to both the texture format
            // and the render mode (standard vs copy), it will work either way.
            int s = RANDN(2)*32, t = RANDN(2)*32;
            rdpq_tex_upload_sub(TILE0, &tiles_surf, NULL, s, t, s+32, t+32);
            rdpq_texture_rectangle(TILE0, tx, ty, tx+32, ty+32, s, t);
        }
    }
    
    // Pop the mode stack if we pushed it before
    if (tlut) rdpq_mode_pop();
    rspq_block_t* tiles_block = rspq_block_end();

    while (1)
    {
        surface_t *disp = display_get();

        rdpq_attach_clear(disp, &zbuffer);
        global_time++;
        controller_scan();
        pressed = get_keys_pressed();
        down = get_keys_down();

        if (!is_main_menu)
        {
            if (down.c[0].start) {
                is_paused = !is_paused;
            }
            if (down.c[0].right) {
                setup_game();
            }
            if (!is_paused)
            {
                step_through_game();
            }
            
            render_game();

            if (is_paused)
            {
                graphics_set_color( 0xFFFFFFFF, 0x0 );

                graphics_draw_text( disp, 20, 20, "Pause" );
            }
        }
        
        if (is_main_menu)
        {
            rdpq_set_mode_copy(false);
            // rdpq_set_mode_standard();
            rspq_block_run(tiles_block);

            rdpq_set_mode_standard();
            rdpq_mode_filter(FILTER_POINT);
            rdpq_mode_alphacompare(1); 

            for (int i = 0; i < 6; i++)
            {
                float scaling = 1.0 + fabs(sin((global_time / 100.0f) + i));
                rdpq_sprite_blit(sprites[24 + i], 5 + (50 * i), 30 + 10 * sin((global_time / 50.0f) + i), &(rdpq_blitparms_t){
                    .scale_x = scaling, .scale_y = scaling, .theta = 0.2 * sin((global_time / 60.0f) + i),
                });
            }

            rdpq_sprite_blit(sprites[30], 160, 200, &(rdpq_blitparms_t){
                .scale_x = 1.0, .scale_y = 1.0, .theta = 0.2 * sin((global_time / 60.0f))
            });

            rdpq_font_begin(RGBA32(0xED, 0xAE, 0x49, 0xFF));
            rdpq_font_position(20, 50);
            rdpq_font_scale(2.0, 2.0);
            rdpq_font_print(fnt1, "Begin");
            rdpq_font_end();

            if (down.c[0].start) {
                is_main_menu = false;
                setup_game();
            }
        }

        //graphics_draw_text( disp, 0, 0, "Using custom font!" );

        //graphics_set_color( 0xFFFFFF00, 0x0 );

        //graphics_draw_text( disp, 20, 20, "Using default font!" );


        rdpq_detach_show();

        if (audio_can_write()) {    	
			short *buf = audio_write_begin();
			mixer_poll(buf, audio_get_buffer_length());
			audio_write_end();
		}


    }

}
