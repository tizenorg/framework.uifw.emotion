#include <tet_api.h>
#include <Ecore.h>
#include <Ecore_Evas.h>
#include <Evas.h>
#include <Emotion.h>
#include <signal.h>
#include "common.h"

static void utc_emotion_object_spu_channel_count_p(void);
static void utc_emotion_object_spu_channel_count_null(void);

struct tet_testlist tet_testlist[] = {
	{ utc_emotion_object_spu_channel_count_p, POSITIVE_TC_IDX},
	{ utc_emotion_object_spu_channel_count_null, NULL_TC_IDX},
	{ NULL, 0 },
};

static void utc_emotion_object_spu_channel_count_p(void)
{
    Evas_Object *object = init();
    int count = emotion_object_spu_channel_count(object);

	if (0 != count) {
		tet_printf("[TET_FAIL]:: %s[%d] : Test has failed..", __FILE__, __LINE__);
		tet_result(TET_FAIL);
		return;
	}
	tet_printf("[TET_PASS]:: %s[%d] : Test has passed..", __FILE__, __LINE__);
	tet_result(TET_PASS);
}

static void utc_emotion_object_spu_channel_count_null(void)
{
	signal(SIGSEGV, sigprocess);
	
    emotion_object_spu_channel_count(NULL);

	tet_printf("[TET_PASS]:: %s[%d] : Test has passed..", __FILE__, __LINE__);
	tet_result(TET_PASS);
}

