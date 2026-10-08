#ifndef MENU_H
#define MENU_H
#include <SDL2/SDL.h>

typedef enum { MENU_NONE = 0, MENU_PLAY, MENU_EXIT } MenuAction;

void       menu_init(int w, int h);
/* a = Android MotionEvent action (0 down, 1 up, 2 move, 3 cancel). returns an action on tap-release */
MenuAction menu_touch(int a, int x, int y);
void       menu_update(float dt);
void       menu_draw(SDL_Renderer *r);

#endif
