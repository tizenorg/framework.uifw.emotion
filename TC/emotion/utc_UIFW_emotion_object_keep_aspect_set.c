#include <tet_api.h>
#include <Ecore.h>
#include <Ecore_Evas.h>
#include <Evas.h>
#include <Emotion.h>
#include <signal.h>
#include "common.h"

static void utc_emotion_object_keep_aspect_set_p(void);
static void utc_emotion_object_keep_aspect_set_n(void);
static void utc_emotion_object_keep_aspect_set_null(void);

struct tet_testlist tet_testlist[] = {
	{ utc_emotion_object_keep_aspect_set_p, POSITIVE_TC_IDX},
	{ utc_emotion_object_keep_aspect_set_n, NEGATIVE_TC_IDX},
	{ utc_emotion_object_keep_aspect_set_null, NULL_TC_IDX},
	{ NULL, 0 },
};

static void utc_emotion_object_keep_aspect_set_p(void)
{
    Evas_Object *object = init();
    emotion_object_keep_aspect_set(object, EMOTION_ASPECT_KEEP_BOTH);
    Emotion_Aspect res = emotion_object_keep_aspect_get(object);

	if (EMOTION_ASPECT_KEEP_BOTH != res) {
		tet_printf("[TET_FAIL]:: %s[%d] : Test has failed..", __FILE__, __LINE__);
		tet_result(TET_FAIL);
		return;
	}
	tet_printf("[TET_PASS]:: %s[%d] : Test has passed..", __FILE__, __LINE__);
	tet_result(TET_PASS);
}

static void utc_emotion_object_keep_aspect_set_n(void)
{
    Evas_Object *object = init();
    emotion_object_keep_aspect_set(object, EMOTION_ASPECT_KEEP_BOTH);
    emotion_object_border_set(object, 1, 1, 1, 1);
    Emotion_Aspect res = emotion_object_keep_aspect_get(object);

	if (EMOTION_ASPECT_CUSTOM != res) {
		tet_printf("[TET_FAIL]:: %s[%d] : Test has failed..", __FILE__, __LINE__);
		tet_result(TET_FAIL);
		return;
	}
	tet_printf("[TET_PASS]:: %s[%d] : Test has passed..", __FILE__, __LINE__);
	tet_result(TET_PASS);
}

static void utc_emotion_object_keep_aspect_set_null(void)
{
	signal(SIGSEGV, sigprocess);
	
    emotion_object_keep_aspect_set(NULL, EMOTION_ASPECT_KEEP_BOTH);

    tet_printf("[TET_PASS]:: %s[%d] : Test has passed..", __FILE__, __LINE__);
	tet_result(TET_PASS);
}
