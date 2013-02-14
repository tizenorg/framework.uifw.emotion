#include <tet_api.h>
#include <Ecore.h>
#include <Ecore_Evas.h>
#include <Evas.h>
#include <Emotion.h>
#include <signal.h>
#include "common.h"
#include "emotion_private.h"

static void utc_emotion_webcam_custom_get_p(void);
static void utc_emotion_webcam_custom_get_null(void);

struct tet_testlist tet_testlist[] = {
	{ utc_emotion_webcam_custom_get_p, POSITIVE_TC_IDX},
	{ utc_emotion_webcam_custom_get_null, NULL_TC_IDX},
	{ NULL, 0 },
};

static void utc_emotion_webcam_custom_get_p(void)
{
    init();
    const char *launch = emotion_webcam_custom_get("/dev/webcam");

	if (launch == NULL) {
		tet_printf("[TET_FAIL]:: %s[%d] : Test has failed..", __FILE__, __LINE__);
		tet_result(TET_FAIL);
		return;
	}
	tet_printf("[TET_PASS]:: %s[%d] : Test has passed..", __FILE__, __LINE__);
	tet_result(TET_PASS);
}

static void utc_emotion_webcam_custom_get_null(void)
{
    init();
    const char *launch = emotion_webcam_custom_get(NULL);

    if (launch != NULL) {
        tet_printf("[TET_FAIL]:: %s[%d] : Test has failed..", __FILE__, __LINE__);
		tet_result(TET_FAIL);
		return;
    }

	tet_printf("[TET_PASS]:: %s[%d] : Test has passed..", __FILE__, __LINE__);
	tet_result(TET_PASS);
}

