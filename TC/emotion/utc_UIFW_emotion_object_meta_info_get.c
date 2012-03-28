#include <tet_api.h>
#include <Ecore.h>
#include <Ecore_Evas.h>
#include <Evas.h>
#include <Emotion.h>
#include <signal.h>
#include "common.h"

static void utc_emotion_object_meta_info_get_p(void);
static void utc_emotion_object_meta_info_get_null(void);

struct tet_testlist tet_testlist[] = {
	{ utc_emotion_object_meta_info_get_p, POSITIVE_TC_IDX},
	{ utc_emotion_object_meta_info_get_null, NULL_TC_IDX},
	{ NULL, 0 },
};

static void utc_emotion_object_meta_info_get_p(void)
{
    Evas_Object *object = init();
    const char *title = emotion_object_meta_info_get(object, EMOTION_META_INFO_TRACK_TITLE);
    const char *artist = emotion_object_meta_info_get(object, EMOTION_META_INFO_TRACK_ARTIST);
    const char *album = emotion_object_meta_info_get(object, EMOTION_META_INFO_TRACK_ALBUM);
    const char *year = emotion_object_meta_info_get(object, EMOTION_META_INFO_TRACK_YEAR);
    const char *genre = emotion_object_meta_info_get(object, EMOTION_META_INFO_TRACK_GENRE);
    const char *comment = emotion_object_meta_info_get(object, EMOTION_META_INFO_TRACK_COMMENT);
    const char *disc_id = emotion_object_meta_info_get(object, EMOTION_META_INFO_TRACK_DISC_ID);
    const char *count = emotion_object_meta_info_get(object, EMOTION_META_INFO_TRACK_COUNT);
    tet_printf("title: %s, artist: %s, album: %s, year: %s, genre: %s, comment: %s, disc_id: %s, count: %s",
            title, artist, album, year, genre, comment, disc_id, count);

    // it was possible to set only these parameters with ffmpeg, so year, disc_id and count are not checked here
	if (strcmp(artist, "Test artist") || strcmp(comment, "Test comment") || strcmp(genre, "Test genre") || 
            strcmp(title, "Test title") || strcmp(album, "Test album")) {
		tet_printf("[TET_FAIL]:: %s[%d] : Test has failed..", __FILE__, __LINE__);
		tet_result(TET_FAIL);
		return;
	}
	tet_printf("[TET_PASS]:: %s[%d] : Test has passed..", __FILE__, __LINE__);
	tet_result(TET_PASS);
}

static void utc_emotion_object_meta_info_get_null(void)
{
	signal(SIGSEGV, sigprocess);
	
    emotion_object_meta_info_get(NULL, EMOTION_META_INFO_TRACK_TITLE);

	tet_printf("[TET_PASS]:: %s[%d] : Test has passed..", __FILE__, __LINE__);
	tet_result(TET_PASS);
}

