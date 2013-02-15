#include <tet_api.h>
#include <Ecore.h>
#include <Ecore_Evas.h>
#include <Evas.h>
#include <Emotion.h>
#include <signal.h>
#include "common.h"

static void utc_emotion_object_event_simple_send_p(void);
static void utc_emotion_object_event_simple_send_null(void);

struct tet_testlist tet_testlist[] = {
	{ utc_emotion_object_event_simple_send_p, POSITIVE_TC_IDX},
	{ utc_emotion_object_event_simple_send_null, NULL_TC_IDX},
	{ NULL, 0 },
};

static void utc_emotion_object_event_simple_send_p(void)
{
    Evas_Object *object = init();
    
    for (int event = EMOTION_EVENT_MENU1; event <= EMOTION_EVENT_10; ++event) {
        emotion_object_event_simple_send(object, event);
    }

	tet_printf("[TET_PASS]:: %s[%d] : Test has passed..", __FILE__, __LINE__);
	tet_result(TET_PASS);
}

static void utc_emotion_object_event_simple_send_null(void)
{
	signal(SIGSEGV, sigprocess);
	
    emotion_object_vis_set(NULL, EMOTION_EVENT_10 + 1);

	tet_printf("[TET_PASS]:: %s[%d] : Test has passed..", __FILE__, __LINE__);
	tet_result(TET_PASS);
}

