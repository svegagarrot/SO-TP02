#include "include/lib.h"
#include "include/game_functions.h"

#define abs(x) ((x) < 0 ? -(x) : (x))

int SCREEN_WIDTH;
int SCREEN_HEIGHT;

void init_screen_dimensions() {
    uint64_t width, height;
    if (getScreenDims(&width, &height)) {
        SCREEN_WIDTH = width;
        SCREEN_HEIGHT = height;
    } else {
        // Valores por defecto en caso de error
        SCREEN_WIDTH = 1024;
        SCREEN_HEIGHT = 768;
    }
}

const int cos_table[36] = {100,98,94,87,77,64,50,34,17,0,-17,-34,-50,-64,-77,-87,-94,-98,-100,-98,-94,-87,-77,-64,-50,-34,-17,0,17,34,50,64,77,87,94,98};
const int sin_table[36] = {0,17,34,50,64,77,87,94,98,100,98,94,87,77,64,50,34,17,0,-17,-34,-50,-64,-77,-87,-94,-98,-100,-98,-94,-87,-77,-64,-50,-34,-17};

int current_level = 1;

typedef struct {
    int freq;    
    int dur_ms;
} MelodyNote;

enum {
    G   = 392,
    A1  = 466,
    C   = 262,
    F   = 349,
    F1  = 370,
    Gb = 93
};

// mision imposible
static const MelodyNote mission_first[] = {
    { G, 500 }, { G, 400 }, { G, 250 }, { G, 250 },
    { A1,250 }, { C, 250 }, { G, 250 }, { G, 250 },
    { G, 250 }, { G, 250 }, { F, 250 }, { F1,250 },
    { G, 500 } 
};

static const int mission_first_len = sizeof(mission_first)/sizeof(MelodyNote);

// Dibuja un círculo
void drawCircle(int cx, int cy, int radius, uint32_t color) {
    for (int y = -radius; y <= radius; y++) {
        for (int x = -radius; x <= radius; x++) {
            if (x*x + y*y <= radius*radius) {
                int px = cx + x;
                int py = cy + y;
                if (px >= 0 && px < SCREEN_WIDTH && py >= 0 && py < SCREEN_HEIGHT) {
                    video_putPixel(px, py, color);
                }
            }
        }
    }
}

// Dibuja texto en pantalla
void drawText(int x, int y, const char *text, uint32_t color) {
    for (int i = 0; text[i] != '\0'; i++) {
        video_putCharXY(x + i * 8, y, text[i], color, COLOR_BG_HOME);
    }
}

// Dibuja texto con fondo
void drawTextWithBg(int x, int y, const char *text, uint32_t textColor, uint32_t bgColor) {
    int lines = y / 16;
    int spaces = x / 8;
    for (int i = 0; i < lines; i++) video_putChar('\n', textColor, bgColor);
    for (int i = 0; i < spaces; i++) video_putChar(' ', textColor, bgColor);
    while (*text) {
        video_putChar(*text, textColor, bgColor);
        text++;
    }
}

// Borra la pelota anterior
void eraseBallSmart(int prev_ball_x, int prev_ball_y, Player *players, int num_players, int hole_x, int hole_y, int hole_radius, Obstacle *obstacles, int num_obstacles) {
    for (int y = -5; y <= 5; y++) {
        for (int x = -5; x <= 5; x++) {
            if (x*x + y*y <= 5*5) {
                int px = prev_ball_x + x;
                int py = prev_ball_y + y;
                if (px >= 0 && px < SCREEN_WIDTH && py >= UI_TOP_MARGIN && py < SCREEN_HEIGHT) {
                    int painted = 0;
                    
                    if (point_in_obstacle(px, py, obstacles, num_obstacles)) {
                        video_putPixel(px, py, COLOR_BROWN);
                        painted = 1;
                    }
                    
                    if (!painted) {
                        for (int j = 0; j < num_players; j++) {
                            if ((px - players[j].x)*(px - players[j].x) + (py - players[j].y)*(py - players[j].y) <= PLAYER_RADIUS*PLAYER_RADIUS) {
                                video_putPixel(px, py, players[j].color);
                                painted = 1;
                                break;
                            }
                        }
                    }
                    
                    if (!painted) {
                        for (int j = 0; j < num_players; j++) {
                            if (prev_ball_x != players[j].ball_x || prev_ball_y != players[j].ball_y) {
                                if ((px - players[j].ball_x)*(px - players[j].ball_x) + (py - players[j].ball_y)*(py - players[j].ball_y) <= 5*5) {
                                    video_putPixel(px, py, players[j].ball_color);
                                    painted = 1;
                                    break;
                                }
                            }
                        }
                    }
                    
                    if (!painted && ((px - hole_x)*(px - hole_x) + (py - hole_y)*(py - hole_y) <= 15*15)) {
                        video_putPixel(px, py, COLOR_BLACK);
                        painted = 1;
                    }
                    if (!painted) {
                        video_putPixel(px, py, COLOR_BG_GREEN);
                    }
                }
            }
        }
    }
}

// Borra el jugador anterior
void erasePlayerSmart(int prev_x, int prev_y, Player *players, int num_players, int hole_x, int hole_y, int hole_radius, Obstacle *obstacles, int num_obstacles) {
    for (int y = -PLAYER_RADIUS; y <= PLAYER_RADIUS; y++) {
        for (int x = -PLAYER_RADIUS; x <= PLAYER_RADIUS; x++) {
            if (x*x + y*y <= PLAYER_RADIUS*PLAYER_RADIUS) {
                int px = prev_x + x;
                int py = prev_y + y;
                if (px >= 0 && px < SCREEN_WIDTH && py >= UI_TOP_MARGIN && py < SCREEN_HEIGHT) {
                    int painted = 0;
                    
                    if (point_in_obstacle(px, py, obstacles, num_obstacles)) {
                        video_putPixel(px, py, COLOR_BROWN);
                        painted = 1;
                    }
                    
                    if (!painted) {
                        for (int j = 0; j < num_players; j++) {
                            if (prev_x != players[j].x || prev_y != players[j].y) {
                                if ((px - players[j].x)*(px - players[j].x) + (py - players[j].y)*(py - players[j].y) <= PLAYER_RADIUS*PLAYER_RADIUS) {
                                    video_putPixel(px, py, players[j].color);
                                    painted = 1;
                                    break;
                                }
                            }
                        }
                    }
                    if (!painted) {
                        for (int j = 0; j < num_players; j++) {
                            if ((px - players[j].ball_x)*(px - players[j].ball_x) + (py - players[j].ball_y)*(py - players[j].ball_y) <= 5*5) {
                                video_putPixel(px, py, players[j].ball_color);
                                painted = 1;
                                break;
                            }
                        }
                    }
                    if (!painted && ((px - hole_x)*(px - hole_x) + (py - hole_y)*(py - hole_y) <= hole_radius*hole_radius)) {
                        video_putPixel(px, py, COLOR_BLACK);
                        painted = 1;
                    }
                    if (!painted) {
                        video_putPixel(px, py, COLOR_BG_GREEN);
                    }
                }
            }
        }
    }
}

// Devuelve 1 si el punto está dentro del hoyo
int isInsideHole(int x, int y, int h_x, int h_y, int hole_radius) {
    int dx = x - h_x;
    int dy = y - h_y;
    return (dx*dx + dy*dy <= hole_radius*hole_radius);
}

// Reemplazo la implementación de rand por my_rand
unsigned int next_rand = 12345;
int my_rand(void) {
    next_rand = next_rand * 1103515245 + 12345;
    return (unsigned int)(next_rand / 65536) % 32768;
}

int rand_range(int min, int max) {
    return min + (my_rand() % (max - min + 1));
}

// Devuelve 1 si dos círculos se superponen
int circles_overlap(int x1, int y1, int r1, int x2, int y2, int r2) {
    int dx = x1 - x2;
    int dy = y1 - y2;
    return (dx*dx + dy*dy < (r1 + r2) * (r1 + r2));
}

// Dibuja una barra horizontal de ancho completo
void drawFullWidthBar(int y, int height, uint32_t color) {
    for (int j = y; j < y + height; j++) {
        for (int i = 0; i < SCREEN_WIDTH; i++) {
            video_putPixel(i, j, color);
        }
    }
}

//Muestra un mensaje en pantalla completa
void displayFullScreenMessage(const char *message, uint32_t textColor) {
    int original_scale = current_font_scale;
    setFontScale(1);

    for (int yy = 0; yy < SCREEN_HEIGHT; yy++) {
        for (int xx = 0; xx < SCREEN_WIDTH; xx++) {
            video_putPixel(xx, yy, COLOR_BLACK);
        }
    }

    for (int xx = 0; xx < SCREEN_WIDTH; xx++) {
        video_putPixel(xx, 10, textColor);
        video_putPixel(xx, SCREEN_HEIGHT - 10, textColor);
    }
    for (int yy = 10; yy < SCREEN_HEIGHT - 10; yy++) {
        video_putPixel(10, yy, textColor);
        video_putPixel(SCREEN_WIDTH - 10, yy, textColor);
    }

    int messageLen = 0;
    for (const char *t = message; *t; t++) {
        messageLen++;
    }

    int textWidth  = messageLen * 8;
    int textHeight = 16;

    int x = (SCREEN_WIDTH  - textWidth ) / 2;
    int y = (SCREEN_HEIGHT - textHeight) / 2;

    for (int yy = y; yy < y + textHeight; yy++) {
        for (int xx = x; xx < x + textWidth; xx++) {
            video_putPixel(xx, yy, COLOR_BLACK);
        }
    }

    drawTextFixed(x, y, message, textColor, COLOR_BLACK);

    int underlineY = y + textHeight - 1;
    for (int xi = x; xi < x + textWidth; xi++) {
        video_putPixel(xi, underlineY, textColor);
    }

    setFontScale(original_scale);
}

// Dibuja la flecha del jugador
void drawPlayerArrow(int player_x, int player_y, int player_angle, int hole_x, int hole_y, int hole_radius, int arrow_color, Player *players, int num_players, Obstacle *obstacles, int num_obstacles) {
    int arrow_len = 18;
    int arrow_x = player_x + (cos_table[player_angle] * arrow_len) / 100;
    int arrow_y = player_y + (sin_table[player_angle] * arrow_len) / 100;
    if (arrow_y < UI_TOP_MARGIN + 5) return;
    if (point_in_obstacle(arrow_x, arrow_y, obstacles, num_obstacles)) return;
    #define IN_SCREEN(x, y) ((x) >= 0 && (x) < SCREEN_WIDTH && (y) >= UI_TOP_MARGIN && (y) < SCREEN_HEIGHT)
    int arrow_points[5][2] = {
        {arrow_x, arrow_y},
        {arrow_x + 1, arrow_y},
        {arrow_x, arrow_y + 1},
        {arrow_x - 1, arrow_y},
        {arrow_x, arrow_y - 1}
    };
    for (int i = 0; i < 5; i++) {
        int px = arrow_points[i][0];
        int py = arrow_points[i][1];
        if (IN_SCREEN(px, py) && !point_in_obstacle(px, py, obstacles, num_obstacles)) {
            video_putPixel(px, py, arrow_color);
        }
    }
    #undef IN_SCREEN
}

// Borra la flecha anterior
void eraseArrow(int prev_x, int prev_y, int prev_angle, int hole_x, int hole_y, int hole_radius, Player *players, int num_players, Obstacle *obstacles, int num_obstacles) {
    int prev_arrow_len = 18;
    int prev_arrow_x = prev_x + (cos_table[prev_angle] * prev_arrow_len) / 100;
    int prev_arrow_y = prev_y + (sin_table[prev_angle] * prev_arrow_len) / 100;
    if (prev_arrow_y >= UI_TOP_MARGIN + 5 && !point_in_obstacle(prev_arrow_x, prev_arrow_y, obstacles, num_obstacles)) {
        #define IN_SCREEN(x, y) ((x) >= 0 && (x) < SCREEN_WIDTH && (y) >= UI_TOP_MARGIN && (y) < SCREEN_HEIGHT)
        int arrow_points[5][2] = {
            {prev_arrow_x, prev_arrow_y},
            {prev_arrow_x + 1, prev_arrow_y},
            {prev_arrow_x, prev_arrow_y + 1},
            {prev_arrow_x - 1, prev_arrow_y},
            {prev_arrow_x, prev_arrow_y - 1}
        };
        for (int i = 0; i < 5; i++) {
            int px = arrow_points[i][0];
            int py = arrow_points[i][1];
            if (IN_SCREEN(px, py) && !point_in_obstacle(px, py, obstacles, num_obstacles)) {
                if (isInsideHole(px, py, hole_x, hole_y, hole_radius)) {
                    video_putPixel(px, py, COLOR_BLACK);
                } else {
                    int painted = 0;
                    
        
                    for (int j = 0; j < num_players; j++) {
                        if (circles_overlap(px, py, 1, players[j].x, players[j].y, PLAYER_RADIUS)) {
                            video_putPixel(px, py, players[j].color);
                            painted = 1;
                            break;
                        }
                    }
                    
                    if (!painted) {
                        // Verificar si es un obstáculo
                        if (point_in_obstacle(px, py, obstacles, num_obstacles)) {
                            video_putPixel(px, py, COLOR_BROWN);
                            painted = 1;
                        }
                    }
                    
                    if (!painted) {
                        for (int j = 0; j < num_players; j++) {
                            int dx = px - players[j].ball_x;
                            int dy = py - players[j].ball_y;
                            if (dx*dx + dy*dy <= 5*5) {
                                video_putPixel(px, py, players[j].ball_color);
                                painted = 1;
                                break;
                            }
                        }
                    }
                    
                    if (!painted) {
                        video_putPixel(px, py, COLOR_BG_GREEN);
                    }
                }
            }
        }
        #undef IN_SCREEN
    }
}

// Dibuja texto fijo en pantalla
void drawTextFixed(int x, int y, const char *text, uint32_t color, uint32_t bg) {
    int i = 0;
    while (text[i]) {
        video_putCharXY(x + i * 8, y, text[i], color, bg);
        i++;
    }
}

void audiobounce() {
    beep(500, 100);
}

static void play_melody(const MelodyNote *mel, int len) {
    for (int i = 0; i < len; i++) {
        if (mel[i].freq > 20) {
            beep(mel[i].freq, mel[i].dur_ms);
        } else {
            sleep(mel[i].dur_ms);
        }
        sleep(20);
    }
}

void play_mission_impossible(void) {
    play_melody(mission_first, mission_first_len);
}

// Funciones para manejar el nivel
void increment_level() {
    current_level++;
    if (current_level > MAX_OBSTACLES) {
        current_level = MAX_OBSTACLES;
    }
}

void reset_level() {
    current_level = 1;
}

int get_current_level() {
    return current_level;
}

// Calcula el radio del hoyo basado en el nivel
int get_hole_radius(int level) {
    if (level <= 1) return 15;  
    if (level >= 20) return 8;  

    int reduction = ((level - 1) * 7) / 19;
    return 15 - reduction;
}

// Calcula la potencia de la pelota basado en el nivel
int get_ball_power_factor(int level) {
    int base_power = POWER_FACTOR;
    if (level <= 1) return base_power;
    if (level >= 20) return (base_power * 60) / 100; 

    int reduction_percent = ((level - 1) * 40) / 19;
    return (base_power * (100 - reduction_percent)) / 100;
}

void generate_obstacles(Obstacle *obstacles, int *num_obstacles, int level, int hole_x, int hole_y) {
    int count = level - 1; 
    if (count < 0) count = 0; 
    if (count > 20) count = 20; 

    int min_size = 15, max_size = 35;
    int tries = 0;
    int i = 0;
    
    while (i < count && tries < 1000) {
        int size = rand_range(min_size, max_size);
        int x = rand_range(0, SCREEN_WIDTH - size - 1);
        int y = rand_range(UI_TOP_MARGIN, SCREEN_HEIGHT - size - 1);
        
        int hx = hole_x, hy = hole_y;
        if (hx >= x-30 && hx <= x+size+30 && hy >= y-30 && hy <= y+size+30) { 
            tries++; 
            continue; 
        }
        
        int overlap = 0;
        for (int j = 0; j < i; j++) {
            if (!(x+size+15 < obstacles[j].x || x > obstacles[j].x+obstacles[j].size+15 ||
                  y+size+15 < obstacles[j].y || y > obstacles[j].y+obstacles[j].size+15)) {
                overlap = 1; 
                break;
            }
        }
        if (overlap) { 
            tries++; 
            continue; 
        }
        
        obstacles[i].x = x;
        obstacles[i].y = y;
        obstacles[i].size = size;
        i++;
    }
    *num_obstacles = i;
}

void draw_obstacles(Obstacle *obstacles, int num_obstacles) {
    for (int i = 0; i < num_obstacles; i++) {
        int x0 = obstacles[i].x, y0 = obstacles[i].y, s = obstacles[i].size;
        for (int y = y0; y < y0 + s; y++) {
            for (int x = x0; x < x0 + s; x++) {
                video_putPixel(x, y, COLOR_BROWN);
            }
        }
    }
}

int point_in_obstacle(int x, int y, Obstacle *obstacles, int num_obstacles) {
    for (int i = 0; i < num_obstacles; i++) {
        if (x >= obstacles[i].x && x < obstacles[i].x + obstacles[i].size &&
            y >= obstacles[i].y && y < obstacles[i].y + obstacles[i].size) {
            return 1;
        }
    }
    return 0;
}

int circle_obstacle_collision(int cx, int cy, int radius, Obstacle *obstacles, int num_obstacles, int *collided_index) {
    for (int i = 0; i < num_obstacles; i++) {
        int left = obstacles[i].x, right = obstacles[i].x + obstacles[i].size;
        int top = obstacles[i].y, bottom = obstacles[i].y + obstacles[i].size;
        int closest_x = (cx < left) ? left : (cx > right ? right : cx);
        int closest_y = (cy < top) ? top : (cy > bottom ? bottom : cy);
        int dx = cx - closest_x, dy = cy - closest_y;
        if (dx*dx + dy*dy <= radius*radius) {
            if (collided_index) *collided_index = i;
            return 1;
        }
    }
    return 0;
}

void bounce_ball_on_obstacle(int *vx, int *vy, int *ball_x, int *ball_y, int radius, Obstacle *obstacles, int num_obstacles) {
    int idx;
    if (!circle_obstacle_collision(*ball_x, *ball_y, radius, obstacles, num_obstacles, &idx)) return;
    Obstacle *o = &obstacles[idx];
    int left = o->x, right = o->x + o->size;
    int top = o->y, bottom = o->y + o->size;

    int center_x = *ball_x;
    int center_y = *ball_y;
    
    int dist_to_left = center_x - left;
    int dist_to_right = right - center_x;
    int dist_to_top = center_y - top;
    int dist_to_bottom = bottom - center_y;
    
    int min_dist = dist_to_left;
    int side = 0; 

    if (dist_to_right < min_dist) { min_dist = dist_to_right; side = 1; }
    if (dist_to_top < min_dist) { min_dist = dist_to_top; side = 2; }
    if (dist_to_bottom < min_dist) { min_dist = dist_to_bottom; side = 3; }
    
    if (side == 0) {
        *vx = -(*vx);
        *ball_x = left - radius;
    } else if (side == 1) {
        *vx = -(*vx);
        *ball_x = right + radius;
    } else if (side == 2) {
        *vy = -(*vy);
        *ball_y = top - radius;
    } else if (side == 3) {
        *vy = -(*vy);
        *ball_y = bottom + radius;
    }
}