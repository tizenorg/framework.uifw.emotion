#include <tet_api.h>
#include <Ecore.h>
#include <Ecore_Evas.h>
#include <Evas.h>
#include <Emotion.h>
#include <signal.h>
#include "common.h"

static void utc_emotion_object_add_p(void);
static void utc_emotion_object_add_n(void);

struct tet_testlist tet_testlist[] = {
	{ utc_emotion_object_add_p, POSITIVE_TC_IDX},
	{ utc_emotion_object_add_n, NEGATIVE_TC_IDX},
	{ NULL, 0 },
};

static void utc_emotion_object_add_p(void)
{
    Evas *evas = ecore_evas_get(ecore_evas);
    Evas_Object *object = emotion_object_add(evas);

	if (NULL == object) {
		tet_printf("[TET_FAIL]:: %s[%d] : Test has failed..", __FILE__, __LINE__);
		tet_result(TET_FAIL);
		return;
	}
	tet_printf("[TET_PASS]:: %s[%d] : Test has passed..", __FILE__, __LINE__);
	tet_result(TET_PASS);
}

static void utc_emotion_object_add_n(void)
{
    signal(SIGSEGV, sigprocess);
    
    Evas_Object *object = emotion_object_add(NULL);

	if (NULL != object) {
		tet_printf("[TET_FAIL]:: %s[%d] : Test has failed..", __FILE__, __LINE__);
		tet_result(TET_FAIL);
		return;
	}
	tet_printf("[TET_PASS]:: %s[%d] : Test has passed..", __FILE__, __LINE__);
	tet_result(TET_PASS);
}
