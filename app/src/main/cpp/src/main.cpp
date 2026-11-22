// Тест для «APK · C++»: заливает экран цветом, по касанию меняет цвет.
#include <android_native_app_glue.h>
#include <android/native_window.h>
#include <android/input.h>
#include <android/log.h>
#include <cstdint>

#define LOG(...) __android_log_print(ANDROID_LOG_INFO, "CppTest", __VA_ARGS__)

struct State { bool ready = false; int idx = 0; int taps = 0; };
static const uint32_t RGB[] = {0x2F6DF6, 0x1A9D5A, 0xD6403B, 0xF2B01E, 0x8A4FFF};

// формат буфера RGBA_8888: байты R,G,B,A -> как uint32 (little-endian) A<<24|B<<16|G<<8|R
static uint32_t pix(uint32_t rgb) {
    return 0xFF000000u | ((rgb & 0xFF) << 16) | (rgb & 0xFF00) | ((rgb >> 16) & 0xFF);
}

static void draw(android_app* app) {
    auto* s = (State*)app->userData;
    if (!s->ready || !app->window) return;
    ANativeWindow_Buffer b;
    if (ANativeWindow_lock(app->window, &b, nullptr) < 0) return;
    auto* px = (uint32_t*)b.bits;
    uint32_t c = pix(RGB[s->idx % 5]);
    for (int y = 0; y < b.height; y++)
        for (int x = 0; x < b.width; x++)
            px[y * b.stride + x] = c;
    ANativeWindow_unlockAndPost(app->window);
}

static int onInput(android_app* app, AInputEvent* e) {
    if (AInputEvent_getType(e) == AINPUT_EVENT_TYPE_MOTION &&
        (AMotionEvent_getAction(e) & AMOTION_EVENT_ACTION_MASK) == AMOTION_EVENT_ACTION_DOWN) {
        auto* s = (State*)app->userData;
        s->idx++; s->taps++;
        LOG("tap #%d", s->taps);
        return 1;
    }
    return 0;
}

static void onCmd(android_app* app, int32_t cmd) {
    auto* s = (State*)app->userData;
    if (cmd == APP_CMD_INIT_WINDOW) {
        ANativeWindow_setBuffersGeometry(app->window, 0, 0, WINDOW_FORMAT_RGBA_8888);
        s->ready = true;
        LOG("window ready");
    } else if (cmd == APP_CMD_TERM_WINDOW) {
        s->ready = false;
    }
}

void android_main(android_app* app) {
    State state;
    app->userData = &state;
    app->onAppCmd = onCmd;
    app->onInputEvent = onInput;
    while (true) {
        int ev; android_poll_source* src;
        int r = ALooper_pollOnce(16, nullptr, &ev, (void**)&src);
        if (r >= 0 && src) src->process(app, src);
        if (app->destroyRequested) return;
        draw(app);
    }
}
