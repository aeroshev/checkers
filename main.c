#include <ncurses.h>
#include <stdio.h>

#define WHITE 1
#define BLACK 2
#define RED 3
#define BLUE 4


const int VGA_WEIGHT = 64;
const int VGA_HEIGHT = 48;

char screen[VGA_WEIGHT * VGA_HEIGHT];

const int BOARD_SIZE = 8;

const int CELL_PIXELS_X = VGA_HEIGHT / BOARD_SIZE;
const int CELL_PIXELS_Y = VGA_WEIGHT / BOARD_SIZE;

const int CHECKER_R_X = CELL_PIXELS_X / 2;
const int CHECKER_R_Y = CELL_PIXELS_Y / 2;

void render_checker(int, int, int);
void move_left(int *, int *, int);
void render_map();


int game[BOARD_SIZE][BOARD_SIZE] = {
    {1, 0, 1, 0, 1, 0, 1, 0},
    {0, 1, 0, 1, 0, 1, 0, 1},
    {0, 0, 0, 0, 0, 0, 0, 0},
    {0, 0, 0, 0, 0, 0, 0, 0},
    {0, 0, 0, 0, 0, 0, 0, 0},
    {0, 0, 0, 0, 0, 0, 0, 0},
    {0, 2, 0, 2, 0, 2, 0, 2},
    {2, 0, 2, 0, 2, 0, 2, 0}
};


struct checker {
    int color;
    int num;
    int pos_x;
    int pos_y;
} typedef checker;


void move_left(int *pos_x, int *pos_y, int side) {
    if (side == 1) {
        // Upper half
        if (*pos_x + 1 < BOARD_SIZE && *pos_y - 1 >= 0) {
            game[*pos_x][*pos_y] = 0;
            ++*pos_x;
            --*pos_y;
            game[*pos_x][*pos_y] = side;
        }
    } else {
        if (pos_x - 1 >= 0 && pos_y - 1 >= 0) {
            game[*pos_x][*pos_y] = 0;
            --*pos_x;
            --*pos_y;
            game[*pos_x][*pos_y] = side;
        }
    }
}

void move_right(int *pos_x, int *pos_y, int side) {
    if (side == 1) {
        if (*pos_x + 1 < BOARD_SIZE && *pos_y + 1 < BOARD_SIZE) {
            game[*pos_x][*pos_y] = 0;
            ++*pos_x;
            ++*pos_y;
            game[*pos_x][*pos_y] = side;
        }
    } else {
        if (*pos_x - 1 >= 0 && *pos_y + 1 < BOARD_SIZE) {
            game[*pos_x][*pos_y] = 0;
            --*pos_x;
            ++*pos_y;
            game[*pos_x][*pos_y] = side;
        }
    }
}

void render_checker(int pos_x, int pos_y, int color) {
    int center_x = (CELL_PIXELS_X * pos_x) + CELL_PIXELS_X / 2;
    int center_y = (CELL_PIXELS_Y * pos_y) + CELL_PIXELS_Y / 2;

    attron(COLOR_PAIR(color));
    mvaddch(center_x, center_y, ' ');
    attroff(COLOR_PAIR(color));
}

void render_map() {
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
        for (int j = 0; j < VGA_WEIGHT; j++) {
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

            if (game[pos_x][pos_y] == 1) {
                render_checker(pos_x, pos_y, RED);
            }
            if (game[pos_x][pos_y] == 2) {
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


    int coord_x = 1;
    int coord_y = 1;

    while(true) {
        render_map();
        refresh();

        char ch = getch();
        switch (ch) 
        {
            case 'l':
                move_left(&coord_x, &coord_y, 1);
                break;
            case 'r':
                move_right(&coord_x, &coord_y, 1);
                break;
        }

    };

    endwin();

    return 0;
}
