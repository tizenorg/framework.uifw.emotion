#include <tet_api.h>
#include <Ecore.h>
#include <Ecore_Evas.h>
#include <Evas.h>
#include <Emotion.h>
#include <signal.h>
#include "common.h"

static void utc_emotion_object_file_set_get_p(void);
static void utc_emotion_object_file_set_get_n(void);
static void utc_emotion_object_file_set_get_null(void);

struct tet_testlist tet_testlist[] = {
	{ utc_emotion_object_file_set_get_p, POSITIVE_TC_IDX},
	{ utc_emotion_object_file_set_get_n, NEGATIVE_TC_IDX},
	{ utc_emotion_object_file_set_get_null, NULL_TC_IDX},
	{ NULL, 0 },
};

static void utc_emotion_object_file_set_get_p(void)
{
    Evas_Object *object;
    Eina_Bool res = EINA_FALSE;

	signal(SIGSEGV, sigprocess);
	
    Evas *evas = ecore_evas_get(ecore_evas);
    object = emotion_object_add(evas);
    emotion_object_init(object, NULL);
    res = emotion_object_file_set(object, "test.avi");
    const char *file_name = emotion_object_file_get(object);

	if (EINA_FALSE == res || strcmp(file_name, "test.avi")) {
		tet_printf("[TET_FAIL]:: %s[%d] : Test has failed..", __FILE__, __LINE__);
		tet_result(TET_FAIL);
		return;
	}
	tet_printf("[TET_PASS]:: %s[%d] : Test has passed..", __FILE__, __LINE__);
	tet_result(TET_PASS);
}

static void utc_emotion_object_file_set_get_n(void)
{
	Evas *evas;  
    Evas_Object *object;
    Eina_Bool res = EINA_FALSE;

	signal(SIGSEGV, sigprocess);
	
    evas = ecore_evas_get(ecore_evas);
    object = emotion_object_add(evas);
    emotion_object_init(object, NULL);
    emotion_object_file_set(object, "test.avi");
    res = emotion_object_file_set(object, "fail.png");
    const char *file_name = emotion_object_file_get(object);
    tet_printf("file name: %s", file_name);

	if (EINA_FALSE != res || strcmp(file_name, "test.avi")) {
		tet_printf("[TET_FAIL]:: %s[%d] : Test has failed..", __FILE__, __LINE__);
		tet_result(TET_FAIL);
		return;
	}
	tet_printf("[TET_PASS]:: %s[%d] : Test has passed..", __FILE__, __LINE__);
	tet_result(TET_PASS);
}

static void utc_emotion_object_file_set_get_null(void)
{
	signal(SIGSEGV, sigprocess);
	
    emotion_object_file_set(NULL, NULL);
    emotion_object_file_get(NULL);

	tet_printf("[TET_PASS]:: %s[%d] : Test has passed..", __FILE__, __LINE__);
	tet_result(TET_PASS);
}
