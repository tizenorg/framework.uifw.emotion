#include <tet_api.h>
#include <Ecore.h>
#include <Ecore_Evas.h>
#include <Evas.h>
#include <Emotion.h>
#include <signal.h>
#include "common.h"

static void utc_emotion_object_ref_file_get_p(void);
static void utc_emotion_object_ref_file_get_null(void);

struct tet_testlist tet_testlist[] = {
	{ utc_emotion_object_ref_file_get_p, POSITIVE_TC_IDX},
	{ utc_emotion_object_ref_file_get_null, NULL_TC_IDX},
	{ NULL, 0 },
};

static void utc_emotion_object_ref_file_get_p(void)
{
    Evas_Object *object = init();
    const char *file = emotion_object_ref_file_get(object);
    tet_printf("file: %s", file);

	if (file != NULL) {
		tet_printf("[TET_FAIL]:: %s[%d] : Test has failed..", __FILE__, __LINE__);
		tet_result(TET_FAIL);
		return;
	}
	tet_printf("[TET_PASS]:: %s[%d] : Test has passed..", __FILE__, __LINE__);
	tet_result(TET_PASS);
}

static void utc_emotion_object_ref_file_get_null(void)
{
	signal(SIGSEGV, sigprocess);
	
    emotion_object_ref_file_get(NULL);

	tet_printf("[TET_PASS]:: %s[%d] : Test has passed..", __FILE__, __LINE__);
	tet_result(TET_PASS);
}

