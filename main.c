#include<stdio.h>
#include<stdlib.h>
#include<stdint.h>
#include<time.h>
#include<unistd.h>
#include<assert.h>
#include<getopt.h>

#define LH "─"
#define LV "│"
#define LDR "┌"
#define LDL "┐"
#define LUR "└"
#define LUL "┘"
#define EMPTY " "

struct Pixel {
    uint8_t color;
    char* content;
};

struct Screen{
    int weight;
    int height;
    struct Pixel screen[];
};

struct Configuration {
    uint32_t grid_line_num;
    uint32_t color;
};

struct Configuration global_control;

void screen_init(struct Screen** s, int weight, int height);
struct Pixel* screen_get_pixel(struct Screen* s, int x, int y);


struct Pixel* screen_get_pixel(struct Screen* s, int x, int y)
{
    struct Pixel *p = s->screen + (s->weight * x + y);
    return p;
}

int touch_x_border(struct Screen* s, int x) {
    return x <= 0 || x >= s->height;
}

int touch_y_border(struct Screen* s, int y) {
    return y <= 0 || y >= s->weight;
}
void draw_circle(struct Screen* s, int x, int y, int dx, int dy) {
    int i = x, j = y;
    while(dx != 0 || dy != 0) {
        i += dx;
        j += dy;
        if (dy != 0 && touch_y_border(s,j)) {
            dx = 0;
            dy = 1;
        }
        if (dx != 0 && touch_x_border(s,i)) {
            dx = 0;
            dy = 1;
        }
        struct Pixel* p = screen_get_pixel(s,i,j);
    }
}

void draw_box(struct Screen* s) {
    int first_line = 0, last_line = 0, first_v = 0, last_v = 0;
    struct Pixel* p = NULL;
    for (int i = 0; i < s->height; i++) {
        for(int j = 0; j < s->weight; j++)  {
            first_line = (i == 0);
            last_line = i == s->height - 1;
            first_v = (j == 0);
            last_v = j == s->weight - 1;
            p = screen_get_pixel(s,i,j); 
            p->content = EMPTY;
            p->color = 1;
            if (first_line) {
                p->content = LH;
            }
            if (last_line) {
                p->content = LH;
            }
            if (first_v) {
                p->content = LV;
            }
            if (last_v) {
                p->content = LV;
            }
            if (first_line && first_v) {
                p->content = LDR;
            }
            if (first_line && last_v) {
                p->content = LDL;
            }
            if (last_line && first_v) {
                p->content = LUR;
            }
            if (last_line && last_v) {
                p->content = LUL;
            }
        }
    }
}

void screen_init(struct Screen** s, int weight, int height) {
    *s = malloc(sizeof(struct Screen) + sizeof(struct Pixel) * weight * height);
    (*s)->weight = weight;
    (*s)->height= height;
    for (int i = 0; i < (*s)->height; i++) {
        for(int j = 0; j < (*s)->weight; j++)  {
            struct Pixel* p = screen_get_pixel(*s,i,j);
            p->content = EMPTY;
            p->color = 37;
        }
    }
}

void screen_destory(struct Screen* s) 
{
    free(s);
}

void print_screen(struct Screen* s) 
{
    for (int i = 0; i < s->height; i++) {
        for(int j = 0; j < s->weight; j++)  {
            struct Pixel* p = screen_get_pixel(s,i,j);
            //printf("\033[44;37;5m");
            //printf("\033[44;%d;5m",p->color);
            printf("\033[38;5;%dm",p->color);
            printf("%s", p->content);
            printf("\033[0m");
        }
        printf("\n");
    }
}

struct Vector2D {
    int x;
    int y;
};

typedef struct Vector2D Position;
struct GridLine{ Position head; Position direction;
    uint8_t color;
    char* shape;
};

// 按方向移动
int gridline_move(struct GridLine* g);
// 更改方向
int gridline_chdir(struct GridLine* g, Position dir);
// 检测是否触到边框
// 1111
// |LEFT|UPPER
#define NOT_REACH  0
#define REACH_UPPER 0x1
#define REACH_DOWN 0x2
#define REACH_LEFT 0x4
#define REACH_RIGHT 0x8
int screen_reach_border(struct Screen* s, Position* p) {
    int res = 0;
    if (s->weight-1 <= p->y) res |= REACH_RIGHT;
    if (p->y <= 0 ) res |= REACH_LEFT;
    if (s->height-1 <= p->x ) res |= REACH_DOWN;
    if (p->x <= 0 ) res |= REACH_UPPER;
    return res;
}

// ------------------- 在屏幕边缘生成grid

struct GridLine* gridline_new(int x, int y, int dx, int dy, uint8_t color) 
{
    struct GridLine * g = NULL;
    g = (struct GridLine *)malloc(sizeof(struct GridLine));
    g->head.x = x;
    g->head.y = y;
    g->direction.x = dx;
    g->direction.y = dy;
    g->color = color;
    return g;
}

void gridline_destroy(struct GridLine* g)
{
    free(g);
}

int gridline_move(struct GridLine* g) 
{
    g->head.x += g->direction.x;
    g->head.y += g->direction.y;
    if (g->direction.x != 0)  g->shape = LV;
    if (g->direction.y != 0)  g->shape = LH;
    return 0;
}

#define DIR_NO  0
#define DIR_UP  0x1
#define DIR_DOWN 0x2
#define DIR_LEFT 0x4
#define DIR_RIGHT 0x8
int check_dir(Position * p) 
{
    int res = 0;
    if (p->x > 0) res |= DIR_DOWN;
    if (p->x < 0) res |= DIR_UP;
    if (p->y > 0) res |= DIR_RIGHT;
    if (p->y < 0) res |= DIR_LEFT;
    return res;
}

Position TURN_LEFT = {0,-1};
Position TURN_RIGHT = {0,1};
Position TURN_UP = {-1,0};
Position TURN_DOWN = {1,0};

int gridline_chdir(struct GridLine* g, Position dir) {
    int res_last = check_dir(&g->direction);
    int res_now = check_dir(&dir);
    if (res_last & DIR_UP && res_now & DIR_RIGHT) {
        g->shape = LDR;
    }
    if (res_last & DIR_UP && res_now & DIR_LEFT) {
        g->shape = LDL;
    }
    if (res_last & DIR_DOWN && res_now & DIR_RIGHT) {
        g->shape = LUR;
    }
    if (res_last & DIR_DOWN && res_now & DIR_LEFT) {
        g->shape = LUL;
    }
    if (res_now & DIR_UP && res_last & DIR_RIGHT) {
        g->shape = LUL;
    }
    if (res_now & DIR_UP && res_last & DIR_LEFT) {
        g->shape = LUR;
    }
    if (res_now & DIR_DOWN && res_last & DIR_RIGHT) {
        g->shape = LDL;
    }
    if (res_now & DIR_DOWN && res_last & DIR_LEFT) {
        g->shape = LDR;
    }

    g->direction.x = dir.x;
    g->direction.y = dir.y;
    return 0; }

int screen_draw_gridline(struct Screen* s, struct GridLine* g) 
{
    struct Pixel* p = screen_get_pixel(s, g->head.x, g->head.y);
    p->content = g->shape;
    p->color = g->color;
    return 0;
}

int gridline_step(struct Screen* s, struct GridLine* g) {
    gridline_move(g);
    assert(! (g->head.x >= s->height || g->head.y >= s->weight));
    assert(! (g->head.x < 0 || g->head.y < 0));
    // 检查是否触到边界
    int border_res = screen_reach_border(s,&g->head);
    int dir_res = check_dir(&g->direction);
    Position *pos = &g->direction;
    // 随机决定是否转换方向
    if (rand()%8 == 0) {
        if (dir_res & DIR_UP || dir_res & DIR_DOWN) {
            if (rand()%2) {
                pos = &TURN_RIGHT;
            } else {
                pos = &TURN_LEFT;
            }
            if (border_res & REACH_LEFT) pos = &TURN_RIGHT; // 修改方向为右
            if (border_res & REACH_RIGHT) pos = &TURN_LEFT; // 修改方向为左
        } else {
            if (rand()%2) {
                pos = &TURN_DOWN;
            } else {
                pos = &TURN_UP;
            }
            // 需要换为上下方向
            if (border_res & REACH_UPPER) pos = &TURN_DOWN; // 修改方向为下
            if (border_res & REACH_DOWN) pos = &TURN_UP; // 修改方向为上
        }
    }
    // 方向的约束
    if ((border_res & REACH_UPPER) && (dir_res & DIR_UP) || (border_res & REACH_DOWN) && (dir_res & DIR_DOWN)) {
        // 任选方向
        if (rand()%2) {
            pos = &TURN_RIGHT;
        } else {
            pos = &TURN_LEFT;
        }
        // 需要换为左右方向
        if (border_res & REACH_LEFT) pos = &TURN_RIGHT; // 修改方向为右
        if (border_res & REACH_RIGHT) pos = &TURN_LEFT; // 修改方向为左
    }
    if (border_res & REACH_LEFT && dir_res & DIR_LEFT || border_res & REACH_RIGHT && dir_res & DIR_RIGHT) {
        if (rand()%2) {
            pos = &TURN_DOWN;
        } else {
            pos = &TURN_UP;
        }
        // 需要换为上下方向
        if (border_res & REACH_UPPER) pos = &TURN_DOWN; // 修改方向为下
        if (border_res & REACH_DOWN) pos = &TURN_UP; // 修改方向为上
    }
    gridline_chdir(g, *pos);
    screen_draw_gridline(s,g);
    return 0;
}

void print_reach_info(int res) 
{
    if (res & REACH_LEFT) printf("reach left\n");
    if (res & REACH_RIGHT) printf("reach right\n");
    if (res & REACH_UPPER) printf("reach upper\n");
    if (res & REACH_DOWN) printf("reach down\n");
}

void print_dir_info(int res) 
{
    if (res & DIR_LEFT) printf("dir left\n");
    if (res & DIR_RIGHT) printf("dir right\n");
    if (res & DIR_UP) printf("dir up\n");
    if (res & DIR_DOWN) printf("dir down\n");
}

void print_grid_info(struct Screen* s, struct GridLine* g) 
{
    printf("------\n");
    printf("position %d, %d\n", g->head.x, g->head.y);
    print_reach_info(screen_reach_border(s,&g->head));
    print_dir_info(check_dir(&g->direction));
}

Position* rand_direction() 
{
    int r = rand();
    switch (r%4) {
        case 0:
        return &TURN_LEFT;
        case 1:
        return &TURN_RIGHT;
        case 2:
        return &TURN_UP;
        case 3:
        return &TURN_DOWN;
    }
    return &TURN_LEFT;
}

Position rand_position(int x, int y) 
{
    Position p;
    do {
        p.x = rand() %(x-1);
    } while (p.x == 0);
    do {
        p.y = rand() %(y-1);
    } while (p.y == 0);
    return p;
}

struct GridLine* rand_gridline(struct Screen* s) {
    Position* dir = rand_direction();
    Position p = rand_position(s->height, s->weight);
    return gridline_new(p.x, p.y, dir->x, dir->y, rand()%255);
}
void set_default_param() 
{
    global_control.grid_line_num = 8;
    global_control.color = 1;
}

int ParseParam(int argc, char* argv[])
{
    int ch;
    while((ch =getopt(argc, argv, "n:c::")) != -1) {
        switch (ch ) {
            case 'n':
                sscanf(optarg,"%u", &global_control.grid_line_num);
                break;
            case 'c':
                global_control.color = 1;
                break;
            case '?':
                exit(0);
            default:
                break;
        }
    }
    return 0;
}

int main(int argc, char* argv[]) 
{
    srand(time(NULL));
    struct Screen* s;
    set_default_param();
    ParseParam(argc, argv);
    screen_init(&s, 64, 32);
    draw_box(s);
    struct GridLine* g[256];
    for (int i = 0; i < global_control.grid_line_num; i ++) {
        //g[i] = gridline_new(rand()%31,rand()%63,rand()/2?1:-1,0, rand()%255);
        g[i] = rand_gridline(s);
    }
    for(int k = 0; k < 50; k ++) {
        for (int i = 0; i < global_control.grid_line_num; i++) {
            // print_grid_info(s,g[i]);
            gridline_step(s,g[i]);
        }
        //printf("\033[0;0H"); // 光标移动到左上角
    }
    printf("\033[2J"); // 清屏
    print_screen(s);
    for (int i = 0; i < global_control.grid_line_num; i ++) {
        gridline_destroy(g[i]);
    }
    screen_destory(s);
}
