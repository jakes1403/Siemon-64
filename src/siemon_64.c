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

#include "papagayo_parse.h"



// Set this to 1 to enable rdpq debug output.
// The demo will only run for a single frame and stop.
#define DEBUG_RDP 0

#define PI 3.1415926535897932385

#define CHANNEL_SFX1    0
#define CHANNEL_VOICE    1
#define CHANNEL_MUSIC   2

static wav64_t sfx_monosample;

static struct controller_data pressed;
static struct controller_data down;

const float floor_y = 2.5f;
static float camera_x, camera_y, camera_z;
static float camera_pitch, camera_yaw;

static uint32_t game_time;
static uint32_t global_time;

const float look_sensitivity = 0.1f;


static surface_t zbuffer;


static rdpq_font_t *fnt1;

static const GLfloat environment_color[] = { 0.1f, 0.03f, 0.2f, 1.f };

static const GLfloat environment_color_heaven[] = { 1.0f, 1.0f, 1.0f, 1.f };

bool is_in_heaven = true;

wav64_t step_sfx;

#define SPRITE_COUNT 31
#define MAP_SIZE 10

typedef struct {
    const char *name;
    int index;
} TextureMap;

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

TextureMap texture_map[MAP_SIZE]; // map array

void build_texture_map() {
    const char *names[MAP_SIZE] = {"AI", "E", "etc", "FV", "L", "MBP", "O", "rest", "U", "WQ"};
    
    for(int i = 0; i < SPRITE_COUNT; ++i) {
        for(int j = 0; j < MAP_SIZE; ++j) {
            // find the last '/' in the string and get the substring after it
            const char *name = strrchr(texture_path[i], '/') + 1;
            // check if the name starts with the given name and ends with "_1.sprite"
            if(strncmp(name, names[j], strlen(names[j])) == 0 && strcmp(name + strlen(names[j]), "_1.sprite") == 0) {
                texture_map[j].name = names[j];
                texture_map[j].index = i;
                break;
            }
        }
    }
}

#define MESSAGE_COUNT 303

static const char *messages[MESSAGE_COUNT] = {
    "Siemon is near. Despair rises.",
    "Fear his name. Siemon.",
    "His shadow engulfs. Siemon.",
    "Screen bleeds. He's near.",
    "The end is nigh. Siemon.",
    "His laugh echoes. It's him.",
    "Maze trembles. Siemon lurks.",
    "He's here. Siemon, the tormentor.",
    "His presence darkens the maze.",
    "His laughter, your death knell.",
    "Hope shatters. Siemon's here.",
    "His name freezes your blood.",
    "Sanctuary crumbles. Siemon arrives.",
    "Siemon looms. Game over.",
    "His laughter chills your bones.",
    "His shadow distorts reality.",
    "He's closer. Run or perish.",
    "Siemon feeds on your fear.",
    "The dread amplifies. He's near.",
    "Endgame is here. Siemon.",
    "Siemon's presence: overwhelming.",
    "Screen reddens. Fear him.",
    "A creeping dread. Siemon.",
    "Your pulse quickens. He's near.",
    "Darkness deepens. Siemon approaches.",
    "Your doom, spelled Siemon.",
    "Terror seizes you. He's near.",
    "Labyrinth shakes. Siemon is here.",
    "There's no refuge from Siemon.",
    "He's your nightmare. Siemon.",
    "Path ends. Siemon looms.",
    "His eyes, twin suns of terror.",
    "His form: harbinger of doom.",
    "Siemon's shadow devours hope.",
    "He's the chill in the air.",
    "Siemon. The end of sanctuary.",
    "His approach: relentless.",
    "Siemon: the predator draws near.",
    "Cold touch in the dark: Siemon.",
    "Terror lurks. Siemon is near.",
    "Nowhere is safe. He's here.",
    "His laugh resonates. Siemon.",
    "Chase intensifies. Siemon's here.",
    "Your terror, his delight.",
    "Hope fades. Siemon's near.",
    "No escape. Siemon has come.",
    "Labyrinth trembles. He approaches.",
    "His laugh haunts your path.",
    "His name poisons the air.",
    "He waits in the dark. Siemon.",
    "Fear. Despair. Siemon.",
    "Siemon's eyes pierce the darkness.",
    "The path narrows. He's here.",
    "He lurks in every shadow.",
    "Your dread is his joy.",
    "Fear swells. Siemon's near.",
    "The echo of dread: Siemon.",
    "The end approaches. Siemon.",
    "His darkness is inescapable.",
    "Your time runs out. Siemon.",
    "His laughter, your despair.",
    "Shadows dance. Siemon's near.",
    "He's the phantom in the maze.",
    "Every echo: Siemon.",
    "Siemon's here. No way out.",
    "His laughter, a death sentence.",
    "He's the scream in the silence.",
    "End is near. Siemon lurks.",
    "His presence: suffocating.",
    "Siemon's shadow: inescapable.",
    "He's closer. Terror spikes.",
    "His arrival, inevitable.",
    "His name echoes. Siemon.",
    "His face looms in the dark.",
    "Maze whispers his name. Siemon.",
    "No sanctuary. Only Siemon.",
    "Siemon's near. Escape fades.",
    "His laughter fills the maze.",
    "No solace in the maze. Siemon.",
    "He's the shiver down your spine.",
    "Fear permeates. Siemon's near.",
    "Monstrous form emerges. Siemon.",
    "No place is safe. Siemon.",
    "He's the predator. You're prey.",
    "Every shadow whispers his name.",
    "Your end is near. Siemon.",
    "Siemon. A nightmare in red.",
    "The maze tightens. Siemon.",
    "Echoes of dread. Siemon lurks.",
    "His presence distorts the air.",
    "No refuge. Siemon is here.",
    "He's the ghost in the maze.",
    "Sanctuary vanishes. Siemon looms.",
    "The maze breathes his name.",
    "His eyes pierce your soul.",
    "Despair seeps in. Siemon nears.",
    "He's the devil in the maze.",
    "Screen darkens. He's close.",
    "Escape is futile. Siemon.",
    "He's the shadow in your mind.",
    "Terror lurks. Siemon looms.",
    "His laughter deafens you.",
    "Siemon. The end is here.",
    "His laugh chills your soul.",
    "Darkness intensifies. Siemon nears.",
    "He's the chill down your spine.",
    "Maze whispers. He's near.",
    "He lurks in the shadows. Siemon.",
    "He's your nemesis. Siemon.",
    "Siemon's laughter, a death knell.",
    "Darkness gathers. Siemon approaches.",
    "His shadow covers all. Siemon.",
    "He's your demise. Siemon.",
    "His laughter shatters your hope.",
    "He's the echo in the maze. Siemon.",
    "No hiding. He's here. Siemon.",
    "His presence dominates. Siemon.",
    "No sanctuary. Siemon's near.",
    "His laughter chokes the air.",
    "Siemon. He's here. Nowhere to hide.",
    "His shadow engulfs all. Siemon.",
    "No escape. He's here. Siemon.",
    "He's the dread in the labyrinth. Siemon.",
    "Siemon. His laughter haunts the maze.",
    "His face haunts your path.",
    "His name chills the air. Siemon.",
    "The red tints your terror. Siemon.",
    "Siemon. Your path ends here.",
    "He's the darkness in the labyrinth.",
    "His presence, a cold touch. Siemon.",
    "Your doom awaits. Siemon.",
    "He's the whisper in the wind. Siemon.",
    "Siemon. The name freezes your heart.",
    "The darkness gathers. Siemon is near.",
    "His name haunts the labyrinth. Siemon.",
    "He's the phantom that chases. Siemon.",
    "Siemon. His eyes pierce the shadows.",
    "The labyrinth tightens. Siemon looms.",
    "Siemon. He's the terror in the dark.",
    "No safety. Only him. Siemon.",
    "He's the shadow in your nightmares.",
    "The labyrinth shivers. Siemon approaches.",
    "Siemon. His name echoes in your fear.",
    "Your terror, his delight. Siemon.",
    "He's the cold wind in the labyrinth.",
    "Siemon's shadow distorts reality.",
    "He's the beast in the labyrinth. Siemon.",
    "No way out. Siemon approaches.",
    "His laugh echoes in your mind.",
    "The maze turns red. He's here.",
    "His eyes, twin stars of terror.",
    "He's the monster in the maze. Siemon.",
    "The air freezes. Siemon is near.",
    "His name, a curse. Siemon.",
    "He's the chill in the air. Siemon.",
    "Dread fills the air. Siemon.",
    "Siemon. He's the echo in the labyrinth.",
    "His laugh, a haunting melody.",
    "No place to hide. Siemon is here.",
    "Your demise is near. Siemon.",
    "His name, a chilling echo. Siemon.",
    "He's the darkness in your path. Siemon.",
    "No solace. Only Siemon.",
    "He's the threat in the labyrinth.",
    "Fear his name. Siemon.",
    "He's the terror that lurks. Siemon.",
    "Your doom is spelled: Siemon.",
    "The labyrinth shudders. Siemon is near.",
    "Your time is running out. Siemon.",
    "No sanctuary in the maze. Siemon.",
    "His presence, a chilling wind.",
    "Siemon. He's the ghost in the maze.",
    "His shadow, a dreadful sight.",
    "Despair mounts. Siemon nears.",
    "He's the echo in the labyrinth.",
    "No rest. Siemon is near.",
    "His laugh, a harrowing sound.",
    "His face haunts the labyrinth. Siemon.",
    "Siemon. The end is imminent.",
    "His laugh, a cold chill. Siemon.",
    "He's the hunter in the maze. Siemon.",
    "The labyrinth turns cold. Siemon.",
    "His name is the echo. Siemon.",
    "No respite in the maze. Siemon.",
    "His eyes pierce the darkness. Siemon.",
    "The dread intensifies. Siemon.",
    "His name, an omen. Siemon.",
    "He's the stalker in the maze. Siemon.",
    "Your fear is his delight. Siemon.",
    "He's the face in the shadows. Siemon.",
    "No refuge from Siemon.",
    "His shadow, a looming threat.",
    "No escape. Siemon lurks.",
    "His name, a cold shiver. Siemon.",
    "He's the beast in the shadows. Siemon.",
    "No respite. Siemon is near.",
    "His laugh, your despair. Siemon.",
    "Siemon. His presence chills the air.",
    "Your fear feeds him. Siemon.",
    "He's the terror in the shadows. Siemon.",
    "The maze tightens. Siemon is near.",
    "His laughter, a chilling sound. Siemon.",
    "He's the monster in your nightmares. Siemon.",
    "No escape from Siemon.",
    "His presence, a daunting reality. Siemon.",
    "He's the echo in your nightmares. Siemon.",
    "No sanctuary from Siemon.",
    "His name chills your heart. Siemon.",
    "He's the darkness in the maze. Siemon.",
    "No escape from his grasp. Siemon.",
    "His shadow engulfs the labyrinth. Siemon.",
    "He's the haunting laugh. Siemon.",
    "No respite in the labyrinth. Siemon.",
    "His eyes, a daunting sight. Siemon.",
    "He's the predator in the maze. Siemon.",
    "No safety from Siemon.",
    "His presence, a creeping dread. Siemon.",
    "He's the shadow in your fear. Siemon.",
    "No escape from the labyrinth. Siemon.",
    "His name, a dreadful echo. Siemon.",
    "He's the terror in your path. Siemon.",
    "No safety. Siemon is here.",
    "His shadow, a chilling sight. Siemon.",
    "He's the menace in the maze. Siemon.",
    "No refuge. Siemon lurks.",
    "His laughter, a dreadful sound. Siemon.",
    "He's the ghost in your nightmares. Siemon.",
    "No sanctuary. Siemon lurks.",
    "His presence, a shivering cold. Siemon.",
    "He's the beast in your fear. Siemon.",
    "No safety in the labyrinth. Siemon.",
    "His name, a haunting whisper. Siemon.",
    "He's the terror in the labyrinth. Siemon.",
    "No refuge from his shadow. Siemon.",
    "His laughter, a chilling echo. Siemon.",
    "He's the monster in the shadows. Siemon.",
    "No sanctuary in the shadows. Siemon.",
    "His shadow, a daunting sight. Siemon.",
    "He's the hunter in your nightmares. Siemon.",
    "No respite. Siemon lurks.",
    "His name, a daunting echo. Siemon.",
    "He's the dread in your path. Siemon.",
    "No safety from his grasp. Siemon.",
    "His laughter, a haunting sound. Siemon.",
    "He's the phantom in the maze. Siemon.",
    "No respite from Siemon.",
    "His presence, a dreadful chill. Siemon.",
    "He's the beast in your nightmares. Siemon.",
    "No safety in the shadows. Siemon.",
    "His name, a chilling whisper. Siemon.",
    "He's the terror in the maze. Siemon.",
    "No refuge from his eyes. Siemon.",
    "His laughter, a dreadful echo. Siemon.",
    "He's the monster in your path. Siemon.",
    "No sanctuary from his grasp. Siemon.",
    "His shadow, a shivering cold. Siemon.",
    "He's the hunter in your fear. Siemon.",
    "No safety. Siemon lurks.",
    "His name, a haunting echo. Siemon.",
    "He's the dread in your nightmares. Siemon.",
    "No escape from his eyes. Siemon.",
    "His laughter, a shivering cold. Siemon.",
    "He's the phantom in your nightmares. Siemon.",
    "No escape from Siemon.",
    "His presence, a haunting chill. Siemon.",
    "He's the beast in the labyrinth. Siemon.",
    "No safety in the maze. Siemon.",
    "His name, a dreadful whisper. Siemon.",
    "He's the terror in your nightmares. Siemon.",
    "No refuge from his laughter. Siemon.",
    "His shadow, a haunting chill. Siemon.",
    "He's the hunter in the labyrinth. Siemon.",
    "No sanctuary. Siemon lurks.",
    "His laughter, a daunting echo. Siemon.",
    "He's the phantom in your fear. Siemon.",
    "No respite from his grasp. Siemon.",
    "His presence, a dreadful echo. Siemon.",
    "He's the beast in your path. Siemon.",
    "No sanctuary in the maze. Siemon.",
    "His name, a shivering cold. Siemon.",
    "He's the terror in your fear. Siemon.",
    "No refuge from his shadow. Siemon.",
    "His shadow, a daunting echo. Siemon.",
    "He's the hunter in your path. Siemon.",
    "No safety. Siemon is here.",
    "His laughter, a shivering echo. Siemon.",
    "He's the phantom in the shadows. Siemon.",
    "No respite from his laughter. Siemon.",
    "His presence, a haunting echo. Siemon.",
    "He's the beast in your nightmares. Siemon.",
    "No safety in the labyrinth. Siemon.",
    "His name, a dreadful chill. Siemon.",
    "He's the terror in the shadows. Siemon.",
    "No refuge from his eyes. Siemon.",
    "His shadow, a haunting echo. Siemon.",
    "He's the hunter in the shadows. Siemon.",
    "No sanctuary. Siemon lurks.",
    "His laughter, a daunting chill. Siemon.",
    "He's the phantom in your path. Siemon.",
    "No respite from his shadow. Siemon.",
    "His presence, a dreadful chill. Siemon.",
    "He's the beast in the shadows. Siemon.",
    "No sanctuary in the labyrinth. Siemon."
};

#define AUDIO_MESSAGE_COUNT 10

static const char *audio_messages[AUDIO_MESSAGE_COUNT] = {
    "rom:/simon_kills.wav64",
    "rom:/to_play.wav64",
    "rom:/press_the.wav64",
    "rom:/you_fool.wav64",
    "rom:/breath.wav64",
    "rom:/company.wav64",
    "rom:/console.wav64",
    "rom:/fear.wav64",
    "rom:/feet.wav64",
    "rom:/panic.wav64"
};

static const char *audio_lip_sync[AUDIO_MESSAGE_COUNT] = {
    "rom:/simon_kills.pgo",
    "rom:/to_play.pgo",
    "rom:/press_the.pgo",
    "rom:/you_fool.pgo",
    "rom:/breath.pgo",
    "rom:/company.pgo",
    "rom:/console.pgo",
    "rom:/fear.pgo",
    "rom:/feet.pgo",
    "rom:/panic.pgo"
};

static Word* audio_words[AUDIO_MESSAGE_COUNT];

static wav64_t audio_clips[AUDIO_MESSAGE_COUNT];

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

void switch_to_heaven_colors()
{
    is_in_heaven = true;
    glFogfv(GL_FOG_COLOR, environment_color_heaven);
}

void switch_to_world_colors()
{
    is_in_heaven = false;
    glFogfv(GL_FOG_COLOR, environment_color);
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
    
    switch_to_heaven_colors();

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

int simon_frame = 0;
int simon_frame_time = 0;

int audio_clip_index = 0;

void play_audio_clip(const char* name)
{
    for (int i = 0; i < AUDIO_MESSAGE_COUNT; i++)
    {
        if (!strcmp(name, audio_messages[i]))
        {
            wav64_play(&audio_clips[i], CHANNEL_VOICE);
            simon_frame_time = 0;
            audio_clip_index = i;
        }
    }
}

void draw_quad()
{
    int animation_frame = simon_frame;
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

    int adjustedCameraX = (int)(camera_x / (cube_size * 2));
    int adjustedCameraY = (int)(camera_z / (cube_size * 2));

    int lowerBoundX = max(0, adjustedCameraX - (MAX_LENGTH / cube_size));
    int upperBoundX = min(width - 1, adjustedCameraX + (MAX_LENGTH / cube_size));

    int lowerBoundY = max(0, adjustedCameraY - (MAX_LENGTH / cube_size));
    int upperBoundY = min(height - 1, adjustedCameraY + (MAX_LENGTH / cube_size));
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

    // Normalize the direction vector
    float nx = dx / d;
    float ny = dy / d;
    float nz = dz / d;

    // Initial direction vector (base vector), for example, along z-axis
    float bx = 0.0f;
    float by = 0.0f;
    float bz = 1.0f;

    // Compute the cross product (b x n) and the dot product (b . n)
    float crossX = by * nz - bz * ny;
    float crossY = bz * nx - bx * nz;
    float crossZ = bx * ny - by * nx;
    float dot = bx * nx + by * ny + bz * nz;

    // Normalize the cross product vector
    float crossD = sqrt(crossX * crossX + crossY * crossY + crossZ * crossZ);
    crossX /= crossD;
    crossY /= crossD;
    crossZ /= crossD;

    // Calculate the rotation matrix using Rodrigues' rotation formula
    float R[16];
    float cosTheta = dot;
    float sinTheta = sqrt(1 - cosTheta * cosTheta);
    float oneMinusCosTheta = 1 - cosTheta;

    R[0] = cosTheta + crossX * crossX * oneMinusCosTheta;
    R[1] = crossX * crossY * oneMinusCosTheta - crossZ * sinTheta;
    R[2] = crossX * crossZ * oneMinusCosTheta + crossY * sinTheta;
    R[3] = 0;

    R[4] = crossY * crossX * oneMinusCosTheta + crossZ * sinTheta;
    R[5] = cosTheta + crossY * crossY * oneMinusCosTheta;
    R[6] = crossY * crossZ * oneMinusCosTheta - crossX * sinTheta;
    R[7] = 0;

    R[8] = crossZ * crossX * oneMinusCosTheta - crossY * sinTheta;
    R[9] = crossZ * crossY * oneMinusCosTheta + crossX * sinTheta;
    R[10] = cosTheta + crossZ * crossZ * oneMinusCosTheta;
    R[11] = 0;

    R[12] = 0;
    R[13] = 0;
    R[14] = 0;
    R[15] = 1;

    glBindTexture(GL_TEXTURE_2D, textures[0]);

    glPushMatrix();

    // Translate the cube to center
    glTranslatef(cx, cy, cz);

    // Apply the rotation matrix
    glMultMatrixf(R);

    // Scale the cube
    glScalef(0.5, d, 0.5);

    // Draw the cube
    draw_cube();

    glPopMatrix();
}

static const float distance_cutoff = 25.0f;
#define DEATH_DISTANCE 12.0f

static bool show_message = false;

const char* message_str = "";

void tintScreenRed(float red_value)
{
    glMatrixMode(GL_PROJECTION);
    glLoadIdentity();
    glOrtho(0.0, display_get_width(), display_get_height(), 0.0, -1.0, 1.0);
    glMatrixMode(GL_MODELVIEW);
    glLoadIdentity();

    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

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
}

void tintScreenRGB(float red_value, float green_value, float blue_value)
{
    glMatrixMode(GL_PROJECTION);
    glLoadIdentity();
    glOrtho(0.0, display_get_width(), display_get_height(), 0.0, -1.0, 1.0);
    glMatrixMode(GL_MODELVIEW);
    glLoadIdentity();

    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

    // The color you want to tint with (RGBA)
    float red = red_value;   // range: 0-1
    float green = green_value; // range: 0-1
    float blue = blue_value;  // range: 0-1
    float alpha = 1.0f; // range: 0-1, 0 means fully transparent, 1 means fully opaque

    glColor4f(red, green, blue, alpha);
    glBegin(GL_QUADS);
    glVertex2i(0, 0);
    glVertex2i(display_get_width(), 0);
    glVertex2i(display_get_width(), display_get_height());
    glVertex2i(0, display_get_height());
    glEnd();

    glDisable(GL_BLEND);
}

int getFrameNumberByVoiceLineTime(Word* head, int time)
{
    Word* tempWord = head;
    bool exit = false;
    char mouth_type[] = "    ";
    while(tempWord != NULL && !exit) {
        //debugf("Word: %s\n", tempWord->word);
        Phoneme* tempPhoneme = tempWord->phoneme;
        while(tempPhoneme != NULL) {
            if (tempPhoneme->time >= time)
            {
                for (int i = 0; i < 4; i++)
                {
                    mouth_type[i] = tempPhoneme->mouth_type[i];
                }
                //debugf("\tTime: %d, Mouth Type: %s\n", tempPhoneme->time, tempPhoneme->mouth_type);
                exit = true;
                break;
            }
            //debugf("\tTime: %d, Mouth Type: %s\n", tempPhoneme->time, tempPhoneme->mouth_type);
            tempPhoneme = tempPhoneme->next;
        }
        tempWord = tempWord->next;
    }

    int retVal = 14;

    // Print out the mapping
    for(int i = 0; i < MAP_SIZE; ++i) {
        if (!strcmp(mouth_type, texture_map[i].name))
        {
            retVal = texture_map[i].index;
            //debugf("Name: %s, Index: %d, Time: %i\n", texture_map[i].name, texture_map[i].index, time);
        }
    }

    return retVal;
}

void render_game()
{

    gl_context_begin();

    if (is_in_heaven)
    {
        glClearColor(environment_color_heaven[0], environment_color_heaven[1], environment_color_heaven[2], environment_color_heaven[3]);
    }
    else
    {
        glClearColor(environment_color[0], environment_color[1], environment_color[2], environment_color[3]);
    }
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

    float distance_from_simon = distance3D(simon_x, simon_y, simon_z, camera_x, camera_y, camera_z);

    float red_value = 1.0f;

    if (distance_from_simon < distance_cutoff)
    {
        red_value = distance_from_simon / distance_cutoff;
    }

    if (!is_in_heaven)
    {
        tintScreenRed(red_value);

        if (distance_from_simon < DEATH_DISTANCE)
        {
            float tint_r = 0.5 + 0.5 * cos(game_time / 2.0f + 0);
            float tint_g = 0.5 + 0.5 * cos(game_time / 2.0f + 2);
            float tint_b = 0.5 + 0.5 * cos(game_time / 2.0f + 4);

            tintScreenRGB(tint_r, tint_g, tint_b);
            
        }
    }

    

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
    rdpq_debug_log_msg("Simon");

    simon_frame = getFrameNumberByVoiceLineTime(audio_words[audio_clip_index], simon_frame_time++ / 2);

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

    //drawCubeBetweenPoints(simon_x, simon_y, simon_z, camera_x, camera_y - 1.5, camera_z);

    gl_context_end();

    if (show_message)
    {
        rdpq_font_begin(RGBA32(0xFF, 0xFF, 0xFF, 0xFF));
        rdpq_font_position(30, 25);
        rdpq_font_printf(fnt1, "%s", message_str);
        rdpq_font_end();
    }
}

float clamp(float val, float min, float max) {
    return val < min ? min : val > max ? max : val;
}

static bool hasDone = false;

static float prev_camera_x;
static float prev_camera_z;

static const char* game_state;

static int transition_count = 0;

void play_step()
{
    static const int mid = 12127;
    static const int dif = 500;
    static const int low = mid - dif;
    static const int high = mid + dif;

    int pitch = low + (rand() % (high - low));

    mixer_ch_set_freq(CHANNEL_SFX1, pitch);
    wav64_play(&step_sfx, CHANNEL_SFX1);
}

bool is_in_opening = true;

bool waiting_for_a = false;

bool a_pressed = false;

void setup_game()
{
    game_time = 3283;
    //camera_x = cube_size * 2, camera_y = floor_y, camera_z = cube_size * 2;
    camera_x = cube_size * 2, camera_y = floor_y + 50, camera_z = cube_size * 2;

    camera_pitch = -0.329688f, camera_yaw = 0.207813f;

    simon_x = 0.0f, simon_y = 1.5f, simon_z = 60.0f;

    waiting_for_a = false;

    is_in_opening = true;
    a_pressed = false;

    srand(global_time); //seed random number generator with system time
	initialize_maze_gen();      //initialize the maze
	generate_maze();        //generate the maze
}

int last_step_time = -60;

void move_simon_towards_player()
{
    simon_x += (camera_x - simon_x) * 0.005f * perlin2d(simon_x, simon_y, 0.1, 4) * 2.0f;
    simon_y += (camera_y - simon_y) * 0.005f * perlin2d(simon_y, simon_x, 0.1, 4) * 1.5f;
    simon_z += (camera_z - simon_z) * 0.005f * perlin2d(simon_x, simon_y, 0.2, 4) * 1.8f;

    //debugf("%f\n", perlin2d(simon_x, simon_y, 0.2, 4));

    rotation = game_time * 0.5f;

    simon_x += sin(rotation*0.01f) * 0.05f;
    simon_y += fabs(sin(rotation*0.01f) * 0.05f);
    simon_z += cos(rotation*0.01f) * 0.05f;
}

void do_player_looking_logic()
{
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
}

void do_player_movement_logic()
{
    static int jumpFrameCount = 0;

    static bool doingJump = false;
    bool is_walking = false;

    if (down.c[0].A && !doingJump) {
        jumpFrameCount = 0;
        doingJump = true;
    }

    float movement_speed = 0.3f;

    if (doingJump)
    {
        float jump_val = jump_height(jumpFrameCount++ / 60.0f);
        
        movement_speed = 0.4f;

        if (jump_val < 0.0f)
        {
            doingJump = false;
        }
        else
        {
            camera_y = floor_y + jump_val;
        }
    }

    if (pressed.c[0].C_up)
    {
        is_walking = true;
        prev_camera_x = camera_x;
        camera_x += sin(camera_yaw) * movement_speed;
        Collision_Info collision = check_for_maze_collision();

        if (collision.hadCollision)
        {
            camera_x = prev_camera_x;
        }

        prev_camera_z = camera_z;
        camera_z += cos(camera_yaw) * movement_speed;
        collision = check_for_maze_collision();

        if (collision.hadCollision)
        {
            camera_z = prev_camera_z;
        }
    }

    if (pressed.c[0].C_left)
    {
        is_walking = true;
        prev_camera_x = camera_x;
        camera_x += cos(camera_yaw) * movement_speed;
        Collision_Info collision = check_for_maze_collision();

        if (collision.hadCollision)
        {
            camera_x = prev_camera_x;
        }

        prev_camera_z = camera_z;
        camera_z -= sin(camera_yaw) * movement_speed;
        collision = check_for_maze_collision();

        if (collision.hadCollision)
        {
            camera_z = prev_camera_z;
        }
    }

    if (pressed.c[0].C_right)
    {
        is_walking = true;
        prev_camera_x = camera_x;
        camera_x -= cos(camera_yaw) * movement_speed;
        Collision_Info collision = check_for_maze_collision();

        if (collision.hadCollision)
        {
            camera_x = prev_camera_x;
        }

        prev_camera_z = camera_z;
        camera_z += sin(camera_yaw) * movement_speed;
        collision = check_for_maze_collision();

        if (collision.hadCollision)
        {
            camera_z = prev_camera_z;
        }
    }

    if (pressed.c[0].C_down)
    {
        is_walking = true;
        prev_camera_x = camera_x;
        camera_x -= sin(camera_yaw) * movement_speed;
        Collision_Info collision = check_for_maze_collision();

        if (collision.hadCollision)
        {
            camera_x = prev_camera_x;
        }

        prev_camera_z = camera_z;
        camera_z -= cos(camera_yaw) * movement_speed;
        collision = check_for_maze_collision();

        if (collision.hadCollision)
        {
            camera_z = prev_camera_z;
        }
    }

    if (is_walking && game_time - last_step_time >= 20 && !doingJump) {
        play_step();
        last_step_time = game_time;
    }
}

void play_simon_game_messages(float distance_from_simon)
{
    static uint32_t random_message_time = 500;
    static uint32_t last_time = 0;

    if (game_time % random_message_time == 0 && game_time - last_time > 1000)
    {
        last_time = game_time;
        random_message_time = 500 + (rand() % 1000);
        const int start_at = 5;
        play_audio_clip(audio_messages[start_at + (rand() % (AUDIO_MESSAGE_COUNT - start_at))]);
    }

    if (distance_from_simon < distance_cutoff)
    {
        if (!hasDone && game_time - last_time > 1000)
        {
            hasDone = true;
            if (rand() % 100 < 30)
            {
                //wav64_play(&kill_sample, CHANNEL_VOICE);
                last_time = game_time;
                play_audio_clip("rom:/simon_kills.wav64");
            }
            message_str = messages[rand() % MESSAGE_COUNT];
        }
        show_message = true;
    }
    else
    {
        hasDone = false;
        show_message = false;
    }
}

void check_and_process_player_death(float distance_from_simon)
{
    bool inDeathRange = distance_from_simon < DEATH_DISTANCE;

    static bool oldDeathRange = false;

    static int deathFrameCount = 0;

    if (inDeathRange != oldDeathRange)
    {
        oldDeathRange = inDeathRange;
        deathFrameCount = 0;
    }
    if (inDeathRange)
    {
        deathFrameCount++;
    }
    if (deathFrameCount > 30)
    {
        // Death
        game_state = "transition";
        transition_count = 0;
        //setup_game();
    }
}

void simon_look_at_player()
{
    float dx = camera_x - simon_x;
    float dz = camera_z - simon_z;

    float target_rotation = atan2(dz, dx) * -(180.0 / PI) + 90.0f;

    // calculate the difference and make sure it stays in the -180 to 180 range
    float rotation_difference = fmodf((target_rotation - current_rotation + 180.0f), 360.0f) - 180.0f;
    current_rotation += rotation_difference * 0.05f * perlin2d(simon_x, simon_y, 0.1, 4);
}

void step_through_game()
{

    game_time++;

    if (is_in_opening)
    {
        static uint32_t time_since_a = 0;
        if (game_time < 4000)
        {
            move_simon_towards_player();
            
        }
        if (game_time == 4176)
        {
            play_audio_clip("rom:/to_play.wav64");
        }
        if (game_time == 4528)
        {
            waiting_for_a = true;
        }

        if (waiting_for_a)
        {
            if (down.c[0].A)
            {
                play_audio_clip("rom:/press_the.wav64");
                a_pressed = true;
                waiting_for_a = false;
                time_since_a = 0;
            }
        }

        if (a_pressed)
        {
            time_since_a++;
            debugf("%li\n", time_since_a);
        }

        if (time_since_a == 451)
        {
            play_audio_clip("rom:/you_fool.wav64");
            switch_to_world_colors();
        }

        if (time_since_a > 887)
        {
            if (camera_y > floor_y)
            {
                camera_y -= perlin2d(camera_x, camera_y, 4, 4);
            }
        }

        if (camera_y < floor_y + 0.1f)
        {
            is_in_opening = false;
        }

        do_player_looking_logic();
    }
    else
    {
        do_player_looking_logic();
        do_player_movement_logic();

        move_simon_towards_player();

        float distance_from_simon = distance3D(simon_x, simon_y, simon_z, camera_x, camera_y, camera_z);

        play_simon_game_messages(distance_from_simon);

        check_and_process_player_death(distance_from_simon);
    }

    simon_look_at_player();
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

    wav64_open(&step_sfx, "rom:/step.wav64");

    rdpq_init();
    gl_init();

// #if DEBUG_RDP
//     rdpq_debug_start();
//     rdpq_debug_log(true);
// #endif

    global_time = 0;

    setup_renderer();

    controller_init();

    glEnable(GL_FOG);

    wav64_play(&sfx_monosample, CHANNEL_MUSIC);

    static bool is_paused = false;

    //setup_game();

    //debugf("Demo by jakes1403. Modified from the libdragon demo.\n");

    fnt1 = rdpq_font_load("rom:/Judges.font64");

    sprite_t* tiles_sprite = sprite_load("rom:/tiles.sprite");

    surface_t tiles_surf = sprite_get_pixels(tiles_sprite);

    for (int i = 0; i < AUDIO_MESSAGE_COUNT; i++)
    {
        wav64_open(&audio_clips[i], audio_messages[i]);

        audio_words[i] = parsePapagayoFile(audio_lip_sync[i]);
    }
    
    //printParsedData(head);

    game_state = "warning";

    // Create a block for the background, so that we can replay it later.
    //rspq_block_begin();

    
    //rspq_block_t* tiles_block = rspq_block_end();

    build_texture_map();

#if !DEBUG_RDP
    while (1)
#endif
    {
        surface_t *disp = display_get();

        rdpq_attach_clear(disp, &zbuffer);
        global_time++;
        controller_scan();
        pressed = get_keys_pressed();
        down = get_keys_down();

        if (down.c[0].left)
        {
            rdpq_debug_start();
            rdpq_debug_log(true);
        }

        if (!strcmp(game_state, "warning"))
        {

            rdpq_set_mode_fill(RGBA32(0x30,0x63,0x8E,0xFF));
            rdpq_fill_rectangle(0, 0, display_get_width(), display_get_height());

            static bool show = true;
            
            if (global_time % 30 == 0)
            {
                show = !show;
            }

            if (show)
            {
                rdpq_font_begin(RGBA32(0xFF, 0x00, 0x00, 0xFF));
                rdpq_font_position(60, 50);
                rdpq_font_print(fnt1, "PHOTOSENSITIVITY WARNING");
                rdpq_font_end();
            }

            rdpq_font_begin(RGBA32(0xFF, 0xFF, 0xFF, 0xFF));
            rdpq_font_position(20, 70);
            rdpq_font_print(fnt1, "A small percentage of people may");
            rdpq_font_end();

            rdpq_font_begin(RGBA32(0xFF, 0xFF, 0xFF, 0xFF));
            rdpq_font_position(20, 80);
            rdpq_font_print(fnt1, "experience seizures when exposed");
            rdpq_font_end();

            rdpq_font_begin(RGBA32(0xFF, 0xFF, 0xFF, 0xFF));
            rdpq_font_position(20, 90);
            rdpq_font_print(fnt1, "to certain lights, patterns, or images,");
            rdpq_font_end();

            rdpq_font_begin(RGBA32(0xFF, 0xFF, 0xFF, 0xFF));
            rdpq_font_position(20, 100);
            rdpq_font_print(fnt1, "even with no history of epilepsy or seizures.");
            rdpq_font_end();

            rdpq_font_begin(RGBA32(0xFF, 0xFF, 0xFF, 0xFF));
            rdpq_font_position(90, 180);
            rdpq_font_print(fnt1, "Press A to continue.");
            rdpq_font_end();

            if (down.c[0].A) {
                game_state = "menu";
            }
        }

        if (!strcmp(game_state, "transition"))
        {
            transition_count++;
            float tint_r = 0.5 + 0.5 * cos(global_time / 2.0f + 0);
            float tint_g = 0.5 + 0.5 * cos(global_time / 2.0f + 2);
            float tint_b = 0.5 + 0.5 * cos(global_time / 2.0f + 4);

            rdpq_set_mode_fill(RGBA32(tint_r * 255,tint_g * 255,tint_b * 255,0xFF));
            rdpq_fill_rectangle(0, 0, display_get_width(), display_get_height());

            rdpq_font_begin(RGBA32(0xFF, 0x00, 0x00, 0xFF));
            rdpq_font_position(40, 100);
            rdpq_font_print(fnt1, "It is advised to turn off your system.");
            rdpq_font_end();

            if (transition_count > 100)
            {
                game_state = "play";
                setup_game();
            }
        }

        if (!strcmp(game_state, "play"))
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
                rdpq_font_begin(RGBA32(0xFF, 0xFF, 0xFF, 0xFF));
                rdpq_font_position(20, 20);
                rdpq_font_print(fnt1, "Pause");
                rdpq_font_end();
            }
        }
        
        if (!strcmp(game_state, "menu"))
        {
            rdpq_set_mode_copy(false);
            // rdpq_set_mode_standard();
            //rspq_block_run(tiles_block);

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

            rdpq_sprite_blit(sprites[30], 150, 200, &(rdpq_blitparms_t){
                .scale_x = 1.0, .scale_y = 1.0, .theta = 0.2 * sin((global_time / 60.0f))
            });

            rdpq_font_begin(RGBA32(0xFF, 0xFF, 0xFF, 0xFF));
            rdpq_font_position(130, 200);
            rdpq_font_print(fnt1, "Press Start");
            rdpq_font_end();

            if (down.c[0].start) {
                game_state = "transition";
                transition_count = 0;
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
            

        if (down.c[0].left)
        {
            rspq_wait();
            break;
        }

    }

}
