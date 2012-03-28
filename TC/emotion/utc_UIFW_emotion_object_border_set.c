#include <tet_api.h>
#include <Ecore.h>
#include <Ecore_Evas.h>
#include <Evas.h>
#include <Emotion.h>
#include <signal.h>
#include "common.h"

static void utc_emotion_object_border_set_p(void);
static void utc_emotion_object_border_set_n(void);
static void utc_emotion_object_border_set_null(void);

struct tet_testlist tet_testlist[] = {
	{ utc_emotion_object_border_set_p, POSITIVE_TC_IDX},
	{ utc_emotion_object_border_set_n, NEGATIVE_TC_IDX},
	{ utc_emotion_object_border_set_null, NULL_TC_IDX},
	{ NULL, 0 },
};

static void utc_emotion_object_border_set_p(void)
{
    int l = 0, r = 0, t = 0, b = 0;

    Evas_Object *object = init();
    emotion_object_border_set(object, 5, 4, 3, 2);
    emotion_object_border_get(object, &l, &r, &t, &b);

	if (5 != l || 4 != r || 3 != t || 2 != b) {
		tet_printf("[TET_FAIL]:: %s[%d] : Test has failed..", __FILE__, __LINE__);
		tet_result(TET_FAIL);
		return;
	}
	tet_printf("[TET_PASS]:: %s[%d] : Test has passed..", __FILE__, __LINE__);
	tet_result(TET_PASS);
}

static void utc_emotion_object_border_set_n(void)
{
    int l = 0, r = 0, t = 0, b = 0;

    Evas_Object *object = init();
    emotion_object_border_set(object, -5, -4, -3, -2);
    emotion_object_border_get(object, &l, &r, &t, &b);

	if (-5 != l || -4 != r || -3 != t || -2 != b) {
		tet_printf("[TET_FAIL]:: %s[%d] : Test has failed..", __FILE__, __LINE__);
		tet_result(TET_FAIL);
		return;
	}
	tet_printf("[TET_PASS]:: %s[%d] : Test has passed..", __FILE__, __LINE__);
	tet_result(TET_PASS);
}

static void utc_emotion_object_border_set_null(void)
{
	signal(SIGSEGV, sigprocess);

    emotion_object_border_set(NULL, -5, -4, -3, -2);

	tet_printf("[TET_PASS]:: %s[%d] : Test has passed..", __FILE__, __LINE__);
	tet_result(TET_PASS);
}

