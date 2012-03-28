#include <tet_api.h>
#include <Ecore.h>
#include <Ecore_Evas.h>
#include <Evas.h>
#include <Emotion.h>
#include <signal.h>
#include "common.h"

static void utc_emotion_init_p(void);

struct tet_testlist tet_testlist[] = {
	{ utc_emotion_init_p, POSITIVE_TC_IDX},
	{ NULL, 0 },
};

static void utc_emotion_init_p(void)
{
    Eina_Bool res = emotion_init();

	if (EINA_FALSE == res) {
		tet_printf("[TET_FAIL]:: %s[%d] : Test has failed..", __FILE__, __LINE__);
		tet_result(TET_FAIL);
		return;
	}

	tet_printf("[TET_PASS]:: %s[%d] : Test has passed..", __FILE__, __LINE__);
	tet_result(TET_PASS);
}

