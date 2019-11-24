#include <math.h>
#include <stdlib.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <time.h>

#include <X11/Xlib.h>
#include <X11/Xos.h>
#include <X11/Xutil.h>
#include <X11/keysym.h>

#include "vector.h"

#define FRAME_XSIZE 1000
#define FRAME_YSIZE 1000
#define PI 3.14159253

// structs
typedef struct {
  bool w;
  bool a;
  bool s;
  bool d;
  bool q;
  bool e;
  bool x;
  bool Left;
  bool Right;
  bool Up;
  bool Down;
  bool mouse_down;
  uint32_t mouse_x;
  uint32_t mouse_y;
  uint32_t previous_mouse_x;
  uint32_t previous_mouse_y;
  uint32_t x_size;
  uint32_t y_size;
} UserInput;

// Program control variables
bool terminate = false;
UserInput user_input = {0};

// here are our X variables
Display *dis;
int screen;
Window win;
GC gc;


void init_x() {
  // open connection to x server
  dis = XOpenDisplay((char *)0);
  screen = DefaultScreen(dis);
  // create the window
  win = XCreateSimpleWindow(dis, DefaultRootWindow(dis), 0, 0, FRAME_XSIZE,
                            FRAME_YSIZE, 0, 0, 0);
  XSelectInput(dis, win,
               StructureNotifyMask | ButtonPressMask | ButtonReleaseMask |
                   PointerMotionMask | KeyPressMask | KeyReleaseMask);
  gc = XCreateGC(dis, win, 0, 0);
  XSetBackground(dis, gc, 0);
  XSetForeground(dis, gc, 0);
  XClearWindow(dis, win);
  XMapRaised(dis, win);
}

void delete_x() {
  XFreeGC(dis, gc);
  XDestroyWindow(dis, win);
  XCloseDisplay(dis);
}

#define INPUTONKEY(key, boolean) \
  case XK_##key: {               \
    input->key = boolean;        \
    break;                       \
  }
void update_user_input(Display *display, UserInput *input) {
  // get the next event and stuff it into our event variable.
  // Note:  only events we set the mask for are detected!
  int32_t previous_mouse_x = input->mouse_x;
  int32_t previous_mouse_y = input->mouse_y;

  while (XPending(dis) > 0) {
    XEvent event;
    XNextEvent(dis, &event);
    switch (event.type) {
      case ConfigureNotify: {
        XConfigureEvent xce = event.xconfigure;
        input->x_size = xce.width;
        input->y_size = xce.height;
      } break;
      case KeyPress: {
        KeySym k = XLookupKeysym(&event.xkey, 0);
        switch (k) {
          INPUTONKEY(w, true)
          INPUTONKEY(a, true)
          INPUTONKEY(s, true)
          INPUTONKEY(d, true)
          INPUTONKEY(q, true)
          INPUTONKEY(e, true)
          INPUTONKEY(x, true)
          INPUTONKEY(Left, true)
          INPUTONKEY(Right, true)
          INPUTONKEY(Up, true)
          INPUTONKEY(Down, true)
          default: {
          } break;
        }
      } break;
      case KeyRelease: {
        KeySym k = XLookupKeysym(&event.xkey, 0);
        switch (k) {
          INPUTONKEY(w, false)
          INPUTONKEY(a, false)
          INPUTONKEY(s, false)
          INPUTONKEY(d, false)
          INPUTONKEY(q, false)
          INPUTONKEY(e, false)
          INPUTONKEY(x, false)
          INPUTONKEY(Left, false)
          INPUTONKEY(Right, false)
          INPUTONKEY(Up, false)
          INPUTONKEY(Down, false)
          default: {
          } break;
        }
      } break;
      case ButtonPress: {
        // mouse is down
        input->mouse_down = true;
      } break;
      case ButtonRelease: {
        // mouse is up
        input->mouse_down = false;
      } break;
      case MotionNotify: {
        // set mouses
        input->mouse_x = event.xmotion.x;
        input->mouse_y = event.xmotion.y;
      } break;
      default: {
      }
    }
  }
  input->previous_mouse_x = previous_mouse_x;
  input->previous_mouse_y = previous_mouse_y;
}
#undef INPUTONKEY

typedef struct {
  double x;
  double y;
  double dx;
  double dy;
} Entity;

typedef struct {
  Entity entity;
  int ticks;
} Bullet;

typedef struct {
  Entity entity;
  double direction;
} Player;


Vector bullets;

Player player;

int radianToAngle(double radians) {
  int degrees = radians*180/PI;
  return degrees*64;
}

void initialize() {
  initVector(&bullets);
  player.direction = 0.0;
  player.entity.x = 200.0;
  player.entity.y = 0.0;
  player.entity.dx = 0.0;
  player.entity.dy = -4.0;
}

// Where the force is coming from
Entity apply_gravity(Entity e) {
  double distance = hypot(e.x, e.y);
  double multiplier = 5000.0;
  e.dx -= multiplier*e.x/pow(distance, 3);
  e.dy -= multiplier*e.y/pow(distance, 3);
  return e;
}

Entity apply_movement(Entity e) {
  double boundary = FRAME_XSIZE/2 -1;
  double distance = hypot(e.x, e.y);
  if(distance > boundary+1) {
    double distance = hypot(e.x, e.y);
    e.x = boundary*e.x/distance;
    e.y = boundary*e.y/distance;
    e.dx = 0;
    e.dy = 0;
  } else {
    e.x += e.dx;
    e.y += e.dy;
  }
  return e;
}

void loop() {
  update_user_input(dis, &user_input);


  double multiplier = 0.05;
  if(user_input.Up) {
    player.entity.dx += multiplier*cos(player.direction);
    player.entity.dy += multiplier*sin(player.direction);
  }
  if(user_input.Down) {
    player.entity.dx -= multiplier*cos(player.direction);
    player.entity.dy -= multiplier*sin(player.direction);
  }
  if(user_input.Left) {
    player.direction -= 0.1;
  }
  if(user_input.Right) {
    player.direction += 0.1;
  }
  if(user_input.q) {
    for(int i = 0; i < 1000; i++) {
      double bullet_velocity = 5.0 + (rand() % 1000 - 500)/1000.0;
      double bullet_directional_offset = (rand() % 1000 - 500)/100000.0;
      Bullet new_bullet;
      new_bullet.ticks = 0;
      new_bullet.entity.x = player.entity.x;
      new_bullet.entity.y = player.entity.y;
      new_bullet.entity.dx = player.entity.dx
        + bullet_velocity*cos(player.direction+bullet_directional_offset);
      new_bullet.entity.dy = player.entity.dy
        + bullet_velocity*sin(player.direction+bullet_directional_offset);
      memcpy(pushVector(&bullets, sizeof(Bullet)), &new_bullet, sizeof(Bullet));
    }
  }

  // Move player
  player.entity = apply_gravity(player.entity);
  player.entity = apply_movement(player.entity);

  // Move bullets
  for(int i = (int)(lengthVector(&bullets)/sizeof(Bullet)) -1; i >= 0; i--) {
    Bullet* bullet_ptr = getVector(&bullets, i*sizeof(Bullet));
    double distance = hypot(bullet_ptr->entity.x, bullet_ptr->entity.y);
    if(bullet_ptr->ticks > 500 || distance < 20) {
      removeVector(&bullets, i*sizeof(Bullet), sizeof(Bullet));
      continue;
    }
    bullet_ptr->ticks++;
    bullet_ptr->entity = apply_gravity(bullet_ptr->entity);
    bullet_ptr->entity = apply_movement(bullet_ptr->entity);
  }

  struct timespec ts = { .tv_sec = 0, .tv_nsec = 40*1000*1000  };
  nanosleep(&ts, NULL);

  XSetForeground(dis, gc, 0xFFFFFF);
  XFillRectangle(dis, win, gc, 0, 0, FRAME_XSIZE, FRAME_YSIZE);
  XSetForeground(dis, gc, 0x000000);
  XFillArc(dis,win,gc, 0, 0, FRAME_XSIZE, FRAME_YSIZE, 0, 360*64);

  // Drawing player
  {
    int32_t player_width = 5;
    int32_t player_length = 10;
    XSetForeground(dis, gc, 0x0000FF);
    XPoint points[3];

    points[0].x = player.entity.x + player_length*cos(player.direction) + FRAME_XSIZE/2;
    points[0].y = player.entity.y + player_length*sin(player.direction) + FRAME_YSIZE/2;

    points[1].x = player.entity.x + -player_length*cos(player.direction) + player_width*cos(player.direction + PI/4) + FRAME_XSIZE/2;
    points[1].y = player.entity.y + -player_length*sin(player.direction) + player_width*sin(player.direction + PI/4) + FRAME_YSIZE/2;

    points[2].x = player.entity.x + -player_length*cos(player.direction) + player_width*cos(player.direction - PI/4) + FRAME_XSIZE/2;
    points[2].y = player.entity.y + -player_length*sin(player.direction) + player_width*sin(player.direction - PI/4) + FRAME_YSIZE/2;

    XFillPolygon(dis, win, gc, points, 3, Convex, CoordModeOrigin);
  }

  // Draw black hole
  {
    XSetForeground(dis, gc, 0xFFFFFF);
    XFillArc(dis,win,gc, FRAME_XSIZE/2 -10, FRAME_YSIZE/2 -10, 20, 20, 0, 360*64);
  }

  // Draw bullets
  {
    XSetForeground(dis, gc, 0xFF0000);
    for(int i = 0; i < lengthVector(&bullets)/sizeof(Bullet); i++) {
      Bullet bullet = *VEC_GET(&bullets, i, Bullet);
      int x1 = bullet.entity.x + FRAME_XSIZE/2;
      int y1 = bullet.entity.y + FRAME_YSIZE/2;
      int x2 = bullet.entity.x + bullet.entity.dx + FRAME_XSIZE/2;
      int y2 = bullet.entity.y + bullet.entity.dy + FRAME_YSIZE/2;
      XDrawLine(dis, win, gc, x1, y1, x2, y2);
    }
  }

  if (user_input.x) {
    terminate = true;
  }
}

void finalize() {
  freeVector(&bullets);
}

int main(int argc, char** argv) {
  init_x();
  initialize();
  while (!terminate) {
    loop();
  }
  finalize();
  delete_x();
  return 0;
}
