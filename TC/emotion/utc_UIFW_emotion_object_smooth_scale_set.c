#include <tet_api.h>
#include <Ecore.h>
#include <Ecore_Evas.h>
#include <Evas.h>
#include <Emotion.h>
#include <signal.h>
#include "common.h"

static void utc_emotion_object_smooth_scale_set_p(void);
static void utc_emotion_object_smooth_scale_set_null(void);

struct tet_testlist tet_testlist[] = {
	{ utc_emotion_object_smooth_scale_set_p, POSITIVE_TC_IDX},
	{ utc_emotion_object_smooth_scale_set_null, NULL_TC_IDX},
	{ NULL, 0 },
};

static void utc_emotion_object_smooth_scale_set_p(void)
{
    Evas_Object *object = init();
    emotion_object_smooth_scale_set(object, EINA_TRUE);
    Eina_Bool smooth = emotion_object_smooth_scale_get(object);

	if (EINA_TRUE != smooth) {
		tet_printf("[TET_FAIL]:: %s[%d] : Test has failed..", __FILE__, __LINE__);
		tet_result(TET_FAIL);
		return;
	}
	tet_printf("[TET_PASS]:: %s[%d] : Test has passed..", __FILE__, __LINE__);
	tet_result(TET_PASS);
}

static void utc_emotion_object_smooth_scale_set_null(void)
{
	signal(SIGSEGV, sigprocess);
	
    emotion_object_smooth_scale_set(NULL, EINA_TRUE);

    tet_printf("[TET_PASS]:: %s[%d] : Test has passed..", __FILE__, __LINE__);
	tet_result(TET_PASS);
}

