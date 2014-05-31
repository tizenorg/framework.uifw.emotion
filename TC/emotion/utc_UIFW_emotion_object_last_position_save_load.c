#include <tet_api.h>
#include <Ecore.h>
#include <Ecore_Evas.h>
#include <Evas.h>
#include <Emotion.h>
#include <signal.h>
#include "common.h"

static void utc_emotion_object_last_position_save_load_p(void);
static void utc_emotion_object_last_position_save_load_null(void);

struct tet_testlist tet_testlist[] = {
	{ utc_emotion_object_last_position_save_load_p, POSITIVE_TC_IDX},
	{ utc_emotion_object_last_position_save_load_null, NULL_TC_IDX},
	{ NULL, 0 },
};

static void utc_emotion_object_last_position_save_load_p(void)
{
    Evas_Object *object = init();
    emotion_object_position_set(object, 5.0);
    emotion_object_last_position_save(object);
    emotion_object_position_set(object, 7.0);
    emotion_object_last_position_load(object);
    double last_position = emotion_object_position_get(object);
    tet_printf("Last position: %d", last_position);

	if (5.0 != last_position) {
		tet_printf("[TET_FAIL]:: %s[%d] : Test has failed..", __FILE__, __LINE__);
		tet_result(TET_FAIL);
		return;
	}
	tet_printf("[TET_PASS]:: %s[%d] : Test has passed..", __FILE__, __LINE__);
	tet_result(TET_PASS);
}

static void utc_emotion_object_last_position_save_load_null(void)
{
	signal(SIGSEGV, sigprocess);

    emotion_object_last_position_save(NULL);
    emotion_object_last_position_load(NULL);

	tet_printf("[TET_PASS]:: %s[%d] : Test has passed..", __FILE__, __LINE__);
	tet_result(TET_PASS);
}

