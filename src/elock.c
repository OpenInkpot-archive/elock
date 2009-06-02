/*
 * Copyright (C) 2009 Alexander Kerner <lunohod@openinkpot.org>
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
#include <stdio.h>
#include <stdarg.h>
#include <stdlib.h>
#include <string.h>
#include <libintl.h>
#include <time.h>

#include <Ecore.h>
#include <Ecore_X.h>
#include <Ecore_Con.h>
#include <Ecore_Evas.h>
#include <Evas.h>
#include <Edje.h>

#ifndef DATADIR
#define DATADIR "."
#endif

#define LOCK "Lock"
#define UNLOCK "Unlock"

#define NOTIME "--:--"

Ecore_Evas *main_win;

void exit_all(void* param) { ecore_main_loop_quit(); }

static void die(const char* fmt, ...)
{
	va_list ap;
	va_start(ap, fmt);
	vfprintf(stderr, fmt, ap);
	va_end(ap);
	exit(EXIT_FAILURE);
}

static void update_time()
{
	Evas_Object *o = evas_object_name_find(ecore_evas_get(main_win), "edje");
	char *tstr = NULL;
	time_t tim = time(NULL);
	struct tm *t = localtime(&tim);

	if(t->tm_year >= 109) {
		asprintf(&tstr, "%2d:%02d", t->tm_hour, t->tm_min);
		edje_object_part_text_set(o, "elock/time", tstr);
		free(tstr);
	} else
		edje_object_part_text_set(o, "elock/time", NOTIME);
}

static void update_battery()
{
    char b[10];
    int charge;
    int x;
    FILE *f_cf, *f_cn;

    if((f_cn = fopen("/sys/class/power_supply/n516-battery/charge_now", "r")) != NULL)
		f_cf = fopen("/sys/class/power_supply/n516-battery/charge_full_design", "r");
	else {
		f_cn = fopen("/sys/class/power_supply/lbookv3_battery/charge_now", "r");
		f_cf = fopen("/sys/class/power_supply/lbookv3_battery/charge_full_design", "r");
	}

    if((f_cn != NULL) && (f_cf != NULL)) {
        fgets(b, 10, f_cn);
        charge = atoi(b);
        fgets(b, 10, f_cf);
        x = atoi(b);
        if(x > 0)
            charge = charge * 100 / atoi(b);
    } else
        charge = 0;

    if(f_cn != NULL)
        fclose(f_cn);
    if(f_cf != NULL)
        fclose(f_cf);

	Evas_Object *o = evas_object_name_find(ecore_evas_get(main_win), "edje");
	if(charge < 5)
		edje_object_signal_emit(o, "set_batt_empty", "");
	else if(charge < 25)
		edje_object_signal_emit(o, "set_batt_1/4", "");
	else if(charge < 50)
		edje_object_signal_emit(o, "set_batt_2/4", "");
	else if(charge < 75)
		edje_object_signal_emit(o, "set_batt_3/4", "");
	else
		edje_object_signal_emit(o, "set_batt_full", "");
}

typedef struct
{
    char* msg;
    int size;
} client_data_t;

static int _client_add(void* param, int ev_type, void* ev)
{
    Ecore_Con_Event_Client_Add* e = ev;
    client_data_t* msg = malloc(sizeof(client_data_t));
    msg->msg = strdup("");
    msg->size = 0;
    ecore_con_client_data_set(e->client, msg);
    return 0;
}

static int _client_del(void* param, int ev_type, void* ev)
{
    Ecore_Con_Event_Client_Del* e = ev;
    client_data_t* msg = ecore_con_client_data_get(e->client);

    /* Handle */
	if(strlen(LOCK) == msg->size && !strncmp(LOCK, msg->msg, msg->size)) {
		update_time();
		update_battery();
		ecore_evas_show(main_win);
	} else if(strlen(UNLOCK) == msg->size && !strncmp(UNLOCK, msg->msg, msg->size))
		ecore_evas_hide(main_win);

    //printf(": %.*s(%d)\n", msg->size, msg->msg, msg->size);


    free(msg->msg);
    free(msg);
    return 0;
}

static int _client_data(void* param, int ev_type, void* ev)
{
    Ecore_Con_Event_Client_Data* e = ev;
    client_data_t* msg = ecore_con_client_data_get(e->client);
    msg->msg = realloc(msg->msg, msg->size + e->size);
    memcpy(msg->msg + msg->size, e->data, e->size);
    msg->size += e->size;
    return 0;
}

int main(int argc, char **argv)
{
	if(!evas_init())
		die("Unable to initialize Evas\n");
	if(!ecore_init())
		die("Unable to initialize Ecore\n");
	if(!ecore_con_init())
		die("Unable to initialize Ecore_Con\n");
	if(!ecore_evas_init())
		die("Unable to initialize Ecore_Evas\n");
	if(!edje_init())
		die("Unable to initialize Edje\n");

	setlocale(LC_ALL, "");
	textdomain("elock");

	ecore_con_server_add(ECORE_CON_LOCAL_USER, "elock", 0, NULL);

	ecore_event_handler_add(ECORE_CON_EVENT_CLIENT_ADD, _client_add, NULL);
	ecore_event_handler_add(ECORE_CON_EVENT_CLIENT_DATA, _client_data, NULL);
	ecore_event_handler_add(ECORE_CON_EVENT_CLIENT_DEL, _client_del, NULL);

	ecore_x_io_error_handler_set(exit_all, NULL);

	//Ecore_Evas *
	main_win = ecore_evas_software_x11_new(0, 0, 0, 0, 600, 800);
	ecore_evas_borderless_set(main_win, 0);
	ecore_evas_shaped_set(main_win, 0);
	ecore_evas_title_set(main_win, "elock");
	ecore_evas_name_class_set(main_win, "elock", "elock");

	Evas *main_canvas = ecore_evas_get(main_win);

	Evas_Object *edje = edje_object_add(main_canvas);
	evas_object_name_set(edje, "edje");
	edje_object_file_set(edje, DATADIR "/elock/themes/elock.edj", "elock");
	evas_object_move(edje, 0, 0);
	evas_object_resize(edje, 600, 800);
	evas_object_show(edje);

	update_time();
	update_battery();

	char *t1 = gettext("Press and hold \"OK\" for 3-4 seconds to unlock the device");
	edje_object_part_text_set(edje, "elock/label", t1);

//	ecore_evas_show(main_win);
	ecore_main_loop_begin();

	edje_shutdown();
	ecore_evas_shutdown();
	ecore_con_shutdown();
	ecore_shutdown();
	evas_shutdown();

	return 0;
}
