#include <stdio.h>
#include <string.h>
#include <sys/ioctl.h>
#include <termios.h>
#include <unistd.h>

#define PADDLE_WIDTH 15
#define ESC "\033"
#define BLUE "\033[34m"
#define BLOCK "\u2588"
#define RESET "\033[0m"
#define CLEAR "\033[2J"

typedef enum { LEFT = -2, RIGHT = 2 } Direction;

typedef enum { UP = -1, DOWN = 1 } BallDirection;

typedef struct {
  int x, y;
} Paddle;

typedef struct {
  int x, y, angle;
  BallDirection dir;
} Ball;

const char *ball = BLUE BLOCK BLOCK RESET;

// ------ TERMMINAL

static struct termios orig;

void get_terminal_size(int *rows, int *cols) {
  struct winsize w;
  ioctl(STDOUT_FILENO, TIOCGWINSZ, &w);
  *rows = w.ws_row;
  *cols = w.ws_col;
}

void enable_raw_mode(void) {
  tcgetattr(STDIN_FILENO, &orig);
  struct termios raw = orig;

  raw.c_lflag &= ~(ECHO | ICANON | ISIG);
  raw.c_iflag &= ~(IXON | ICRNL);
  raw.c_oflag &= ~(OPOST);

  raw.c_cc[VMIN] = 0;
  raw.c_cc[VTIME] = 0;

  tcsetattr(STDIN_FILENO, TCSAFLUSH, &raw);
}

void disable_raw_mode(void) { tcsetattr(STDIN_FILENO, TCSAFLUSH, &orig); }

void cursor(int show) {
  char buf[8];
  int len = snprintf(buf, sizeof(buf), ESC "[?25%c", show == 1 ? 'h' : 'l');
  write(STDOUT_FILENO, buf, len);
}

void game_over() {
  char buf[32];
  int len = snprintf(buf, sizeof(buf), RESET ESC "[3;3HGame Over!");
  write(STDOUT_FILENO, buf, len);
}

// ------- PADDLE

void draw_paddle(Paddle *p) {
  char buf[32];
  int len = 0;

  len += snprintf(buf + len, sizeof(buf) - len, ESC "[%d;%dH", p->y, p->x);

  for (int i = 0; i < PADDLE_WIDTH; i++) {
    memcpy(buf + len, BLOCK, 3);
    len += 3;
  }

  write(STDOUT_FILENO, buf, len);
}

void move_paddle(Paddle *p, Direction dir) {
  char buf[32];

  int drawPos = (p->x + (dir == RIGHT ? PADDLE_WIDTH : -2));
  int erasePos = (p->x + (dir == RIGHT ? 0 : PADDLE_WIDTH - 2));

  write(STDOUT_FILENO, buf,
        snprintf(buf, sizeof(buf),
                 ESC "[%d;%dH" RESET BLOCK BLOCK ESC "[%d;%dH  ", p->y, drawPos,
                 p->y, erasePos));

  p->x += dir;
}

// -------- BALL

void move_ball(Ball *b, int *cols) {
  char buf[64];

  int len = 0;

  len += snprintf(buf + len, sizeof(buf) - len, ESC "[%d;%dH  ", b->y, b->x);

  b->y += b->dir;
  if (b->x + b->angle >= *cols || b->x + b->angle <= 1) {
    b->angle *= -1;
  }
  b->x += (b->y % 2) * b->angle;

  len +=
      snprintf(buf + len, sizeof(buf) - len, ESC "[%d;%dH%s", b->y, b->x, ball);

  write(STDOUT_FILENO, buf, len);
}

int get_angle_change(Ball *b, Paddle *p) {
  int half = (PADDLE_WIDTH - 1) / 2;
  int part = half / 3;
  int relativeX = b->x - p->x - half;
  int contactPart = relativeX / part + 1;
  return contactPart;
}

// ------- MAIN

int main() {
  write(STDOUT_FILENO, ESC "[?1049h", 8);
  enable_raw_mode();
  cursor(0);

  int rows, cols;
  get_terminal_size(&rows, &cols);

  Paddle p1 = {cols / 2 - 10, 3};
  Paddle p2 = {cols / 2 - 10, rows - 2};
  Ball b = {cols / 2 - 1, rows / 2, 2, DOWN};

  draw_paddle(&p1);
  draw_paddle(&p2);
  move_ball(&b, &cols);

  int frame_counter = 0;
  int ball_speed_divider = 10;

  int game_going = 1;

  while (1) {
    frame_counter++;

    if (frame_counter >= ball_speed_divider && game_going == 1) {
      move_ball(&b, &cols);

      if ((b.y == (p2.y - 1) || b.y == (p1.y + 1)) &&
          ((b.dir == UP && b.x >= p1.x - 1 && b.x < p1.x + PADDLE_WIDTH) ||
           (b.dir == DOWN && b.x >= p2.x - 1 && b.x < p2.x + PADDLE_WIDTH))) {

        b.dir = b.dir == UP ? DOWN : UP;
        write(STDOUT_FILENO, "\a", 1); // beep
        b.angle = get_angle_change(&b, b.y > rows / 2 ? &p2 : &p1);
      }

      if (b.y >= rows || b.y <= 0) {
        game_going = 0;
        game_over();
      }

      frame_counter = 0;
    }

    char c;
    if (read(STDIN_FILENO, &c, 1) == 1) {
      if (c == 'q' || c == 'Q')
        break;

      if (game_going == 1) {
        if ((c == 'a' || c == 'A') && p1.x > 0)
          move_paddle(&p1, LEFT);
        else if ((c == 'd' || c == 'D') && p1.x + PADDLE_WIDTH < cols)
          move_paddle(&p1, RIGHT);

        if (c == '\x1b') {
          char seq[2];
          if (read(STDIN_FILENO, &seq[0], 1) == 0)
            continue;
          if (read(STDIN_FILENO, &seq[1], 1) == 0)
            continue;

          if (seq[0] == '[') {
            if (seq[1] == 'D' && p2.x > 0)
              move_paddle(&p2, LEFT);
            else if (seq[1] == 'C' && p2.x + PADDLE_WIDTH < cols)
              move_paddle(&p2, RIGHT);
          }
        }
      }
    }

    usleep(5000);
  }

  disable_raw_mode();
  cursor(1);
  write(STDERR_FILENO, ESC "[?1049l", 8);

  return 0;
}
