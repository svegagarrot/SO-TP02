#include "include/lib.h"
#include "include/game.h"
#include "include/game_functions.h"


// Pantalla principal del juego
void game_main_screen() {
    reset_level();
    clear_key_buffer();
    init_screen_dimensions();
    video_clearScreenColor(COLOR_BG_HOME);
    const char *lines[] = {
        "Bienvenido a Pongis-Golf",
        "Presione 1 para jugar",
        "Presione 2 para dos jugadores",
        "Presione ESC para salir",
        "Presione CTRL+R para tomar un snapshot"
    };
    int num_lines = 5;
    int line_height = 20;
    int font_width = 8;
    int total_height = num_lines * line_height;
    int startY = (SCREEN_HEIGHT - total_height) / 2;
    for (int i = 0; i < num_lines; i++) {
        size_t text_len = strlen(lines[i]);
        int text_width = text_len * font_width;
        int x = (SCREEN_WIDTH - text_width) / 2;
        int y = startY + i * line_height;
        drawText(x, y, lines[i], COLOR_TEXT_HOME);
    }
    while (1) {
        if (is_key_pressed_syscall((unsigned char)SC_1)) {
            clear_key_buffer(); 
            game_start(1);
            clear_key_buffer();
            break;
        } else if (is_key_pressed_syscall((unsigned char)SC_2)) {
            clear_key_buffer();
            game_start(2);
            clear_key_buffer(); 
            break;
        } else if (is_key_pressed_syscall((unsigned char)SC_ESC)) {
            clear_key_buffer();
            clearScreen();
            return;
        }
        for (volatile int i = 0; i < 100000; i++);
    }
}

// Variables estáticas para contar victorias en modo multijugador
static int victorias_p1 = 0;
static int victorias_p2 = 0;
static int reset_victorias = 1; 

// Lógica principal del juego
void game_start(int num_players) {

    if (reset_victorias) {
        victorias_p1 = 0;
        victorias_p2 = 0;
        reset_victorias = 0;
    }
    
    video_clearScreenColor(COLOR_BG_GREEN);
    int margin = 50;
    int hole_radius = get_hole_radius(get_current_level()); 
    int hole_x = rand_range(margin + hole_radius, SCREEN_WIDTH - margin - hole_radius);
    int hole_y = rand_range(UI_TOP_MARGIN + margin + hole_radius, SCREEN_HEIGHT - margin - hole_radius);
    Player players[2] = {0};

    // Configuración de jugadores
    players[0].color = COLOR_PLAYER1;
    players[0].arrow_color = COLOR_TEXT_HOME;
    players[0].ball_color = COLOR_WHITE;
    players[0].control_up = SC_UP;     
    players[0].control_left = SC_LEFT;  
    players[0].control_right = SC_RIGHT; 
    players[0].name = "Rosa";

    if (num_players == 2) {
        players[1].color = COLOR_PLAYER2;
        players[1].arrow_color = COLOR_TEXT_HOME;
        players[1].ball_color = COLOR_BALL2;
        players[1].control_up = SC_W;     
        players[1].control_left = SC_A;   
        players[1].control_right = SC_D;  
        players[1].name = "Azul";
    }

    //Obstáculos
    static int obstaculos_generados = 0;
    Obstacle obstacles[MAX_OBSTACLES];
    int num_obstacles = 0;
    if (!obstaculos_generados) {
        generate_obstacles(obstacles, &num_obstacles, get_current_level(), hole_x, hole_y);
        obstaculos_generados = 1;
    } else {
        generate_obstacles(obstacles, &num_obstacles, get_current_level(), hole_x, hole_y);
    }

    // Inicialización de posiciones
    for (int i = 0; i < num_players; i++) {
        do {
            players[i].x = rand_range(margin + PLAYER_RADIUS, SCREEN_WIDTH - margin - PLAYER_RADIUS);
            players[i].y = rand_range(UI_TOP_MARGIN + margin + PLAYER_RADIUS, SCREEN_HEIGHT - margin - PLAYER_RADIUS);
        } while (circles_overlap(players[i].x, players[i].y, PLAYER_RADIUS + 10, hole_x, hole_y, hole_radius + 10) ||
                 (i == 1 && circles_overlap(players[0].x, players[0].y, PLAYER_RADIUS * 2 + 10, players[1].x, players[1].y, PLAYER_RADIUS + 10)) ||
                 circle_obstacle_collision(players[i].x, players[i].y, PLAYER_RADIUS + 10, obstacles, num_obstacles, NULL));

        do {
            players[i].ball_x = rand_range(margin + 5, SCREEN_WIDTH - margin - 5);
            players[i].ball_y = rand_range(UI_TOP_MARGIN + margin + 5, SCREEN_HEIGHT - margin - 5);
        } while (
            circles_overlap(players[i].ball_x, players[i].ball_y, 5 + 10, hole_x, hole_y, hole_radius + 10) ||
            circles_overlap(players[i].ball_x, players[i].ball_y, 5 + 10, players[i].x, players[i].y, PLAYER_RADIUS + 10) ||
            (i == 1 && circles_overlap(players[0].ball_x, players[0].ball_y, 5 + 10, players[1].ball_x, players[1].ball_y, 5 + 10)) ||
            circle_obstacle_collision(players[i].ball_x, players[i].ball_y, 5 + 10, obstacles, num_obstacles, NULL)
        );

        players[i].prev_x = players[i].x;
        players[i].prev_y = players[i].y;
        players[i].prev_angle = players[i].angle = 0;
        players[i].prev_ball_x = players[i].ball_x;
        players[i].prev_ball_y = players[i].ball_y;
        players[i].golpes = 0;
        players[i].puede_golpear = 1;
        players[i].ball_vx = 0;
        players[i].ball_vy = 0;
        players[i].ball_in_hole = 0;
        players[i].debe_verificar_derrota = 0;
    }

    draw_obstacles(obstacles, num_obstacles);

    // Dibujo inicial
    drawCircle(hole_x, hole_y, hole_radius, COLOR_BLACK);
    for (int i = 0; i < num_players; i++) {
        drawCircle(players[i].x, players[i].y, PLAYER_RADIUS, players[i].color);
        drawCircle(players[i].ball_x, players[i].ball_y, 5, players[i].ball_color);
        drawPlayerArrow(players[i].x, players[i].y, players[i].angle, hole_x, hole_y, hole_radius, players[i].arrow_color, players, num_players, obstacles, num_obstacles);
    }

    drawFullWidthBar(0, 16, COLOR_BLACK);
    drawFullWidthBar(16, 16, COLOR_BLACK);

    int last_golpes_p1 = -1, last_golpes_p2 = -1;
    char status_str[120];

    if (num_players == 1) {
        sprintf(status_str, "Nivel: %d | Golpes: %d | Victorias: %d", get_current_level(), players[0].golpes, victorias_p1);
        drawTextFixed(10, 0, status_str, COLOR_WHITE, COLOR_BLACK);
        last_golpes_p1 = players[0].golpes;
    } else {
        sprintf(status_str, "Nivel: %d | Toques P1: %d | Toques P2: %d | Victorias P1: %d | Victorias P2: %d", 
                get_current_level(), players[0].golpes, players[1].golpes, victorias_p1, victorias_p2);
        drawTextFixed(10, 0, status_str, COLOR_WHITE, COLOR_BLACK);
        last_golpes_p1 = players[0].golpes;
        last_golpes_p2 = players[1].golpes;
    }

    int ganador = -1;
    while (1) {
        if (num_players == 1) {
            if (players[0].golpes != last_golpes_p1) {
                sprintf(status_str, "Nivel: %d | Golpes: %d | Victorias: %d", get_current_level(), players[0].golpes, victorias_p1);
                drawTextFixed(10, 0, status_str, COLOR_WHITE, COLOR_BLACK);
                last_golpes_p1 = players[0].golpes;
            }
        } else {
            if (players[0].golpes != last_golpes_p1 || players[1].golpes != last_golpes_p2) {
                sprintf(status_str, "Nivel: %d | Toques P1: %d | Toques P2: %d | Victorias P1: %d | Victorias P2: %d", 
                        get_current_level(), players[0].golpes, players[1].golpes, victorias_p1, victorias_p2);
                drawTextFixed(10, 0, status_str, COLOR_WHITE, COLOR_BLACK);
                last_golpes_p1 = players[0].golpes;
                last_golpes_p2 = players[1].golpes;
            }
        }

        // Verificar si se presiona ESC para salir
        if (is_key_pressed_syscall((unsigned char)SC_ESC)) {
            reset_level(); 
            clear_key_buffer(); 
            if (num_players == 2) {
                char final_msg[200];
                if (victorias_p1 > victorias_p2) {
                    sprintf(final_msg, "GANA JUGADOR ROSA! Rosa %d - Azul %d", victorias_p1, victorias_p2);
                } else if (victorias_p2 > victorias_p1) {
                    sprintf(final_msg, "GANA JUGADOR AZUL! Rosa %d - Azul %d", victorias_p1, victorias_p2);
                } else {
                    sprintf(final_msg, "EMPATE! Rosa %d - Azul %d", victorias_p1, victorias_p2);
                }
                displayFullScreenMessage(final_msg, COLOR_TEXT_HOME);
                sleep(5000);
                reset_victorias = 1; 
            } else {
                char solo_msg[100];
                sprintf(solo_msg, "Victorias totales: %d", victorias_p1);
                displayFullScreenMessage(solo_msg, COLOR_TEXT_HOME);
                sleep(3000);
                reset_victorias = 1;
            }
            clearScreen();
            return;
        }

        // Control de jugadores
        for (int i = 0; i < num_players; i++) {
            if (!players[i].ball_in_hole) {
                if (is_key_pressed_syscall((unsigned char)players[i].control_up)) {
                    int step = 1;
                    int new_x = players[i].x + (cos_table[players[i].angle] * step) / 10;
                    int new_y = players[i].y + (sin_table[players[i].angle] * step) / 10;
                    if (new_x >= PLAYER_RADIUS && new_x <= SCREEN_WIDTH - PLAYER_RADIUS &&
                        new_y >= UI_TOP_MARGIN + PLAYER_RADIUS && new_y <= SCREEN_HEIGHT - PLAYER_RADIUS) {
                        int dx = new_x - hole_x;
                        int dy = new_y - hole_y;
                        if (dx*dx + dy*dy > (PLAYER_RADIUS + hole_radius)*(PLAYER_RADIUS + hole_radius)
                            && !circle_obstacle_collision(new_x, new_y, PLAYER_RADIUS, obstacles, num_obstacles, NULL)) {
                            players[i].x = new_x;
                            players[i].y = new_y;
                        }
                    }
                }
                
                if (is_key_pressed_syscall((unsigned char)players[i].control_left)) {
                    players[i].angle = (players[i].angle + 35) % 36;
                }
                
                if (is_key_pressed_syscall((unsigned char)players[i].control_right)) {
                    players[i].angle = (players[i].angle + 1) % 36;
                }
            }
        }

        for (int i = 0; i < num_players; i++) {
            for (int j = 0; j < num_players; j++) {
                if (!players[j].ball_in_hole) {
                    int dx = players[j].ball_x - players[i].x;
                    int dy = players[j].ball_y - players[i].y;
                    int dist_squared = dx*dx + dy*dy;
                    if (dist_squared <= (PLAYER_RADIUS + 5)*(PLAYER_RADIUS + 5) && players[i].puede_golpear) {
                        if (i == j && players[i].golpes >= 6 && ganador == -1) {
                            ganador = (i == 0) ? 1 : 0; 
                            players[i].debe_verificar_derrota = 1;
                            char msg[120];
                            if (num_players == 1) {
                                sprintf(msg, "DERROTA! No lograste meter la pelota en 6 golpes. Presiona Espacio/ENTER para reintentar o ESC para salir.");
                            } else {
                                int perdedor = i;
                                int ganador_idx = (i == 0) ? 1 : 0;
                                sprintf(msg, "GANA %s! %s no logro meter la pelota en 6 golpes. Presiona Espacio/ENTER para seguir o ESC para salir.", players[ganador_idx].name, players[perdedor].name);
                            }
                            displayFullScreenMessage(msg, COLOR_TEXT_HOME);
                            
                            beep(392, 250);
                            
                            sleep(100);
                            
                            clear_key_buffer();
                            
                            while (1) {
                                if (is_key_pressed_syscall((unsigned char)SC_ESC)) {
                                    reset_level();
                                    clear_key_buffer();
                                    
                                    if (num_players == 2) {
                                        char final_msg[200];
                                        if (victorias_p1 > victorias_p2) {
                                            sprintf(final_msg, "GANA JUGADOR ROSA! Victorias: Rosa %d - Azul %d", victorias_p1, victorias_p2);
                                        } else if (victorias_p2 > victorias_p1) {
                                            sprintf(final_msg, "GANA JUGADOR AZUL! Victorias: Rosa %d - Azul %d", victorias_p1, victorias_p2);
                                        } else {
                                            sprintf(final_msg, "EMPATE! Victorias: Rosa %d - Azul %d", victorias_p1, victorias_p2);
                                        }
                                        displayFullScreenMessage(final_msg, COLOR_TEXT_HOME);
                                        sleep(5000);
                                        reset_victorias = 1;
                                    } else {
                                        char solo_msg[100];
                                        sprintf(solo_msg, "Victorias: %d", victorias_p1);
                                        displayFullScreenMessage(solo_msg, COLOR_TEXT_HOME);
                                        sleep(3000);
                                        reset_victorias = 1;
                                    }
                                    clearScreen(); 
                                    return; 
                                }
                                else if (is_key_pressed_syscall((unsigned char)SC_SPACE) || 
                                         is_key_pressed_syscall((unsigned char)SC_ENTER)) {
                                    clearScreen();
                                    game_start(num_players);
                                    return;
                                }
                                
                                sleep(30);
                            }
                        }
                        players[j].ball_vx = (cos_table[players[i].angle] * POWER_FACTOR) / 10;
                        players[j].ball_vy = (sin_table[players[i].angle] * POWER_FACTOR) / 10;
                        if (i == j && players[i].golpes < 6) {
                            players[i].golpes++;
                        }
                        players[i].puede_golpear = 0;
                        audiobounce();
                    }
                }
            }
        }

        for (int i = 0; i < num_players; i++) {
            int dx = players[i].ball_x - players[i].x;
            int dy = players[i].ball_y - players[i].y;
            if (dx*dx + dy*dy > (PLAYER_RADIUS + 5)*(PLAYER_RADIUS + 5)) 
                players[i].puede_golpear = 1;
        }

        //FISICA DE LA PELOTA 
        for (int i = 0; i < num_players; i++) {
            if ((players[i].ball_vx != 0 || players[i].ball_vy != 0) && !players[i].ball_in_hole) {
                players[i].ball_x += players[i].ball_vx / 10;
                players[i].ball_y += players[i].ball_vy / 10;
                
                int idx = -1;
                if (circle_obstacle_collision(players[i].ball_x, players[i].ball_y, 5, obstacles, num_obstacles, &idx)) {
                    Obstacle *o = &obstacles[idx];
                    int left = o->x, right = o->x + o->size;
                    int top = o->y, bottom = o->y + o->size;
                    

                    int center_x = players[i].ball_x;
                    int center_y = players[i].ball_y;
                    
                    int dist_to_left = center_x - left;
                    int dist_to_right = right - center_x;
                    int dist_to_top = center_y - top;
                    int dist_to_bottom = bottom - center_y;
                    
                    int min_dist = dist_to_left;
                    int side = 0; 
                    
                    if (dist_to_right < min_dist) { min_dist = dist_to_right; side = 1; }
                    if (dist_to_top < min_dist) { min_dist = dist_to_top; side = 2; }
                    if (dist_to_bottom < min_dist) { min_dist = dist_to_bottom; side = 3; }
                    
                    if (side == 0 || side == 1) {
                        players[i].ball_vx = -players[i].ball_vx;
                        if (side == 0) players[i].ball_x = left - 5;  
                        else players[i].ball_x = right + 5; 
                    } else {
                        players[i].ball_vy = -players[i].ball_vy;
                        if (side == 2) players[i].ball_y = top - 5;
                        else players[i].ball_y = bottom + 5;
                    }
                    audiobounce();
                }
                
                players[i].ball_vx = (players[i].ball_vx * 95) / 100;
                players[i].ball_vy = (players[i].ball_vy * 95) / 100;
                if (players[i].ball_vx < 1 && players[i].ball_vx > -1) players[i].ball_vx = 0;
                if (players[i].ball_vy < 1 && players[i].ball_vy > -1) players[i].ball_vy = 0;
            }
        }
        // Rebote en bordes
        for (int i = 0; i < num_players; i++) {
            if (players[i].ball_x < 5) { players[i].ball_x = 5; players[i].ball_vx = -players[i].ball_vx; audiobounce(); }
            if (players[i].ball_x > SCREEN_WIDTH - 5) { players[i].ball_x = SCREEN_WIDTH - 5; players[i].ball_vx = -players[i].ball_vx; audiobounce(); }
            if (players[i].ball_y < UI_TOP_MARGIN + 5) { players[i].ball_y = UI_TOP_MARGIN + 5; players[i].ball_vy = -players[i].ball_vy; audiobounce(); }
            if (players[i].ball_y > SCREEN_HEIGHT - 5) { players[i].ball_y = SCREEN_HEIGHT - 5; players[i].ball_vy = -players[i].ball_vy; audiobounce(); }
        }
        
        for (int i = 0; i < num_players; i++) {
            int hx = players[i].ball_x - hole_x, hy = players[i].ball_y - hole_y;
            if (hx*hx + hy*hy <= hole_radius*hole_radius && !players[i].ball_in_hole && ganador == -1) {
                players[i].ball_in_hole = 1;
                players[i].ball_vx = 0;
                players[i].ball_vy = 0;
                players[i].ball_x = hole_x;
                players[i].ball_y = hole_y;
                ganador = i;
                
                if (num_players == 2) {
                    if (i == 0) {
                        victorias_p1++;
                    } else {
                        victorias_p2++;
                    }
                }
            }
        }

        for (int i = 0; i < num_players; i++) {
            if (!players[i].ball_in_hole && players[i].golpes >= 6 && 
                players[i].ball_vx == 0 && players[i].ball_vy == 0 && 
                ganador == -1) {
                if (num_players == 1) {
                    ganador = -2;
                } 
                else {
                    ganador = (i == 0) ? 1 : 0;
                    if (ganador == 0) {
                        victorias_p1++;
                    } else {
                        victorias_p2++;
                    }
                }
            }
        }

        drawCircle(hole_x, hole_y, hole_radius, COLOR_BLACK);
        for (int i = 0; i < num_players; i++) {
            int playerMoved = (players[i].prev_x != players[i].x || players[i].prev_y != players[i].y);
            int ballMoved = (players[i].prev_ball_x != players[i].ball_x || players[i].prev_ball_y != players[i].ball_y);
            int arrowChanged = (!players[i].ball_in_hole && (players[i].prev_angle != players[i].angle || playerMoved));
            if (playerMoved) {
                erasePlayerSmart(players[i].prev_x, players[i].prev_y, players, num_players, hole_x, hole_y, hole_radius, obstacles, num_obstacles);
                drawCircle(players[i].x, players[i].y, PLAYER_RADIUS, players[i].color);
            }
            if (ballMoved) {
                eraseBallSmart(players[i].prev_ball_x, players[i].prev_ball_y, players, num_players, hole_x, hole_y, hole_radius, obstacles, num_obstacles);
                drawCircle(players[i].ball_x, players[i].ball_y, 5, players[i].ball_color);
            }
            if (arrowChanged) {
                eraseArrow(players[i].prev_x, players[i].prev_y, players[i].prev_angle, hole_x, hole_y, hole_radius, players, num_players, obstacles, num_obstacles);
                drawPlayerArrow(players[i].x, players[i].y, players[i].angle, hole_x, hole_y, hole_radius, players[i].arrow_color, players, num_players, obstacles, num_obstacles);
            }
            players[i].prev_x = players[i].x;
            players[i].prev_y = players[i].y;
            players[i].prev_angle = players[i].angle;
            players[i].prev_ball_x = players[i].ball_x;
            players[i].prev_ball_y = players[i].ball_y;
        }

        // Si hay ganador
        if (ganador != -1) {
            char victory_msg[120];
            if (num_players == 1) {
                if (ganador == -2) { 
                    sprintf(victory_msg, "DERROTA! No lograste meter la pelota en 6 golpes. Presiona Espacio/ENTER para reintentar o ESC para salir.");
                } else if (players[0].ball_in_hole) {
                    sprintf(victory_msg, "VICTORIA! Hoyo completado en %d golpes. Presiona Espacio/ENTER para seguir o ESC para salir.", players[0].golpes);
                    victorias_p1++; // Incrementar victorias en solitario
                }
            } else {
                // Multijugador: determinar quién ganó y actualizar victorias
                if (players[ganador].ball_in_hole) {
                    sprintf(victory_msg, "GANA %s! Logro meter la pelota en %d golpes. Presiona Espacio/ENTER para seguir o ESC para salir.", 
                        players[ganador].name, players[ganador].golpes);
                    if (ganador == 0) {
                        victorias_p1++;
                    } else {
                        victorias_p2++;
                    }
                } else {
                    int perdedor = (ganador == 0) ? 1 : 0;
                    sprintf(victory_msg, "GANA %s! %s no logro meter la pelota en 6 golpes. Presiona Espacio/ENTER para seguir o ESC para salir.", 
                        players[ganador].name, players[perdedor].name);
                    if (ganador == 0) {
                        victorias_p1++;
                    } else {
                        victorias_p2++;
                    }
                }
            }
            displayFullScreenMessage(victory_msg, COLOR_TEXT_HOME);
            
            
            play_mission_impossible();
            sleep(100);
            clear_key_buffer();
            while (1) {
                if (is_key_pressed_syscall((unsigned char)SC_ESC)) { 
                    reset_level(); 
                    clear_key_buffer();
                    if (num_players == 2) {
                        char final_msg[200];
                        if (victorias_p1 > victorias_p2) {
                            sprintf(final_msg, "GANA JUGADOR ROSA! Victorias: Rosa %d - Azul %d", victorias_p1, victorias_p2);
                        } else if (victorias_p2 > victorias_p1) {
                            sprintf(final_msg, "GANA JUGADOR AZUL! Victorias: Rosa %d - Azul %d", victorias_p1, victorias_p2);
                        } else {
                            sprintf(final_msg, "EMPATE! Victorias: Rosa %d - Azul %d", victorias_p1, victorias_p2);
                        }
                        displayFullScreenMessage(final_msg, COLOR_TEXT_HOME);
                        sleep(5000); 
                        reset_victorias = 1;
                    } else {
                        char solo_msg[100];
                        sprintf(solo_msg, "Victorias: %d", victorias_p1);
                        displayFullScreenMessage(solo_msg, COLOR_TEXT_HOME);
                        sleep(3000);
                        reset_victorias = 1;
                    }
                    clearScreen(); 
                    return; 
                }
                else if (is_key_pressed_syscall((unsigned char)SC_SPACE) || 
                         is_key_pressed_syscall((unsigned char)SC_ENTER)) {
                    clearScreen();
                    if (ganador != -2) {
                        increment_level();
                    }
                    game_start(num_players);
                    return;
                }
                
                sleep(30); 
            }
        }

        sleep(25);
    }
}
