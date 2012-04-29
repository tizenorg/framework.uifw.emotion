#include <tet_api.h>
#include <Ecore.h>
#include <Ecore_Evas.h>
#include <Evas.h>
#include <Emotion.h>
#include <signal.h>
#include "common.h"
#include "emotion_private.h"

static void utc_emotion_playback_started_p(void);
static void utc_emotion_playback_started_null(void);

struct tet_testlist tet_testlist[] = {
	{ utc_emotion_playback_started_p, POSITIVE_TC_IDX},
	{ utc_emotion_playback_started_null, NULL_TC_IDX},
	{ NULL, 0 },
};

static void utc_emotion_playback_started_p(void)
{
    Evas_Object *object = init();
    _emotion_playback_started(object);

	tet_printf("[TET_PASS]:: %s[%d] : Test has passed..", __FILE__, __LINE__);
	tet_result(TET_PASS);
}

static void utc_emotion_playback_started_null(void)
{
	signal(SIGSEGV, sigprocess);
	
    _emotion_playback_started(NULL);

	tet_printf("[TET_PASS]:: %s[%d] : Test has passed..", __FILE__, __LINE__);
	tet_result(TET_PASS);
}

