#include <tet_api.h>
#include <Ecore.h>
#include <Ecore_Evas.h>
#include <Evas.h>
#include <Emotion.h>
#include <signal.h>
#include "emotion_private.h"
#include "common.h"

static void utc_emotion_frame_new_p(void);
static void utc_emotion_frame_new_null(void);

struct tet_testlist tet_testlist[] = {
	{ utc_emotion_frame_new_p, POSITIVE_TC_IDX},
	{ utc_emotion_frame_new_null, NULL_TC_IDX},
	{ NULL, 0 },
};

static void utc_emotion_frame_new_p(void)
{
    Evas_Object *object = init();	
    _emotion_frame_new(object);

	tet_printf("[TET_PASS]:: %s[%d] : Test has passed..", __FILE__, __LINE__);
	tet_result(TET_PASS);
}

static void utc_emotion_frame_new_null(void)
{
	signal(SIGSEGV, sigprocess);
    
    _emotion_frame_new(NULL);

	tet_printf("[TET_PASS]:: %s[%d] : Test has passed..", __FILE__, __LINE__);
	tet_result(TET_PASS);
}

