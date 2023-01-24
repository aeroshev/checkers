#include <ncurses.h>
#include <stdio.h>
#include <stdlib.h>

#define VGA_WIDTH 64
#define VGA_HEIGHT 48
#define CHECKER_R2 40
#define BOARD_SIZE 8

enum Colors {
    WHITE = 1,
    BLACK = 2,
    RED = 3,
    BLUE = 4
};

enum Player {
    EMPTY,
    PLAYER_1,
    PLAYER_2,
    PLAYER_1_QUEEN,
    PLAYER_2_QUEEN
};

enum Move {
    NO,
    YES_UPSIDE,
    YES_DOWNSIDE,
    CAN_EAT_UPSIDE,
    CAN_EAT_DOWNSIDE
};

struct game {
    int* board;
    int count_1;
    int count_2;
} typedef game;


const int CELL_PIXELS_X = VGA_HEIGHT / BOARD_SIZE;
const int CELL_PIXELS_Y = VGA_WIDTH / BOARD_SIZE;

const int CHECKER_R_X = CELL_PIXELS_X / 2;
const int CHECKER_R_Y = CELL_PIXELS_Y / 2;

void render_checker(int, int, int);
int can_move_left(game *, int, int, int);
int can_move_right(game *, int, int, int);
int move_left(game *, int , int, int);
int move_right(game *, int, int, int);
void render_map(game *);
void eat(game *, int);
int can_continue_game(game *);
game *init_game();
void end_game(game *);


game *init_game() {
    game *match = (game *)malloc(sizeof(game));

    match->board = (int *)malloc(BOARD_SIZE * BOARD_SIZE * sizeof(int));

    for (int i = 0; i < BOARD_SIZE; i++) {
        if (i % 2 == 0) {
            match->board[0 * BOARD_SIZE + i] = EMPTY;
            match->board[1 * BOARD_SIZE + i] = PLAYER_1;

            match->board[(BOARD_SIZE - 1) * BOARD_SIZE + i] = PLAYER_2;
            match->board[(BOARD_SIZE - 2) * BOARD_SIZE + i] = EMPTY;
        } else {
            match->board[0 * BOARD_SIZE + i] = PLAYER_1;
            match->board[1 * BOARD_SIZE + i] = EMPTY;
    
            match->board[(BOARD_SIZE - 1) * BOARD_SIZE + i] = EMPTY;
            match->board[(BOARD_SIZE - 2) * BOARD_SIZE + i] = PLAYER_2;
        }
        for (int j = 2; j < BOARD_SIZE - 2; j++) {
            match->board[j * BOARD_SIZE + i] = EMPTY;
        }
    }

    return match;
}

void end_game(game *game) {
    free(game->board);
    free(game);
}

int can_continue_game(game *game) {
    if (game->count_1 < 1) {
        return -1;
    }
    if (game->count_2 < 1) {
        return -2;
    }
    return 1;
}

void eat(game *game, int side) {
    if (side == PLAYER_1)
        game->count_2--;
    else
        game->count_1--;
}


int can_move_left(game *game, int pos_x, int pos_y, int side) {
    int field;
    if (side == PLAYER_1 || side == PLAYER_2_QUEEN) {
        // Upper half
        if (pos_x + 1 < BOARD_SIZE && pos_y - 1 >= 0) {
            // Edge collision
            field = game->board[(pos_x + 1) * BOARD_SIZE + (pos_y - 1)];
            if (field == EMPTY) {
                // Player collision
                return YES_UPSIDE;
            } else if (field != side) {
                return CAN_EAT_UPSIDE;
            }
        }
        return NO;
    } else {
        // Down half
        if (pos_x - 1 >= 0 && pos_y - 1 >= 0) {
            field = game->board[(pos_x - 1) * BOARD_SIZE + (pos_y - 1)];
            if (field == EMPTY) {
                return YES_DOWNSIDE;
            } else if (field != side) {
                return CAN_EAT_DOWNSIDE;
            }
        }
        return NO;
    }
}

int can_move_right(game *game, int pos_x, int pos_y, int side) {
    int field;
    if (side == PLAYER_1 || side == PLAYER_2_QUEEN) {
        // Upper half
        if (pos_x + 1 < BOARD_SIZE && pos_y + 1 < BOARD_SIZE) {
            field = game->board[(pos_x + 1) * BOARD_SIZE + (pos_y + 1)];
            // Edge collision
            if (field == EMPTY) {
                // Player collision
                return YES_UPSIDE;
            } else if (field != side) {
                return CAN_EAT_UPSIDE;
            }
        }
        return NO;
    } else {
        // Down half
        if (pos_x - 1 >= 0 && pos_y + 1 < BOARD_SIZE) {
            field = game->board[(pos_x - 1) * BOARD_SIZE + (pos_y + 1)];
            if (field == EMPTY) {
                return YES_DOWNSIDE;
            } else if (field != side) {
                return CAN_EAT_DOWNSIDE;
            }
        }
        return NO;
    }
}

int move_left(game *game, int pos_x, int pos_y, int side) {
    int answer = can_move_left(game, pos_x, pos_y, side);

    game->board[pos_x * BOARD_SIZE + pos_y] = EMPTY;

    if (answer == YES_UPSIDE) {
        ++pos_x;
        --pos_y;
        game->board[pos_x * BOARD_SIZE + pos_y] = side;
        return 1;
    } else if (answer == YES_DOWNSIDE) {
        --pos_x;
        --pos_y;
        game->board[pos_x * BOARD_SIZE + pos_y] = side;
        return 1;
    } else if (answer == CAN_EAT_UPSIDE) {
        if (move_left(game, pos_x + 1, pos_y - 1, side)) {
            eat(game, side);
            return 1;
        }
    } else if (answer == CAN_EAT_DOWNSIDE) {
        if (move_left(game, pos_x - 1, pos_y - 1, side)) {
            eat(game, side);
            return 1;
        }
    }

    game->board[pos_x * BOARD_SIZE + pos_y] = side;

    return -1;
}

int move_right(game *game, int pos_x, int pos_y, int side) {
    int answer = can_move_right(game, pos_x, pos_y, side);

    game->board[pos_x * BOARD_SIZE + pos_y] = EMPTY;

    if (answer == YES_UPSIDE) {
        ++pos_x;
        ++pos_y;
        game->board[pos_x * BOARD_SIZE + pos_y] = side;
        return 1;
    } else if (answer == YES_DOWNSIDE) {
        --pos_x;
        ++pos_y;
        game->board[pos_x * BOARD_SIZE + pos_y] = side;
        return 1;
    } else if (answer == CAN_EAT_UPSIDE) {
        if (move_right(game, pos_x + 1, pos_y + 1, side)) {
            eat(game, side);
            return 1;
        }
    } else if (answer == CAN_EAT_DOWNSIDE) {
        if (move_right(game, pos_x - 1, pos_y + 1, side)) {
            eat(game, side);
            return 1;
        }
    }

    game->board[pos_x * BOARD_SIZE + pos_y] = side;

    return -1;
}

void render_checker(int pos_x, int pos_y, int player) {
	// int start_x = (CELL_PIXELS_X * pos_x);
	// int start_y = (CELL_PIXELS_Y * pos_y);

    // int center_x = start_x + CELL_PIXELS_X / 2;
    // int center_y = start_y + CELL_PIXELS_Y / 2;

    // int Index;
    // int cicrle_x, circle_y;
    // for (int x = 0; x < CELL_PIXELS_X; x++) {
    // 	for (int y = 0; y < CELL_PIXELS_Y; y++) {
    // 		Index = VGA_WIDTH * (start_x + x) + (start_y + y);
    // 		cicrle_x = start_x + x - center_x;
    // 		circle_y = start_y + y - center_y;
    //     	if ((cicrle_x * cicrle_x + circle_y * circle_y) < CHECKER_R2) {
    //             attron(COLOR_PAIR(player));
    //             mvaddch(cicrle_x, circle_y, ' ');
    //             attroff(COLOR_PAIR(player));
    //     	}
    // 	}
    // }
    int center_x = (CELL_PIXELS_X * pos_x) + CELL_PIXELS_X / 2;
    int center_y = (CELL_PIXELS_Y * pos_y) + CELL_PIXELS_Y / 2;

    attron(COLOR_PAIR(player));
    mvaddch(center_x, center_y, ' ');
    attroff(COLOR_PAIR(player));
}

void render_map(game *game) {
    // Render board
    int accum_x = 0, accum_y = 0;
    int pos_x = 0, pos_y = 0;
    for (int i = 0; i < VGA_HEIGHT; i++) {
        accum_x++;
        if (accum_x == CELL_PIXELS_X) {
            accum_x = 0;
            pos_x++;
        }
        pos_y = 0;
        for (int j = 0; j < VGA_WIDTH; j++) {
            accum_y++;
            if (accum_y == CELL_PIXELS_Y) {
                accum_y = 0;
                pos_y++;
            }

            // Render cells
            if (pos_x % 2 == 0) {
                if (pos_y % 2 == 0) {
                    attron(COLOR_PAIR(WHITE));
                    mvaddch(i, j, ' ');
                    attroff(COLOR_PAIR(WHITE));
                } else {
                    attron(COLOR_PAIR(BLACK));
                    mvaddch(i, j, ' ');
                    attroff(COLOR_PAIR(BLACK));
                }
            } else {
                if (pos_y % 2 == 1) {
                    attron(COLOR_PAIR(WHITE));
                    mvaddch(i, j, ' ');
                    attroff(COLOR_PAIR(WHITE));
                } else {
                    attron(COLOR_PAIR(BLACK));
                    mvaddch(i, j, ' ');
                    attroff(COLOR_PAIR(BLACK));
                }
            }

            // Render checkers
            if (game->board[pos_x * BOARD_SIZE + pos_y] == PLAYER_1) {
                render_checker(pos_x, pos_y, RED);
            }
            if (game->board[pos_x * BOARD_SIZE + pos_y] == PLAYER_2) {
                render_checker(pos_x, pos_y, BLUE);
            }
        }
    }
}


int main() {
    // Init ncurses
    initscr();
    keypad(stdscr, TRUE);
    cbreak();
    noecho();

    // Init colors
    start_color();
    init_pair(WHITE, COLOR_WHITE, COLOR_WHITE);
    init_pair(BLACK, COLOR_BLACK, COLOR_BLACK);
    init_pair(RED, COLOR_RED, COLOR_RED);
    init_pair(BLUE, COLOR_BLUE, COLOR_BLUE);


    game *game = init_game();
    int round = PLAYER_1;
    char move;
    int field, coord_x, coord_y;
    int can_play;


    while(true) {
        render_map(game);
        refresh();
        print("Round %d\n", round);

        coord_x = getch() - 48;
        coord_y = getch() - 48;
        if (!((-1 < coord_x) && (coord_x < BOARD_SIZE) && (-1 < coord_y) && (coord_y < BOARD_SIZE)))
            continue;
 
        field = game->board[coord_x * BOARD_SIZE + coord_y];

        if (field != EMPTY && field == round) {
            move = getch();
            switch (move) 
            {
                case 'l':
                    move_left(game, coord_x, coord_y, round);
                    break;
                case 'r':
                    move_right(game, coord_x, coord_y, round);
                    break;
            }
            if (!(can_play = can_continue_game(game))) {
                switch (can_play)
                {
                case -1:
                    printf("Win Player 2\n");
                    break;
                case -2:
                    printf("Win Player 1\n");
                    break;
                }
                break;
            }
            round = (round == PLAYER_1) ? PLAYER_2 : PLAYER_1;
        }
    };

    end_game(game);
    endwin();

    return 0;
}
