#include <tet_api.h>
#include <Ecore.h>
#include <Ecore_Evas.h>
#include <Evas.h>
#include <Emotion.h>
#include <signal.h>
#include "common.h"

static void utc_emotion_object_play_set_p(void);
static void utc_emotion_object_play_set_n(void);
static void utc_emotion_object_play_set_null(void);

struct tet_testlist tet_testlist[] = {
	{ utc_emotion_object_play_set_p, POSITIVE_TC_IDX},
	{ utc_emotion_object_play_set_n, NEGATIVE_TC_IDX},
	{ utc_emotion_object_play_set_null, NULL_TC_IDX},
	{ NULL, 0 },
};

static void utc_emotion_object_play_set_p(void)
{
    Evas_Object *object = init();

    emotion_object_play_set(object, EINA_TRUE);

    ecore_main_loop_begin();

    Eina_Bool res = emotion_object_play_get(object);

	if (EINA_FALSE == res) {
		tet_printf("[TET_FAIL]:: %s[%d] : Test has failed..", __FILE__, __LINE__);
		tet_result(TET_FAIL);
		return;
	}
	tet_printf("[TET_PASS]:: %s[%d] : Test has passed..", __FILE__, __LINE__);
	tet_result(TET_PASS);
}

static void utc_emotion_object_play_set_n(void)
{
    Evas_Object *object = init();
    emotion_object_play_set(object, EINA_FALSE);
    Eina_Bool res = emotion_object_play_get(object);

	if (EINA_FALSE != res) {
		tet_printf("[TET_FAIL]:: %s[%d] : Test has failed..", __FILE__, __LINE__);
		tet_result(TET_FAIL);
		return;
	}
	tet_printf("[TET_PASS]:: %s[%d] : Test has passed..", __FILE__, __LINE__);
	tet_result(TET_PASS);
}

static void utc_emotion_object_play_set_null(void)
{
	signal(SIGSEGV, sigprocess);
	
    emotion_object_play_set(NULL, EINA_TRUE);
    emotion_object_play_get(NULL);

	tet_printf("[TET_PASS]:: %s[%d] : Test has passed..", __FILE__, __LINE__);
	tet_result(TET_PASS);
}
