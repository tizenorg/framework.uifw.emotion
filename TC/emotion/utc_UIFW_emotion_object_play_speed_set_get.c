#include <tet_api.h>
#include <Ecore.h>
#include <Ecore_Evas.h>
#include <Evas.h>
#include <Emotion.h>
#include <signal.h>
#include "common.h"

static void utc_emotion_object_play_speed_set_p(void);
static void utc_emotion_object_play_speed_set_null(void);

struct tet_testlist tet_testlist[] = {
	{ utc_emotion_object_play_speed_set_p, POSITIVE_TC_IDX},
	{ utc_emotion_object_play_speed_set_null, NULL_TC_IDX},
	{ NULL, 0 },
};

static void utc_emotion_object_play_speed_set_p(void)
{
    Evas_Object *object = init();
    ecore_main_loop_begin();
    emotion_object_play_speed_set(object, 5.0);
    double play_speed = emotion_object_play_speed_get(object);

	if (5.0 != play_speed) {
		tet_printf("[TET_FAIL]:: %s[%d] : Test has failed..", __FILE__, __LINE__);
        tet_printf("Expected value: 5.0, return value: %f", play_speed);
		tet_result(TET_FAIL);
		return;
	}
	tet_printf("[TET_PASS]:: %s[%d] : Test has passed..", __FILE__, __LINE__);
	tet_result(TET_PASS);
}

static void utc_emotion_object_play_speed_set_null(void)
{
	signal(SIGSEGV, sigprocess);
	
    emotion_object_play_speed_set(NULL, 5.0);
    emotion_object_play_speed_get(NULL);

	tet_printf("[TET_PASS]:: %s[%d] : Test has passed..", __FILE__, __LINE__);
	tet_result(TET_PASS);
}

