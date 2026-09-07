#include "common.h"

#include <assert.h>
#include <SDL3/SDL.h>

#include "controller.h"
#include "keyboard_sdk.h"
#include "screen.h"
#include "uhid/keyboard_uhid.h"
#include "util/log.h"

static void
check_text_input(struct sc_key_processor *kp, bool expected) {
    struct sc_screen screen = {0};
    struct sc_screen_params params = {
        .video = true, // Keep the window hidden; no device or video needed
        .kp = kp,
        .window_title = "scrcpy text input test",
        .window_x = SC_WINDOW_POSITION_UNDEFINED,
        .window_y = SC_WINDOW_POSITION_UNDEFINED,
    };
    assert(sc_screen_init(&screen, &params));
    bool active = SDL_TextInputActive(screen.window);
    sc_screen_destroy(&screen);
    assert(active == expected);
}

static void
on_controller_ended(struct sc_controller *controller, bool error,
                    void *userdata) {
    (void) controller;
    (void) error;
    (void) userdata;
    assert(!"The test must not start the controller thread");
}

int
main(int argc, char *argv[]) {
    (void) argc;
    (void) argv;

    assert(SDL_SetHintWithPriority(SDL_HINT_VIDEO_DRIVER, "dummy",
                                   SDL_HINT_OVERRIDE));
    assert(SDL_SetHintWithPriority(SDL_HINT_RENDER_DRIVER, "software",
                                   SDL_HINT_OVERRIDE));
    assert(SDL_Init(SDL_INIT_VIDEO));
    // The dummy video driver cannot set window icons.
    sc_set_log_level(SC_LOG_LEVEL_ERROR);

    // SDK input still needs text events for punctuation and --prefer-text.
    struct sc_keyboard_sdk sdk;
    sc_keyboard_sdk_init(&sdk, NULL, SC_KEY_INJECT_MODE_MIXED, true);
    check_text_input(&sdk.key_processor, true);
    sc_keyboard_sdk_init(&sdk, NULL, SC_KEY_INJECT_MODE_TEXT, true);
    check_text_input(&sdk.key_processor, true);

    static const struct sc_controller_callbacks cbs = {
        .on_ended = on_controller_ended,
    };
    struct sc_controller controller;
    assert(sc_controller_init(&controller, SC_SOCKET_NONE, &cbs, NULL));
    struct sc_keyboard_uhid uhid;
    assert(sc_keyboard_uhid_init(&uhid, &controller));

    // Enabling host text input here lets an IME consume HID key events.
    check_text_input(&uhid.key_processor, false);
    check_text_input(NULL, false); // Keyboard disabled, e.g. --no-control

    sc_controller_destroy(&controller);
    SDL_Quit();
    return 0;
}
