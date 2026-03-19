#include "doomgeneric.h"

#include <emscripten/emscripten.h>

#include <stdint.h>

enum {
    DGW_KEY_QUEUE_SIZE = 64,
};

static uint16_t s_key_queue[DGW_KEY_QUEUE_SIZE];
static uint32_t s_key_read = 0;
static uint32_t s_key_write = 0;
static int s_booted = 0;

static void push_key_event(int pressed, unsigned char doom_key) {
    const uint32_t next = (s_key_write + 1U) % DGW_KEY_QUEUE_SIZE;
    if (next == s_key_read) {
        s_key_read = (s_key_read + 1U) % DGW_KEY_QUEUE_SIZE;
    }

    s_key_queue[s_key_write] = ((pressed ? 1U : 0U) << 8) | doom_key;
    s_key_write = next;
}

EMSCRIPTEN_KEEPALIVE
int DGW_Boot(void) {
    static char arg0[] = "doom";
    static char arg1[] = "-iwad";
    static char arg2[] = "game.wad";
    static char *argv[] = {arg0, arg1, arg2, NULL};

    if (s_booted) {
        return 1;
    }

    doomgeneric_Create(3, argv);
    s_booted = 1;
    return 1;
}

EMSCRIPTEN_KEEPALIVE
void DGW_Tick(void) {
    if (s_booted) {
        doomgeneric_Tick();
    }
}

EMSCRIPTEN_KEEPALIVE
uintptr_t DGW_GetScreenBuffer(void) {
    return (uintptr_t) DG_ScreenBuffer;
}

EMSCRIPTEN_KEEPALIVE
int DGW_GetScreenWidth(void) {
    return DOOMGENERIC_RESX;
}

EMSCRIPTEN_KEEPALIVE
int DGW_GetScreenHeight(void) {
    return DOOMGENERIC_RESY;
}

EMSCRIPTEN_KEEPALIVE
void DGW_PushKeyEvent(int pressed, int doom_key) {
    if (doom_key < 0 || doom_key > 0xff) {
        return;
    }

    push_key_event(pressed, (unsigned char) doom_key);
}

void DG_Init(void) {
}

void DG_DrawFrame(void) {
}

void DG_SleepMs(uint32_t ms) {
    (void) ms;
}

uint32_t DG_GetTicksMs(void) {
    return (uint32_t) emscripten_get_now();
}

int DG_GetKey(int *pressed, unsigned char *doom_key) {
    uint16_t key_data;

    if (s_key_read == s_key_write) {
        return 0;
    }

    key_data = s_key_queue[s_key_read];
    s_key_read = (s_key_read + 1U) % DGW_KEY_QUEUE_SIZE;

    *pressed = (int) (key_data >> 8);
    *doom_key = (unsigned char) (key_data & 0xff);
    return 1;
}

void DG_SetWindowTitle(const char *title) {
    (void) title;
}

int main(void) {
    return 0;
}
