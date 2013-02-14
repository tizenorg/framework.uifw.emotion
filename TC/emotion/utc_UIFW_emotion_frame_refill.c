#include <tet_api.h>
#include <Ecore.h>
#include <Ecore_Evas.h>
#include <Evas.h>
#include <Emotion.h>
#include <signal.h>
#include "common.h"
#include "emotion_private.h"

static void utc_emotion_frame_refill_p(void);
static void utc_emotion_frame_refill_n(void);
static void utc_emotion_frame_refill_null(void);

struct tet_testlist tet_testlist[] = {
	{ utc_emotion_frame_refill_p, POSITIVE_TC_IDX},
	{ utc_emotion_frame_refill_n, NEGATIVE_TC_IDX},
	{ utc_emotion_frame_refill_null, NULL_TC_IDX},
	{ NULL, 0 },
};

static void utc_emotion_frame_refill_p(void)
{
    Evas_Object *object = init();
    _emotion_frame_refill(object, 5, 5);

	tet_printf("[TET_PASS]:: %s[%d] : Test has passed..", __FILE__, __LINE__);
	tet_result(TET_PASS);
}

static void utc_emotion_frame_refill_n(void)
{
	signal(SIGSEGV, sigprocess);
	
    Evas_Object *object = init();
    _emotion_frame_refill(object, -5, -5);
    _emotion_frame_refill(object, 0, 0);

	tet_printf("[TET_PASS]:: %s[%d] : Test has passed..", __FILE__, __LINE__);
	tet_result(TET_PASS);
}

static void utc_emotion_frame_refill_null(void)
{
	signal(SIGSEGV, sigprocess);
	
    _emotion_frame_refill(NULL, 0, 0);

	tet_printf("[TET_PASS]:: %s[%d] : Test has passed..", __FILE__, __LINE__);
	tet_result(TET_PASS);
}

