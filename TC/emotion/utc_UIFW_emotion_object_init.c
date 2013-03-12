#include <tet_api.h>
#include <Ecore.h>
#include <Ecore_Evas.h>
#include <Evas.h>
#include <Emotion.h>
#include <signal.h>
#include "common.h"

static void utc_emotion_object_init_p(void);
static void utc_emotion_object_init_n(void);
static void utc_emotion_object_init_null(void);

struct tet_testlist tet_testlist[] = {
	{ utc_emotion_object_init_p, POSITIVE_TC_IDX},
	{ utc_emotion_object_init_n, NEGATIVE_TC_IDX},
	{ utc_emotion_object_init_null, NULL_TC_IDX},
	{ NULL, 0 },
};

static void utc_emotion_object_init_p(void)
{
	Evas *evas = NULL;  
    Evas_Object *object = NULL;
    Eina_Bool res_gstreamer = EINA_FALSE;

	signal(SIGSEGV, sigprocess);
	
    evas = ecore_evas_get(ecore_evas);
    object = emotion_object_add(evas);
    res_gstreamer = emotion_object_init(object, NULL);

	if (EINA_FALSE == res_gstreamer) {
		tet_printf("[TET_FAIL]:: %s[%d] : Test has failed..", __FILE__, __LINE__);
		tet_result(TET_FAIL);
		return;
	}
	tet_printf("[TET_PASS]:: %s[%d] : Test has passed..", __FILE__, __LINE__);
	tet_result(TET_PASS);
}

static void utc_emotion_object_init_n(void)
{
	Evas *evas = NULL;  
    Evas_Object *object = NULL;
    Eina_Bool res = EINA_FALSE;

	signal(SIGSEGV, sigprocess);
	
    evas = ecore_evas_get(ecore_evas);
    object = emotion_object_add(evas);
    res = emotion_object_init(object, "test");

	if (EINA_FALSE != res) {
		tet_printf("[TET_FAIL]:: %s[%d] : Test has failed..", __FILE__, __LINE__);
		tet_result(TET_FAIL);
		return;
	}
	tet_printf("[TET_PASS]:: %s[%d] : Test has passed..", __FILE__, __LINE__);
	tet_result(TET_PASS);
}

static void utc_emotion_object_init_null(void)
{
	signal(SIGSEGV, sigprocess);
	
    emotion_object_init(NULL, NULL);

    tet_printf("[TET_PASS]:: %s[%d] : Test has passed..", __FILE__, __LINE__);
	tet_result(TET_PASS);
}
