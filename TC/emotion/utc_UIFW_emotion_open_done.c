#include <tet_api.h>
#include <Ecore.h>
#include <Ecore_Evas.h>
#include <Evas.h>
#include <Emotion.h>
#include <signal.h>
#include "common.h"
#include "emotion_private.h"

static void utc_emotion_open_done_p(void);
static void utc_emotion_open_done_null(void);

struct tet_testlist tet_testlist[] = {
	{ utc_emotion_open_done_p, POSITIVE_TC_IDX},
	{ utc_emotion_open_done_null, NULL_TC_IDX},
	{ NULL, 0 },
};

static void utc_emotion_open_done_p(void)
{
    Evas_Object *object = init();
    _emotion_open_done(object);

	tet_printf("[TET_PASS]:: %s[%d] : Test has passed..", __FILE__, __LINE__);
	tet_result(TET_PASS);
}

static void utc_emotion_open_done_null(void)
{
	signal(SIGSEGV, sigprocess);
	
    _emotion_open_done(NULL);

	tet_printf("[TET_PASS]:: %s[%d] : Test has passed..", __FILE__, __LINE__);
	tet_result(TET_PASS);
}

