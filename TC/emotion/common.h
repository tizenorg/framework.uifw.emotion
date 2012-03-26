#ifndef _COMMON_H_
#define _COMMON_H_

enum {
	POSITIVE_TC_IDX = 0x01,
	NEGATIVE_TC_IDX,
	NULL_TC_IDX,
};

static void startup(void);
static void cleanup(void);

void (*tet_startup)(void) = startup;
void (*tet_cleanup)(void) = cleanup;

void sigprocess(int sig)
{
	signal(SIGSEGV, SIG_DFL);
	tet_printf("[TET_SEGFAULT]:: segmentation fault in file: %s", __FILE__);
	tet_result(TET_FAIL);
	raise(sig);
}

static Ecore_Evas *ecore_evas;

static void startup(void)
{
	tet_infoline("[[ TET_MSG ]]:: ============ Startup ============ ");
	ecore_evas_init();
    ecore_evas = ecore_evas_buffer_new(100, 45);
}

static void cleanup(void)
{
	tet_infoline("[[ TET_MSG ]]:: ============ Cleanup ============ ");
    ecore_evas_shutdown();
}

static Eina_Bool
_exit_func(void *data)
{
    printf("\n\nExiting main loop\n");
    ecore_main_loop_quit();
    return EINA_FALSE;
}

static Evas_Object *init(void)
{
	signal(SIGSEGV, sigprocess);
	   
    ecore_timer_add(1.0, _exit_func, NULL);

    Evas *evas = ecore_evas_get(ecore_evas);
    Evas_Object *object = emotion_object_add(evas);
    emotion_object_init(object, NULL/*"gstreamer"*/);
    emotion_object_file_set(object, "test.avi");

    evas_object_move(object, 0, 0);
    evas_object_resize(object, 100, 100);
    evas_object_show(object);

    return object;
}

#endif
