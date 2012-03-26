#include <tet_api.h>
#include <Ecore.h>
#include <Ecore_Evas.h>
#include <Evas.h>
#include <Emotion.h>
#include <signal.h>
#include "common.h"
#include "emotion_private.h"

static void utc_emotion_module_unregister_p(void);
static void utc_emotion_module_unregister_null(void);

struct tet_testlist tet_testlist[] = {
	{ utc_emotion_module_unregister_p, POSITIVE_TC_IDX},
	{ utc_emotion_module_unregister_null, NULL_TC_IDX},
	{ NULL, 0 },
};

static Eina_Bool
module_open(Evas_Object *obj,
        const Emotion_Video_Module **module,
        void **video,
        Emotion_Module_Options *opt)
{
    tet_printf("module_open");
    return EINA_TRUE;
}

static void
module_close(Emotion_Video_Module *module,
             void *video)
{
    tet_printf("module_close");
}

static void utc_emotion_module_unregister_p(void)
{
    init();
    _emotion_module_register("gstreamer", module_open, module_close);
    _emotion_module_unregister("gstreamer");

	tet_printf("[TET_PASS]:: %s[%d] : Test has passed..", __FILE__, __LINE__);
	tet_result(TET_PASS);
}

static void utc_emotion_module_unregister_null(void)
{
	signal(SIGSEGV, sigprocess);
	
    _emotion_module_unregister(NULL);

	tet_printf("[TET_PASS]:: %s[%d] : Test has passed..", __FILE__, __LINE__);
	tet_result(TET_PASS);
}

