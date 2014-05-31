#include <tet_api.h>
#include <Ecore.h>
#include <Ecore_Evas.h>
#include <Evas.h>
#include <Emotion.h>
#include <signal.h>
#include "common.h"

static void utc_emotion_object_module_option_set_p(void);
static void utc_emotion_object_module_option_set_null(void);

struct tet_testlist tet_testlist[] = {
	{ utc_emotion_object_module_option_set_p, POSITIVE_TC_IDX},
	{ utc_emotion_object_module_option_set_null, NULL_TC_IDX},
	{ NULL, 0 },
};

static void utc_emotion_object_module_option_set_p(void)
{
    Evas_Object *object = init();
    ecore_main_loop_begin();
    emotion_object_video_mute_set(object, EINA_FALSE);
    emotion_object_module_option_set(object, "video", "off");
    Eina_Bool res_video = emotion_object_video_mute_get(object);

    emotion_object_audio_mute_set(object, EINA_FALSE);
    emotion_object_module_option_set(object, "audio", "off");
    Eina_Bool res_audio = emotion_object_audio_mute_get(object);

	if (EINA_TRUE != res_video || EINA_TRUE != res_audio) {
		tet_printf("[TET_FAIL]:: %s[%d] : Test has failed..", __FILE__, __LINE__);
		tet_result(TET_FAIL);
		return;
    }

	tet_printf("[TET_PASS]:: %s[%d] : Test has passed..", __FILE__, __LINE__);
	tet_result(TET_PASS);
}

static void utc_emotion_object_module_option_set_null(void)
{
	signal(SIGSEGV, sigprocess);
	
    emotion_object_module_option_set(NULL, "video", "off");
    emotion_object_module_option_set(NULL, "audio", "off");

	tet_printf("[TET_PASS]:: %s[%d] : Test has passed..", __FILE__, __LINE__);
	tet_result(TET_PASS);
}

