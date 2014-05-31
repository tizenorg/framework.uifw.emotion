#include <tet_api.h>
#include <Ecore.h>
#include <Ecore_Evas.h>
#include <Evas.h>
#include <Emotion.h>
#include <signal.h>
#include "common.h"

static void utc_emotion_object_vis_supported_p(void);
static void utc_emotion_object_vis_supported_null(void);

struct tet_testlist tet_testlist[] = {
	{ utc_emotion_object_vis_supported_p, POSITIVE_TC_IDX},
	{ utc_emotion_object_vis_supported_null, NULL_TC_IDX},
	{ NULL, 0 },
};

static void utc_emotion_object_vis_supported_p(void)
{
    Evas_Object *object = init();
       
    Eina_Bool goom = emotion_object_vis_supported(object, EMOTION_VIS_GOOM);
    Eina_Bool bumpscope = emotion_object_vis_supported(object, EMOTION_VIS_LIBVISUAL_BUMPSCOPE);
    Eina_Bool corona = emotion_object_vis_supported(object, EMOTION_VIS_LIBVISUAL_CORONA);
    Eina_Bool dancing_particles = emotion_object_vis_supported(object, EMOTION_VIS_LIBVISUAL_DANCING_PARTICLES);
    Eina_Bool gdbpixbuf = emotion_object_vis_supported(object, EMOTION_VIS_LIBVISUAL_GDKPIXBUF);
    Eina_Bool g_force = emotion_object_vis_supported(object, EMOTION_VIS_LIBVISUAL_G_FORCE);
    Eina_Bool goom2 = emotion_object_vis_supported(object, EMOTION_VIS_LIBVISUAL_GOOM);
    Eina_Bool infinite = emotion_object_vis_supported(object, EMOTION_VIS_LIBVISUAL_INFINITE);
    Eina_Bool jakdaw = emotion_object_vis_supported(object, EMOTION_VIS_LIBVISUAL_JAKDAW);
    Eina_Bool jess = emotion_object_vis_supported(object, EMOTION_VIS_LIBVISUAL_JESS );
    Eina_Bool analyser = emotion_object_vis_supported(object, EMOTION_VIS_LIBVISUAL_LV_ANALYSER);
    Eina_Bool flower = emotion_object_vis_supported(object, EMOTION_VIS_LIBVISUAL_LV_FLOWER);
    Eina_Bool gltest = emotion_object_vis_supported(object, EMOTION_VIS_LIBVISUAL_LV_GLTEST);
    Eina_Bool scope = emotion_object_vis_supported(object, EMOTION_VIS_LIBVISUAL_LV_SCOPE);
    Eina_Bool madspin = emotion_object_vis_supported(object, EMOTION_VIS_LIBVISUAL_MADSPIN);
    Eina_Bool nebulus = emotion_object_vis_supported(object, EMOTION_VIS_LIBVISUAL_NEBULUS);
    Eina_Bool oinksie = emotion_object_vis_supported(object, EMOTION_VIS_LIBVISUAL_OINKSIE);
    Eina_Bool plasma = emotion_object_vis_supported(object, EMOTION_VIS_LIBVISUAL_PLASMA);

    tet_printf("goom: %d, bumpscope: %d, corona: %d, dancing_particles: %d, gdbpixbuf: %d, \
            g_force: %d, goom2: %d, infinite: %d, jakdaw: %d, jess: %d, analyser: %d, flower: %d, \
            gltest: %d, scope: %d, madspin: %d, nebulus: %d, oinksie: %d, plasma: %d",
            goom, bumpscope, corona, dancing_particles, gdbpixbuf, g_force, goom2, infinite,
            jakdaw, jess, analyser, flower, gltest, scope, madspin, nebulus, oinksie, plasma);

	if (goom != EINA_FALSE || bumpscope != EINA_FALSE || corona != EINA_FALSE || dancing_particles != EINA_FALSE
            || gdbpixbuf != EINA_FALSE || g_force != EINA_FALSE || goom2 != EINA_FALSE || infinite != EINA_FALSE
            || jakdaw != EINA_FALSE || jess != EINA_FALSE || analyser != EINA_FALSE || flower != EINA_FALSE
            || madspin != EINA_FALSE || nebulus != EINA_FALSE || oinksie != EINA_FALSE || plasma != EINA_FALSE) {
		tet_printf("[TET_FAIL]:: %s[%d] : Test has failed..", __FILE__, __LINE__);
		tet_result(TET_FAIL);
		return;
	}
	tet_printf("[TET_PASS]:: %s[%d] : Test has passed..", __FILE__, __LINE__);
	tet_result(TET_PASS);
}

static void utc_emotion_object_vis_supported_null(void)
{
	signal(SIGSEGV, sigprocess);
	
    emotion_object_vis_supported(NULL, EMOTION_META_INFO_TRACK_TITLE);

	tet_printf("[TET_PASS]:: %s[%d] : Test has passed..", __FILE__, __LINE__);
	tet_result(TET_PASS);
}

