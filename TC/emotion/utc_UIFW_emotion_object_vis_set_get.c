#include <tet_api.h>
#include <Ecore.h>
#include <Ecore_Evas.h>
#include <Evas.h>
#include <Emotion.h>
#include <signal.h>
#include "common.h"

static void utc_emotion_object_vis_set_get_p(void);
static void utc_emotion_object_vis_set_get_null(void);

struct tet_testlist tet_testlist[] = {
	{ utc_emotion_object_vis_set_get_p, POSITIVE_TC_IDX},
	{ utc_emotion_object_vis_set_get_null, NULL_TC_IDX},
	{ NULL, 0 },
};

static void utc_emotion_object_vis_set_get_p(void)
{
    Evas_Object *object = init();
    
    for (int i = EMOTION_VIS_GOOM; i <= EMOTION_VIS_LIBVISUAL_PLASMA; ++i) {
        emotion_object_vis_set(object, i);
        Emotion_Vis vis = emotion_object_vis_get(object);
        
        if (vis != i) {
            tet_printf("[TET_FAIL]:: %s[%d] : Test has failed..", __FILE__, __LINE__);
            tet_result(TET_FAIL);
            return;
        } 
    }

//    emotion_object_vis_set(object, EMOTION_VIS_GOOM);
//    Emotion_Vis vis = emotion_object_vis_get(object);
//    if (vis != EMOTION_VIS_GOOM) {
//    
//    }
//    
//    emotion_object_vis_set(object, EMOTION_VIS_LIBVISUAL_BUMPSCOPE);
//    vis = emotion_object_vis_get(object);
//    
//    emotion_object_vis_set(object, EMOTION_VIS_LIBVISUAL_CORONA);
//    vis = emotion_object_vis_get(object);
//    
//    emotion_object_vis_set(object, EMOTION_VIS_LIBVISUAL_DANCING_PARTICLES);
//    vis = emotion_object_vis_get(object);
//    
//    emotion_object_vis_set(object, EMOTION_VIS_LIBVISUAL_GDKPIXBUF);
//    vis = emotion_object_vis_get(object);
//    
//    emotion_object_vis_set(object, EMOTION_VIS_LIBVISUAL_G_FORCE);
//    vis = emotion_object_vis_get(object);
//    
//    emotion_object_vis_set(object, EMOTION_VIS_LIBVISUAL_GOOM);
//    vis = emotion_object_vis_get(object);
//    
//    emotion_object_vis_set(object, EMOTION_VIS_LIBVISUAL_INFINITE);
//    vis = emotion_object_vis_get(object);
//    
//    emotion_object_vis_set(object, EMOTION_VIS_LIBVISUAL_JAKDAW);
//    vis = emotion_object_vis_get(object);
//    
//    emotion_object_vis_set(object, EMOTION_VIS_LIBVISUAL_JESS );
//    vis = emotion_object_vis_get(object);
//    
//    emotion_object_vis_set(object, EMOTION_VIS_LIBVISUAL_LV_ANALYSER);
//    vis = emotion_object_vis_get(object);
//    
//    emotion_object_vis_set(object, EMOTION_VIS_LIBVISUAL_LV_FLOWER);
//    vis = emotion_object_vis_get(object);
//    
//    emotion_object_vis_set(object, EMOTION_VIS_LIBVISUAL_LV_GLTEST);
//    vis = emotion_object_vis_get(object);
//    
//    emotion_object_vis_set(object, EMOTION_VIS_LIBVISUAL_LV_SCOPE);
//    vis = emotion_object_vis_get(object);
//    
//    emotion_object_vis_set(object, EMOTION_VIS_LIBVISUAL_MADSPIN);
//    vis = emotion_object_vis_get(object);
//    
//    emotion_object_vis_set(object, EMOTION_VIS_LIBVISUAL_NEBULUS);
//    vis = emotion_object_vis_get(object);
//    
//    emotion_object_vis_set(object, EMOTION_VIS_LIBVISUAL_OINKSIE);
//    vis = emotion_object_vis_get(object);
//    
//    emotion_object_vis_set(object, EMOTION_VIS_LIBVISUAL_PLASMA);
//    vis = emotion_object_vis_get(object);
//
//	if (goom != EINA_FALSE || bumpscope != EINA_FALSE || corona != EINA_FALSE || dancing_particles != EINA_FALSE
//            || gdbpixbuf != EINA_FALSE || g_force != EINA_FALSE || goom2 != EINA_FALSE || infinite != EINA_FALSE
//            || jakdaw != EINA_FALSE || jess != EINA_FALSE || analyser != EINA_FALSE || flower != EINA_FALSE
//            || madspin != EINA_FALSE || nebulus != EINA_FALSE || oinksie != EINA_FALSE || plasma != EINA_FALSE) {
//		tet_printf("[TET_FAIL]:: %s[%d] : Test has failed..", __FILE__, __LINE__);
//		tet_result(TET_FAIL);
//		return;
//	}
	tet_printf("[TET_PASS]:: %s[%d] : Test has passed..", __FILE__, __LINE__);
	tet_result(TET_PASS);
}

static void utc_emotion_object_vis_set_get_null(void)
{
	signal(SIGSEGV, sigprocess);
	
    emotion_object_vis_set(NULL, EMOTION_META_INFO_TRACK_TITLE);

	tet_printf("[TET_PASS]:: %s[%d] : Test has passed..", __FILE__, __LINE__);
	tet_result(TET_PASS);
}

