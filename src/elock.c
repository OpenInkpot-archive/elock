/*
 * Copyright (C) 2009 Alexander Kerner <lunohod@openinkpot.org>
 * Copyright © 2009,2010 Mikhail Gusarov <dottedmag@dottedmag.net>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA
 * 02110-1301, USA.
 */

#define _GNU_SOURCE
#include <stdbool.h>
#include <stdio.h>
#include <stdarg.h>
#include <stdlib.h>
#include <string.h>
#include <libintl.h>
#include <time.h>
#include <unistd.h>
#include <err.h>
#include <locale.h>

#include <Ecore.h>
#include <Ecore_X.h>
#include <Ecore_Con.h>
#include <Ecore_Evas.h>
#include <Evas.h>
#include <Edje.h>

#include <libkeys.h>
#include <libeoi_themes.h>
#include <libeoi_dialog.h>

#define LOCK "Lock"
#define UNLOCK "Unlock"

typedef struct {
    keys_t *keys;
    Ecore_Evas *main_win;
} elock_state_t;

static void
exit_all(void *param)
{
    ecore_main_loop_quit();
}

static void
do_lock(elock_state_t *state)
{
    ecore_evas_show(state->main_win);
}

static void
do_unlock(elock_state_t *state)
{
    ecore_evas_hide(state->main_win);
}

typedef struct {
    char *msg;
    int size;
} client_data_t;

static Eina_Bool
_client_add(void *param, int ev_type, void *ev)
{
    Ecore_Con_Event_Client_Add *e = ev;
    client_data_t *msg = malloc(sizeof(client_data_t));
    msg->msg = strdup("");
    msg->size = 0;
    ecore_con_client_data_set(e->client, msg);
    return 0;
}

static Eina_Bool
_client_del(void *param, int ev_type, void *ev)
{
    Ecore_Con_Event_Client_Del *e = ev;
    client_data_t *msg = ecore_con_client_data_get(e->client);

    /* Handle */
    if (strlen(LOCK) == msg->size && !strncmp(LOCK, msg->msg, msg->size))
        do_lock((elock_state_t *)param);
    else if (strlen(UNLOCK) == msg->size && !strncmp(UNLOCK, msg->msg, msg->size))
        do_unlock((elock_state_t *)param);

    free(msg->msg);
    free(msg);
    return 0;
}

static Eina_Bool
_client_data(void *param, int ev_type, void *ev)
{
    Ecore_Con_Event_Client_Data *e = ev;
    client_data_t *msg = ecore_con_client_data_get(e->client);
    msg->msg = realloc(msg->msg, msg->size + e->size);
    memcpy(msg->msg + msg->size, e->data, e->size);
    msg->size += e->size;
    return 0;
}

static void
main_win_key_handler(void *data, Evas *evas, Evas_Object *obj, void *event_info)
{
    elock_state_t *state = data;

    const char *action = keys_lookup_by_event(state->keys, "default",
                                              (Evas_Event_Key_Up *)event_info);

    if (action && !strcmp(action, "Unlock"))
        do_unlock(state);
}

static Eina_Bool
main_win_show_handler(void *param, int ev_type, void *ev)
{
    elock_state_t *state = param;
    Ecore_X_Window win = ecore_evas_software_x11_window_get(state->main_win);
    if (!ecore_x_keyboard_grab(win))
        errx(1, "Unable to grab keyboard");
    return 0;
}

static void
main_win_resize_handler(Ecore_Evas *main_win)
{
    int w, h;
    Evas *canvas = ecore_evas_get(main_win);
    evas_output_size_get(canvas, &w, &h);

    Evas_Object *dlg = evas_object_name_find(canvas, "dlg");
    evas_object_resize(dlg, w, h);
}

int
main(int argc, char **argv)
{
    if (!ecore_con_init())
        errx(1, "Unable to initialize Ecore_Con");
    if (!edje_init())
        errx(1, "Unable to initialize Edje");
    if (!ecore_x_init(NULL))
        errx(1, "Unable to initialize Ecore_X, maybe missing DISPLAY");
    if (!ecore_evas_init())
        errx(1, "Unable to initialize Ecore_Evas");

    bool hardware_lock = argc > 1 && !strcmp(argv[1], "--hardware-lock");

    elock_state_t state;

    setlocale(LC_ALL, "");
    textdomain("elock");
    state.keys = keys_alloc("elock");

    ecore_x_io_error_handler_set(exit_all, NULL);

    state.main_win = ecore_evas_software_x11_new(0, 0, 0, 0, 600, 800);
    ecore_evas_borderless_set(state.main_win, 0);
    ecore_evas_shaped_set(state.main_win, 0);
    ecore_evas_title_set(state.main_win, "elock");
    ecore_evas_name_class_set(state.main_win, "elock", "elock");

    ecore_evas_callback_resize_set(state.main_win, main_win_resize_handler);

    ecore_con_server_add(ECORE_CON_LOCAL_SYSTEM, "elock", 0, NULL);
    ecore_event_handler_add(ECORE_CON_EVENT_CLIENT_ADD, _client_add, NULL);
    ecore_event_handler_add(ECORE_CON_EVENT_CLIENT_DATA, _client_data, NULL);
    ecore_event_handler_add(ECORE_CON_EVENT_CLIENT_DEL, _client_del, &state);

    Evas *main_canvas = ecore_evas_get(state.main_win);

    Evas_Object *edje = eoi_create_themed_edje(main_canvas, "elock", "elock");

    Evas_Object *dlg = eoi_dialog_create("dlg", edje);
    eoi_dialog_title_set(dlg, gettext("Keyboard Lock"));
    evas_object_move(dlg, 0, 0);
    evas_object_resize(dlg, 600, 800);
    evas_object_focus_set(dlg, true);

    if (!hardware_lock) {
        evas_object_event_callback_add(dlg, EVAS_CALLBACK_KEY_UP,
                                       &main_win_key_handler, &state);
        ecore_event_handler_add(ECORE_X_EVENT_WINDOW_SHOW,
                                &main_win_show_handler, &state);

        edje_object_part_text_set(edje, "text",
                                  gettext("Press Power button to unlock the device"));
    } else {
        edje_object_part_text_set(edje, "text",
                                  gettext("Press and hold \"OK\" for 3-4 seconds to unlock the device"));
    }

    Evas_Object *icon = eoi_create_themed_edje(main_canvas, "elock", "icon");
    edje_object_part_swallow(dlg, "icon", icon);

    evas_object_show(dlg);

    ecore_main_loop_begin();

    keys_free(state.keys);

    ecore_evas_shutdown();
    ecore_x_shutdown();
    edje_shutdown();
    ecore_con_shutdown();

    return 0;
}
