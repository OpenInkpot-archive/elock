#include <stdio.h>
#include <string.h>
#include <libintl.h>

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

Ecore_Evas *main_win;

void exit_all(void* param) { ecore_main_loop_quit(); }

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
	if(strlen(LOCK) == msg->size && !strncmp(LOCK, msg->msg, msg->size))
		ecore_evas_show(main_win);
	else if(strlen(UNLOCK) == msg->size && !strncmp(UNLOCK, msg->msg, msg->size))
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
	ecore_init();
	ecore_con_init();
	ecore_evas_init();
	edje_init();

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

	char *t1 = gettext("Press and hold \"OK\" for 3-4 seconds to unlock the device");
	edje_object_part_text_set(edje, "elock/label", t1);

	edje_object_signal_emit(edje, "set_batt_full", "");

	ecore_main_loop_begin();

	edje_shutdown();
	ecore_evas_shutdown();
	ecore_con_shutdown();
	ecore_shutdown();

	return 0;
}
