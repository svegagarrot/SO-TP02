#ifndef GAME_FUNCTIONS_H
#define GAME_FUNCTIONS_H

#include <stdint.h>

#define COLOR_BG_HOME 0x3D8B7D
#define COLOR_TEXT_HOME 0xF9DFE0
#define COLOR_BG_GREEN 0xB5CE64
#define COLOR_PLAYER1 0xFF4D85
#define COLOR_PLAYER2 0x006280
#define COLOR_BALL2 0x00BBE6
#define COLOR_WHITE 0xFFFFFF
#define COLOR_BLACK 0x000000 
#define COLOR_BROWN 0x8B4513

extern int SCREEN_WIDTH;
extern int SCREEN_HEIGHT;
#define UI_TOP_MARGIN (SCREEN_HEIGHT / 24)
#define PLAYER_RADIUS (SCREEN_WIDTH / 100)

#define FRICTION 95         
#define MIN_VELOCITY 1      
#define BOUNCE_FACTOR 90    
#define POWER_FACTOR (SCREEN_WIDTH / 50)

#define MAX_OBSTACLES 20

extern const int cos_table[36];
extern const int sin_table[36];

extern int current_level;

typedef struct {
    int x, y, prev_x, prev_y;
    int angle, prev_angle;
    int golpes;
    int puede_golpear;
    int color;
    int arrow_color;
    int ball_x, ball_y, prev_ball_x, prev_ball_y;
    int ball_vx, ball_vy;
    int ball_in_hole;
    int ball_color;
    int debe_verificar_derrota;
    char control_up, control_left, control_right;
    char *name;
} Player;

typedef struct {
    int x, y;
    int size; 
} Obstacle;

void drawCircle(int cx, int cy, int radius, uint32_t color);
void drawText(int x, int y, const char *text, uint32_t color);
void drawTextWithBg(int x, int y, const char *text, uint32_t textColor, uint32_t bgColor);
void eraseBallSmart(int prev_ball_x, int prev_ball_y, Player *players, int num_players, int hole_x, int hole_y, int hole_radius, Obstacle *obstacles, int num_obstacles);
void erasePlayerSmart(int prev_x, int prev_y, Player *players, int num_players, int hole_x, int hole_y, int hole_radius, Obstacle *obstacles, int num_obstacles);
int isInsideHole(int x, int y, int h_x, int h_y, int hole_radius);
unsigned int rand();
int my_rand(void);
int rand_range(int min, int max);
int circles_overlap(int x1, int y1, int r1, int x2, int y2, int r2);
void drawFullWidthBar(int y, int height, uint32_t color);
void displayFullScreenMessage(const char *message, uint32_t textColor);
void drawPlayerArrow(int player_x, int player_y, int player_angle, int hole_x, int hole_y, int hole_radius, int arrow_color, Player *players, int num_players, Obstacle *obstacles, int num_obstacles);
void eraseArrow(int prev_x, int prev_y, int prev_angle, int hole_x, int hole_y, int hole_radius, Player *players, int num_players, Obstacle *obstacles, int num_obstacles);
void drawTextFixed(int x, int y, const char *text, uint32_t color, uint32_t bg);
void audiobounce();
void play_mission_impossible();
void init_screen_dimensions();

// Funciones de nivel
void increment_level();
void reset_level();
int get_current_level();

// Funciones para progresión de dificultad
int get_ball_power_factor(int level);
int get_hole_radius(int level);

// Genera obstáculos aleatorios
void generate_obstacles(Obstacle *obstacles, int *num_obstacles, int level, int hole_x, int hole_y);
// Dibuja los obstáculos
void draw_obstacles(Obstacle *obstacles, int num_obstacles);
// Chequea colisión de punto con obstáculos
int point_in_obstacle(int x, int y, Obstacle *obstacles, int num_obstacles);
// Chequea colisión de círculo con obstáculos
int circle_obstacle_collision(int cx, int cy, int radius, Obstacle *obstacles, int num_obstacles, int *collided_index);
// Rebote de pelota contra obstáculo
void bounce_ball_on_obstacle(int *vx, int *vy, int *ball_x, int *ball_y, int radius, Obstacle *obstacles, int num_obstacles);

#endif
