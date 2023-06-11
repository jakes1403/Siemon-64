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
#define CHANNEL_SFX2    1
#define CHANNEL_MUSIC   2

typedef struct {
    float x, y, z;
    float rotation;
} Object;

void lookAt(Object* obj, float camX, float camZ) {
    float dx = camX - obj->x;
    float dz = camZ - obj->z;
    
    obj->rotation = atan2(dz, dx);
}

const float floor_y = 2.5f;
static float camera_x = cube_size * 2, camera_y = floor_y, camera_z = cube_size * 2;
static float camera_pitch = 0.0f, camera_yaw = 0.0f;

static uint32_t animation = 3283;

const float look_sensitivity = 0.1f;


static surface_t zbuffer;

static bool fog_enabled = true;

static const GLfloat environment_color[] = { 0.1f, 0.03f, 0.2f, 1.f };

// static const GLfloat light_pos[8][4] = {
//     { 1, 0, 0, 0 },
//     { -1, 0, 0, 0 },
//     { 0, 0, 1, 0 },
//     { 0, 0, -1, 0 },
//     { 8, 3, 0, 1 },
//     { -8, 3, 0, 1 },
//     { 0, 3, 8, 1 },
//     { 0, 3, -8, 1 },
// };

static const GLfloat light_diffuse[8][4] = {
    { 1.0f, 0.0f, 0.0f, 1.0f },
    { 0.0f, 1.0f, 0.0f, 1.0f },
    { 0.0f, 0.0f, 1.0f, 1.0f },
    { 1.0f, 1.0f, 0.0f, 1.0f },
    { 1.0f, 0.0f, 1.0f, 1.0f },
    { 0.0f, 1.0f, 1.0f, 1.0f },
    { 1.0f, 1.0f, 1.0f, 1.0f },
    { 1.0f, 1.0f, 1.0f, 1.0f },
};

#define SPRITE_COUNT 24

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
    "rom:/WQ_2.sprite"
};

static GLuint textures[SPRITE_COUNT];

static sprite_t *sprites[SPRITE_COUNT];

// The gravitational constant in m/s^2
#define GRAVITY 9.81
// The time at which the jump reaches its peak in seconds
#define TIME_TO_PEAK 0.5
// The peak height of the jump in meters
#define PEAK_HEIGHT 0.9

float jump_height(float time) {
    // Calculate the initial velocity needed to reach the peak height
    float initial_velocity = GRAVITY * TIME_TO_PEAK;
    
    // The jump height at the given time is determined by the kinematic equation:
    // h(t) = v_0 * t - 0.5 * g * t^2
    // where v_0 is the initial velocity, g is the acceleration due to gravity, 
    // and t is the time since the start of the jump
    float height = initial_velocity * time - 0.5 * GRAVITY * pow(time, 2);
    
    // If the height is negative (which can happen after the "downward" part of the jump),
    // clamp it to 0
    
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



void setup()
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

    float aspect_ratio = (float)display_get_width() / (float)display_get_height();
    float near_plane = 1.0f;
    float far_plane = 50.0f;

    glMatrixMode(GL_PROJECTION);
    glLoadIdentity();
    glFrustum(-near_plane*aspect_ratio, near_plane*aspect_ratio, -near_plane, near_plane, near_plane, far_plane);

    glMatrixMode(GL_MODELVIEW);
    glLoadIdentity();

    glLightModelfv(GL_LIGHT_MODEL_AMBIENT, environment_color);
    glLightModeli(GL_LIGHT_MODEL_LOCAL_VIEWER, GL_TRUE);

    float light_radius = 10.0f;

    for (uint32_t i = 0; i < 8; i++)
    {
        glEnable(GL_LIGHT0 + i);
        glLightfv(GL_LIGHT0 + i, GL_DIFFUSE, light_diffuse[i]);
        glLightf(GL_LIGHT0 + i, GL_LINEAR_ATTENUATION, 2.0f/light_radius);
        glLightf(GL_LIGHT0 + i, GL_QUADRATIC_ATTENUATION, 1.0f/(light_radius*light_radius));
    }

    GLfloat mat_diffuse[] = { 1.0f, 1.0f, 1.0f, 1.0f };
    glMaterialfv(GL_FRONT_AND_BACK, GL_AMBIENT_AND_DIFFUSE, mat_diffuse);

    glFogf(GL_FOG_START, 5);
    glFogf(GL_FOG_END, 20);
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
    int animation_frame = 4 + ((animation / 10) % 10) * 2;
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

    // Mirrored plane
    // Top right with textures[4]
    glBindTexture(GL_TEXTURE_2D, textures[animation_frame]);
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

    // Bottom right with textures[5]
    glBindTexture(GL_TEXTURE_2D, textures[animation_frame + 1]);
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

#define MAX_LENGTH (cube_size * 5)

static float collision_x = 0.0f, collision_y = 0.0f;

static bool unprocessedCollision = false;

void drawCube (float x, float y)
{
    glBindTexture(GL_TEXTURE_2D, textures[0]);
    if (check_collision(1.0f, camera_x, camera_y, camera_z, cube_size * 2, x, cube_size, y))
    {
        collision_x = x;
        collision_y = y;
        unprocessedCollision = true;
    }
    if (calculate_distance(x, y, camera_x, camera_z) < MAX_LENGTH)
    {
        glTranslatef(x, cube_size,y);
        rdpq_debug_log_msg("Cube");
        draw_cube();
        glTranslatef(-x, -cube_size,-y);
    }
    
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

void render()
{
    surface_t *disp = display_get();

    rdpq_attach(disp, &zbuffer);

    gl_context_begin();

    glClearColor(environment_color[0], environment_color[1], environment_color[2], environment_color[3]);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

    float look_x = sin(camera_yaw), look_y = tan(-camera_pitch), look_z = cos(camera_yaw);

    glMatrixMode(GL_MODELVIEW);
    glLoadIdentity();
    gluLookAt(camera_x, camera_y, camera_z,
              camera_x + look_x, camera_y + look_y, camera_z + look_z,
              0.0f, 1.0f, 0.0f);

    float rotation = animation * 0.5f;

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

    static float simon_x = 0.0f, simon_y = 1.5f, simon_z = 6.0f;

    simon_x += (camera_x - simon_x) * 0.005f * perlin2d(simon_x, simon_y, 0.1, 4) * 2.0f;
    simon_y += (camera_y - simon_y) * 0.005f * perlin2d(simon_y, simon_x, 0.1, 4) * 1.5f;
    simon_z += (camera_z - simon_z) * 0.005f * perlin2d(simon_x, simon_y, 0.2, 4) * 1.8f;

    //debugf("%f\n", perlin2d(simon_x, simon_y, 0.2, 4));

    simon_x += sin(rotation*0.01f) * 0.05f;
    simon_y += fabs(sin(rotation*0.01f) * 0.05f);
    simon_z += cos(rotation*0.01f) * 0.05f;
	//$Simon.global_position.x += cos($Timer.time_left * 2.0) * 8.0 * _noise2.get_noise_1d($Timer.time_left)
	//$Simon.global_position.y += sin($Timer.time_left * 2.0) * 4.0 * _noise3.get_noise_1d($Timer.time_left)

    glTranslatef(simon_x, simon_y, simon_z);

    float dx = camera_x - simon_x;
    float dz = camera_z - simon_z;

    static float current_rotation = 0.0f;
    current_rotation += (((atan2(dz, dx) * -(180.0 / PI)) + 90.0f) - current_rotation) * 0.05f * perlin2d(simon_x, simon_y, 0.1, 4);

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

    glPushMatrix();

    glTranslatef(0, 6, 0);
    glRotatef(-rotation*2.46f, 0, 1, 0);

    glDisable(GL_TEXTURE_2D);
    glDisable(GL_CULL_FACE);
    //glDisable(GL_LIGHTING);
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

    rdpq_debug_log_msg("Primitives");
    glColor4f(1, 1, 1, 0.4f);
    prim_test();

    glEnable(GL_CULL_FACE);
    glDisable(GL_BLEND);

    glPopMatrix();

    
    gl_context_end();

    //graphics_set_font_sprite( custom_font );


    rdpq_detach_show();

    
}

float clamp(float val, float min, float max) {
    return val < min ? min : val > max ? max : val;
}

int main(void)
{
	debug_init_isviewer();
	debug_init_usblog();

    int ret = dfs_init(DFS_DEFAULT_LOCATION);
	assert(ret == DFS_ESUCCESS);

	audio_init(44100, 4);
	mixer_init(16);

    display_init(RESOLUTION_320x240, DEPTH_16_BPP, 3, GAMMA_NONE, ANTIALIAS_RESAMPLE_FETCH_ALWAYS);

    wav64_t sfx_monosample;

    wav64_open(&sfx_monosample, "rom:/dungeon_music.wav64");
	wav64_set_loop(&sfx_monosample, true);

    rdpq_init();
    gl_init();

#if DEBUG_RDP
    rdpq_debug_start();
    rdpq_debug_log(true);
#endif

    setup();

    controller_init();

    static int jumpFrameCount = 0;

    static bool doingJump = false;

    //srand((unsigned int)time(NULL)); //seed random number generator with system time
	initialize();      //initialize the maze
	generate();        //generate the maze

    glEnable(GL_FOG);

    wav64_play(&sfx_monosample, CHANNEL_MUSIC);

    debugf("Demo by jakes1403. Modified from the libdragon demo.\n");

#if !DEBUG_RDP
    while (1)
#endif
    {
        controller_scan();
        struct controller_data pressed = get_keys_pressed();
        struct controller_data down = get_keys_down();

        animation++;

        if (down.c[0].A && !doingJump) {
            jumpFrameCount = 0;
            doingJump = true;
        }



        //if (pressed.c[0].A) {
            
        //}

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

        //if (pressed.c[0].B) {
            //animation--;
        ///}

        if (down.c[0].start) {
            debugf("%ld\n", animation);
        }

        // if (down.c[0].R) {
        //     shade_model = shade_model == GL_SMOOTH ? GL_FLAT : GL_SMOOTH;
        //     glShadeModel(shade_model);
        // }

        if (down.c[0].L) {
            fog_enabled = !fog_enabled;
            if (fog_enabled) {
                glEnable(GL_FOG);
            } else {
                glDisable(GL_FOG);
            }
        }

        // if (down.c[0].C_up) {
        //     if (sphere_rings < SPHERE_MAX_RINGS) {
        //         sphere_rings++;
        //     }

        //     if (sphere_segments < SPHERE_MAX_SEGMENTS) {
        //         sphere_segments++;
        //     }

        //     make_sphere_mesh();
        // }

        // if (down.c[0].C_down) {
        //     if (sphere_rings > SPHERE_MIN_RINGS) {
        //         sphere_rings--;
        //     }

        //     if (sphere_segments > SPHERE_MIN_SEGMENTS) {
        //         sphere_segments--;
        //     }
            
        //     make_sphere_mesh();
        // }

        //if (down.c[0].C_right) {
            //texture_index = (texture_index + 1) % 4;
        //}

        float y = pressed.c[0].y / 128.f;
        float x = pressed.c[0].x / 128.f;
        float mag = x*x + y*y;

        if (fabsf(mag) > 0.01f) {
            //distance += y * 0.2f;
            //cam_rotate = cam_rotate - x * 1.2f;

            camera_yaw -= x * look_sensitivity;
            camera_pitch -= y * look_sensitivity;

            //debugf("%f\n", camera_pitch);
            camera_pitch = clamp(camera_pitch, -1.5f, 1.5f);
        }

        static float movement_speed = 0.1f;

        if (down.c[0].up)
        {
            movement_speed += 0.1f;
        }
        if (down.c[0].down)
        {
            movement_speed -= 0.1f;
        }

        if (pressed.c[0].C_up)
        {
            camera_x += sin(camera_yaw) * movement_speed;
            camera_z += cos(camera_yaw) * movement_speed;
        }
        if (pressed.c[0].C_left)
        {
            camera_x += cos(camera_yaw) * movement_speed;
            camera_z -= sin(camera_yaw) * movement_speed;
        }
        if (pressed.c[0].C_right)
        {
            camera_x -= cos(camera_yaw) * movement_speed;
            camera_z += sin(camera_yaw) * movement_speed;
        }
        if (pressed.c[0].C_down)
        {
            camera_x -= sin(camera_yaw) * movement_speed;
            camera_z -= cos(camera_yaw) * movement_speed;
        }

        if (unprocessedCollision)
        {
            debugf("Collision! At x: %f y: %f\n", collision_x, collision_y);
            unprocessedCollision = false;
        }

        render();

        if (audio_can_write()) {    	
			short *buf = audio_write_begin();
			mixer_poll(buf, audio_get_buffer_length());
			audio_write_end();
		}


        

        if (DEBUG_RDP)
            rspq_wait();
    }

}
