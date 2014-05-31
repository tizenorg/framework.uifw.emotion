#include <tet_api.h>
#include <Ecore.h>
#include <Ecore_Evas.h>
#include <Evas.h>
#include <Emotion.h>
#include <signal.h>
#include "common.h"

static void utc_emotion_object_bg_color_set_p(void);
static void utc_emotion_object_bg_color_set_n(void);
static void utc_emotion_object_bg_color_set_null(void);

struct tet_testlist tet_testlist[] = {
	{ utc_emotion_object_bg_color_set_p, POSITIVE_TC_IDX},
	{ utc_emotion_object_bg_color_set_n, NEGATIVE_TC_IDX},
	{ utc_emotion_object_bg_color_set_null, NULL_TC_IDX},
	{ NULL, 0 },
};

static void utc_emotion_object_bg_color_set_p(void)
{
    int r = 0, g = 0, b = 0, a = 0;

    Evas_Object *object = init();
    ecore_main_loop_begin();
    emotion_object_bg_color_set(object, 5, 4, 3, 2);
    emotion_object_bg_color_get(object, &r, &g, &b, &a);

	if (5 != r || 4 != g || 3 != b || 2 != a) {
		tet_printf("[TET_FAIL]:: %s[%d] : Test has failed..", __FILE__, __LINE__);
        tet_printf("Expected value(rgba): (5,4,3,2), result value(rgba): (%d,%d,%d,%d)", r, g, b, a);
		tet_result(TET_FAIL);
		return;
	}
	tet_printf("[TET_PASS]:: %s[%d] : Test has passed..", __FILE__, __LINE__);
	tet_result(TET_PASS);
}

static void utc_emotion_object_bg_color_set_n(void)
{
    int r = 0, g = 0, b = 0, a = 0;

    Evas_Object *object = init();
    emotion_object_bg_color_set(object, -5, -4, -3, -2);
    emotion_object_bg_color_get(object, &r, &g, &b, &a);

	if (0 != r || 0 != g || 0 != b || 0 != a) {
		tet_printf("[TET_FAIL]:: %s[%d] : Test has failed..", __FILE__, __LINE__);
		tet_result(TET_FAIL);
		return;
	}
	tet_printf("[TET_PASS]:: %s[%d] : Test has passed..", __FILE__, __LINE__);
	tet_result(TET_PASS);
}

static void utc_emotion_object_bg_color_set_null(void)
{
	signal(SIGSEGV, sigprocess);
	
    emotion_object_bg_color_set(NULL, -5, -4, -3, -2);

	tet_printf("[TET_PASS]:: %s[%d] : Test has passed..", __FILE__, __LINE__);
	tet_result(TET_PASS);
}

