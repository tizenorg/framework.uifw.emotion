#include <tet_api.h>
#include <Ecore.h>
#include <Ecore_Evas.h>
#include <Evas.h>
#include <Emotion.h>
#include <signal.h>
#include "common.h"

static void utc_emotion_object_size_get_p(void);
static void utc_emotion_object_size_get_null(void);

struct tet_testlist tet_testlist[] = {
	{ utc_emotion_object_size_get_p, POSITIVE_TC_IDX},
	{ utc_emotion_object_size_get_null, NULL_TC_IDX},
	{ NULL, 0 },
};

static void utc_emotion_object_size_get_p(void)
{
    int w = 0, h = 0;

    Evas_Object *object = init();
    ecore_main_loop_begin();
    emotion_object_size_get(object, &w, &h);

	if (436 != w || 344 != h) {
		tet_printf("[TET_FAIL]:: %s[%d] : Test has failed..", __FILE__, __LINE__);
		tet_result(TET_FAIL);
		return;
	}
	tet_printf("[TET_PASS]:: %s[%d] : Test has passed..", __FILE__, __LINE__);
	tet_result(TET_PASS);
}

static void utc_emotion_object_size_get_null(void)
{
    int w = 0, h = 0;

	signal(SIGSEGV, sigprocess);

    emotion_object_size_get(NULL, &w, &h);

	tet_printf("[TET_PASS]:: %s[%d] : Test has passed..", __FILE__, __LINE__);
	tet_result(TET_PASS);
}

