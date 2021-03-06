#ifdef HAVE_CONFIG_H
# include "config.h"
#endif

#include <Evas.h>
#include <Ecore.h>

#ifdef HAVE_EIO
# include <math.h>
# include <Eio.h>
#endif

#include "Emotion.h"
#include "emotion_private.h"

#ifdef _WIN32
# define FMT_UCHAR "%c"
#else
# define FMT_UCHAR "%hhu"
#endif

#define E_SMART_OBJ_GET(smart, o, type) \
     { \
	char *_e_smart_str; \
	\
	if (!o) return; \
	smart = evas_object_smart_data_get(o); \
	if (!smart) return; \
	_e_smart_str = (char *)evas_object_type_get(o); \
	if (!_e_smart_str) return; \
	if (strcmp(_e_smart_str, type)) return; \
     }

#define E_SMART_OBJ_GET_RETURN(smart, o, type, ret) \
   { \
      char *_e_smart_str; \
      \
      if (!o) return ret; \
      smart = evas_object_smart_data_get(o); \
      if (!smart) return ret; \
      _e_smart_str = (char *)evas_object_type_get(o); \
      if (!_e_smart_str) return ret; \
      if (strcmp(_e_smart_str, type)) return ret; \
   }

#define DBG(...) EINA_LOG_DOM_DBG(_log_domain, __VA_ARGS__)
#define INF(...) EINA_LOG_DOM_INFO(_log_domain, __VA_ARGS__)
#define WRN(...) EINA_LOG_DOM_WARN(_log_domain, __VA_ARGS__)
#define ERR(...) EINA_LOG_DOM_ERR(_log_domain, __VA_ARGS__)
#define CRITICAL(...) EINA_LOG_DOM_CRIT(_log_domain, __VA_ARGS__)

#define E_OBJ_NAME "emotion_object"

typedef struct _Smart_Data Smart_Data;

struct _Smart_Data
{
   EINA_REFCOUNT;
   Emotion_Video_Module  *module;
   void                  *video_data;

   char                  *module_name;

   const char    *file;
   Evas_Object   *obj;
   Evas_Object   *bg;

   Ecore_Job     *job;

   char *title;

#ifdef HAVE_EIO
   Eio_File *load_xattr;
#endif

   struct {
      char   *info;
      double  stat;
   } progress;
   struct {
      char *file;
      int   num;
   } ref;
   struct {
      int button_num;
      int button;
   } spu;
   struct {
      int l; /* left */
      int r; /* right */
      int t; /* top */
      int b; /* bottom */
      Evas_Object *clipper;
   } crop;

   struct {
      int         w, h;
   } video;
   struct {
      double      w, h;
   } fill;

   double         ratio;
   double         pos;
   double         remember_jump;
   double         seek_pos;
   double         len;

   Emotion_Module_Options module_options;

   Emotion_Suspend state;
   Emotion_Aspect aspect;

   Ecore_Animator *anim;

   Eina_Bool open : 1;
   Eina_Bool play : 1;
   Eina_Bool remember_play : 1;
   Eina_Bool seek : 1;
   Eina_Bool seeking : 1;
};

static void _mouse_move(void *data, Evas *ev, Evas_Object *obj, void *event_info);
static void _mouse_down(void *data, Evas *ev, Evas_Object *obj, void *event_info);
static void _pos_set_job(void *data);
static void _pixels_get(void *data, Evas_Object *obj);

static void _smart_init(void);
static void _smart_add(Evas_Object * obj);
static void _smart_del(Evas_Object * obj);
static void _smart_move(Evas_Object * obj, Evas_Coord x, Evas_Coord y);
static void _smart_resize(Evas_Object * obj, Evas_Coord w, Evas_Coord h);
static void _smart_show(Evas_Object * obj);
static void _smart_hide(Evas_Object * obj);
static void _smart_color_set(Evas_Object * obj, int r, int g, int b, int a);
static void _smart_clip_set(Evas_Object * obj, Evas_Object * clip);
static void _smart_clip_unset(Evas_Object * obj);

/**********************************/
/* Globals for the E Video Object */
/**********************************/
static Evas_Smart  *smart = NULL;
static Eina_Hash *_backends = NULL;
static Eina_Array *_modules = NULL;
static int _log_domain = -1;

static const char *_backend_priority[] = {
  "gstreamer",
  "xine",
  "generic"
};

static const char SIG_FRAME_DECODE[] = "frame_decode";
static const char SIG_POSITION_UPDATE[] = "position_update";
static const char SIG_LENGTH_CHANGE[] = "length_change";
static const char SIG_FRAME_RESIZE[] = "frame_resize";
static const char SIG_DECODE_STOP[] = "decode_stop";
static const char SIG_PLAYBACK_STARTED[] = "playback_started";
static const char SIG_PLAYBACK_FINISHED[] = "playback_finished";
static const char SIG_AUDIO_LEVEL_CHANGE[] = "audio_level_change";
static const char SIG_CHANNELS_CHANGE[] = "channels_change";
static const char SIG_TITLE_CHANGE[] = "title_change";
static const char SIG_PROGRESS_CHANGE[] = "progress_change";
static const char SIG_REF_CHANGE[] = "ref_change";
static const char SIG_BUTTON_NUM_CHANGE[] = "button_num_change";
static const char SIG_BUTTON_CHANGE[] = "button_change";
static const char SIG_OPEN_DONE[] = "open_done";
static const char SIG_POSITION_SAVE_SUCCEED[] = "position_save,succeed";
static const char SIG_POSITION_SAVE_FAILED[] = "position_save,failed";
static const char SIG_POSITION_LOAD_SUCCEED[] = "position_load,succeed";
static const char SIG_POSITION_LOAD_FAILED[] = "position_load,failed";

static const Evas_Smart_Cb_Description _smart_callbacks[] = {
  {SIG_FRAME_DECODE, ""},
  {SIG_POSITION_UPDATE, ""},
  {SIG_LENGTH_CHANGE, ""},
  {SIG_FRAME_RESIZE, ""},
  {SIG_DECODE_STOP, ""},
  {SIG_PLAYBACK_STARTED, ""},
  {SIG_PLAYBACK_FINISHED, ""},
  {SIG_AUDIO_LEVEL_CHANGE, ""},
  {SIG_CHANNELS_CHANGE, ""},
  {SIG_TITLE_CHANGE, ""},
  {SIG_PROGRESS_CHANGE, ""},
  {SIG_REF_CHANGE, ""},
  {SIG_BUTTON_NUM_CHANGE, ""},
  {SIG_BUTTON_CHANGE, ""},
  {SIG_OPEN_DONE, ""},
  {NULL, NULL}
};

static void
_emotion_image_data_zero(Evas_Object *img)
{
   void *data;

   data = evas_object_image_data_get(img, 1);
   if (data)
     {
        int w, h, sz = 0;
        Evas_Colorspace cs;

        evas_object_image_size_get(img, &w, &h);
        cs = evas_object_image_colorspace_get(img);
        if (cs == EVAS_COLORSPACE_ARGB8888)
           sz = w * h * 4;
        if ((cs == EVAS_COLORSPACE_YCBCR422P601_PL) ||
            (cs == EVAS_COLORSPACE_YCBCR422P709_PL))
           sz = h * 2 * sizeof(unsigned char *);
        if (sz != 0) memset(data, 0, sz);
     }
   evas_object_image_data_set(img, data);
}

static void
_emotion_module_close(Emotion_Video_Module *mod, void *video)
{
   if (!mod) return;
   if (mod->plugin->close && video)
     mod->plugin->close(mod, video);
   /* FIXME: we can't go dlclosing here as a thread still may be running from
    * the module - this in theory will leak- but it shouldn't be too bad and
    * mean that once a module is dlopened() it can't be closed - its refcount
    * will just keep going up
    */
}

static void
_smart_data_free(Smart_Data *sd)
{
   if (sd->video_data) sd->module->file_close(sd->video_data);
   _emotion_module_close(sd->module, sd->video_data);
   evas_object_del(sd->obj);
   evas_object_del(sd->crop.clipper);
   evas_object_del(sd->bg);
   eina_stringshare_del(sd->file);
   free(sd->module_name);
   if (sd->job) ecore_job_del(sd->job);
   if (sd->anim) ecore_animator_del(sd->anim);
   free(sd->progress.info);
   free(sd->ref.file);
   free(sd);

   ecore_shutdown();
}

EAPI Eina_Bool
_emotion_module_register(const char *name, Emotion_Module_Open mod_open, Emotion_Module_Close mod_close)
{
   Eina_Emotion_Plugins *plugin;

   plugin = malloc(sizeof (Eina_Emotion_Plugins));
   if (!plugin) return EINA_FALSE;

   plugin->open = mod_open;
   plugin->close = mod_close;

   return eina_hash_add(_backends, name, plugin);
}

EAPI Eina_Bool
_emotion_module_unregister(const char *name)
{
   return eina_hash_del(_backends, name, NULL);
}

static const char *
_emotion_module_open(const char *name, Evas_Object *obj, Emotion_Video_Module **mod, void **video)
{
   Eina_Emotion_Plugins *plugin;
   Smart_Data *sd;
   unsigned int i = 0;

   E_SMART_OBJ_GET_RETURN(sd, obj, E_OBJ_NAME, 0);
   if (!_backends)
     {
        ERR("No backend loaded");
	return NULL;
     }

   if (!name && getenv("EMOTION_ENGINE"))
     name = getenv("EMOTION_ENGINE");

   /* FIXME: Always look for a working backend. */
 retry:
   if (!name || i > 0)
     name = _backend_priority[i++];

   plugin = eina_hash_find(_backends, name);
   if (!plugin)
     {
	if (i != 0 && i < (sizeof (_backend_priority) / sizeof (char*)))
	  goto retry;

	ERR("No backend loaded");
	return EINA_FALSE;
     }

   if (plugin->open(obj, (const Emotion_Video_Module **) mod, video, &(sd->module_options)))
     {
	if (*mod)
	  {
	     (*mod)->plugin = plugin;
	     return name;
	  }
     }

   if (i != 0 && i < (sizeof (_backend_priority) / sizeof (char*)))
     goto retry;

   ERR("Unable to load module: %s", name);

   return NULL;
}

static void
_clipper_position_size_update(Evas_Object *obj, int x, int y, int w, int h, int vid_w, int vid_h)
{
   Smart_Data *sd;
   double scale_w, scale_h;

   E_SMART_OBJ_GET(sd, obj, E_OBJ_NAME);

   if (vid_w == 0 || vid_h == 0)
     {
       evas_object_image_fill_set(sd->obj, 0, 0, 0, 0);
       evas_object_move(sd->obj, x, y);
       evas_object_resize(sd->obj, 0, 0);
       evas_object_move(sd->crop.clipper, x, y);
       evas_object_resize(sd->crop.clipper, 0, 0);
     }
   else
     {
       evas_object_move(sd->crop.clipper, x, y);
       scale_w = (double)w / (double)(vid_w - sd->crop.l - sd->crop.r);
       scale_h = (double)h / (double)(vid_h - sd->crop.t - sd->crop.b);

       if (sd->fill.w < 0 && sd->fill.h < 0)
         evas_object_image_fill_set(sd->obj, 0, 0, vid_w * scale_w, vid_h * scale_h);
       else
         evas_object_image_fill_set(sd->obj, 0, 0, sd->fill.w * w, sd->fill.h * h);
       evas_object_resize(sd->obj, vid_w * scale_w, vid_h * scale_h);
       evas_object_move(sd->obj, x - sd->crop.l * scale_w, y - sd->crop.t * scale_h);
       evas_object_resize(sd->crop.clipper, w, h);
     }
}

/*******************************/
/* Externally accessible calls */
/*******************************/



EAPI Evas_Object *
emotion_object_add(Evas *evas)
{
   _smart_init();
   return evas_object_smart_add(evas, smart);
}

EAPI Evas_Object *
emotion_object_image_get(const Evas_Object *obj)
{
   Smart_Data *sd;

   sd = evas_object_smart_data_get(obj);
   if (!sd) return NULL;
   return sd->obj;
}

EAPI void
emotion_object_module_option_set(Evas_Object *obj, const char *opt, const char *val)
{
   Smart_Data *sd;

   E_SMART_OBJ_GET(sd, obj, E_OBJ_NAME);
   if ((!opt) || (!val)) return;

   if (!strcmp(opt, "player"))
     eina_stringshare_replace(&sd->module_options.player, val);
}

EAPI Eina_Bool
emotion_object_init(Evas_Object *obj, const char *module_filename)
{
   Smart_Data *sd;
   const char *file;

   E_SMART_OBJ_GET_RETURN(sd, obj, E_OBJ_NAME, 0);

   if ((sd->module_name) && module_filename && (!strcmp(sd->module_name, module_filename)))
     return EINA_TRUE;
   free(sd->module_name);
   sd->module_name = NULL;

   file = sd->file;
   sd->file = NULL;

   free(sd->title);
   sd->title = NULL;
   free(sd->progress.info);
   sd->progress.info = NULL;
   sd->progress.stat = 0.0;
   free(sd->ref.file);
   sd->ref.file = NULL;
   sd->ref.num = 0;
   sd->spu.button_num = 0;
   sd->spu.button = -1;
   sd->ratio = 1.0;
   sd->pos = 0;
   sd->remember_jump = 0;
   sd->seek_pos = 0;
   sd->len = 0;
   sd->remember_play = 0;

   if (sd->anim) ecore_animator_del(sd->anim);
   sd->anim = NULL;

   _emotion_module_close(sd->module, sd->video_data);
   sd->module = NULL;
   sd->video_data = NULL;

   module_filename = _emotion_module_open(module_filename, obj, &sd->module, &sd->video_data);
   if (!module_filename)
     return EINA_FALSE;

   sd->module_name = strdup(module_filename);

   if (file)
     {
	emotion_object_file_set(obj, file);
	eina_stringshare_del(file);
     }

   return EINA_TRUE;
}

EAPI Eina_Bool
emotion_object_file_set(Evas_Object *obj, const char *file)
{
   Smart_Data *sd;

   E_SMART_OBJ_GET_RETURN(sd, obj, E_OBJ_NAME, EINA_FALSE);

   DBG("file=%s", file);
   if (!sd->module) return EINA_FALSE;

   sd->video.w = 0;
   sd->video.h = 0;
   if ((file) && (sd->file) &&
       ((file == sd->file) || (!strcmp(file, sd->file)))) return EINA_FALSE;
   if ((file) && (file[0] != 0))
     {
	eina_stringshare_replace(&sd->file, file);
	sd->module->file_close(sd->video_data);
        evas_object_image_data_set(sd->obj, NULL);
	evas_object_image_size_set(sd->obj, 1, 1);
        _emotion_image_data_zero(sd->obj);
        sd->open = 0;
	if (!sd->module->file_open(sd->file, obj, sd->video_data))
	  return EINA_FALSE;
	sd->pos = 0.0;
	if (sd->play) sd->module->play(sd->video_data, 0.0);
     }
   else
     {
        if (sd->video_data && sd->module)
	  {
	     sd->module->file_close(sd->video_data);
             evas_object_image_data_set(sd->obj, NULL);
	     evas_object_image_size_set(sd->obj, 1, 1);
             _emotion_image_data_zero(sd->obj);
	  }
        eina_stringshare_replace(&sd->file, NULL);
     }

   if (sd->anim) ecore_animator_del(sd->anim);
   sd->anim = NULL;

#ifdef HAVE_EIO
   /* Only cancel the load_xattr or we will loose ref to time_seek stringshare */
   if (sd->load_xattr) eio_file_cancel(sd->load_xattr);
   sd->load_xattr = NULL;
#endif

   return EINA_TRUE;
}

EAPI const char *
emotion_object_file_get(const Evas_Object *obj)
{
   Smart_Data *sd;

   E_SMART_OBJ_GET_RETURN(sd, obj, E_OBJ_NAME, NULL);
   return sd->file;
}

static void
_emotion_aspect_borders_apply(Evas_Object *obj, Smart_Data *sd, int w, int h, int iw, int ih)
{
   int x, y;

   evas_object_geometry_get(obj, &x, &y, NULL, NULL);

   /* applying calculated borders */
   if (sd->crop.l == 0 && sd->crop.r == 0 &&
       sd->crop.t == 0 && sd->crop.b == 0)
     {
	Evas_Object *old_clipper;
	if (sd->crop.clipper)
	  {
	     old_clipper = evas_object_clip_get(sd->crop.clipper);
	     evas_object_clip_unset(sd->obj);
	     evas_object_clip_set(sd->obj, old_clipper);
	     evas_object_del(sd->crop.clipper);
	     sd->crop.clipper = NULL;
	  }
     }
   else
     {
	if (!sd->crop.clipper)
	  {
	     Evas_Object *old_clipper;
	     sd->crop.clipper = evas_object_rectangle_add(
		evas_object_evas_get(obj));
	     evas_object_color_set(sd->crop.clipper, 255, 255, 255, 255);
	     evas_object_smart_member_add(sd->crop.clipper, obj);
	     old_clipper = evas_object_clip_get(sd->obj);
	     evas_object_clip_set(sd->obj, sd->crop.clipper);
	     evas_object_clip_set(sd->crop.clipper, old_clipper);
	     if (evas_object_visible_get(sd->obj))
	       evas_object_show(sd->crop.clipper);
	  }
     }
   _clipper_position_size_update(obj, x, y, w, h, iw, ih);
}

static void
_emotion_object_aspect_border_apply(Evas_Object *obj, Smart_Data *sd, int w, int h)
{
   int iw, ih;
   double ir;
   double r;

   int aspect_opt = 0;

   iw = sd->video.w;
   ih = sd->video.h;

   ir = (double)iw / ih;
   r = (double)w / h;

   /* First check if we should fit the width or height of the video inside the
    * width/height of the object.  This check takes into account the original
    * aspect ratio and the object aspect ratio, if we are keeping both sizes or
    * cropping the exceding area.
    */
   if (sd->aspect == EMOTION_ASPECT_KEEP_NONE)
     {
	sd->crop.l = 0;
	sd->crop.r = 0;
	sd->crop.t = 0;
	sd->crop.b = 0;
	aspect_opt = 0; // just ignore keep_aspect
     }
   else if (sd->aspect == EMOTION_ASPECT_KEEP_WIDTH)
     {
	aspect_opt = 1;
     }
   else if (sd->aspect == EMOTION_ASPECT_KEEP_HEIGHT)
     {
	aspect_opt = 2;
     }
   else if (sd->aspect == EMOTION_ASPECT_KEEP_BOTH)
     {
	if (ir > r)
	  aspect_opt = 1;
	else
	  aspect_opt = 2;
     }
   else if (sd->aspect == EMOTION_ASPECT_CROP)
     {
	if (ir > r)
	  aspect_opt = 2;
	else
	  aspect_opt = 1;
     }
   else if (sd->aspect == EMOTION_ASPECT_CUSTOM)
     {
	// nothing to do, just respect the border settings
	aspect_opt = 0;
     }

   /* updating borders based on keep_aspect settings */
   if (aspect_opt == 1) // keep width
     {
	int th, dh;
	double scale;

	sd->crop.l = 0;
	sd->crop.r = 0;
	scale = (double)iw / w;
	th = h * scale;
	dh = ih - th;
	sd->crop.t = sd->crop.b = dh / 2;
     }
   else if (aspect_opt == 2) // keep height
     {
	int tw, dw;
	double scale;

	sd->crop.t = 0;
	sd->crop.b = 0;
	scale = (double)ih / h;
	tw = w * scale;
	dw = iw - tw;
	sd->crop.l = sd->crop.r = dw / 2;
     }

   _emotion_aspect_borders_apply(obj, sd, w, h, iw, ih);
}

EAPI void
emotion_object_border_set(Evas_Object *obj, int l, int r, int t, int b)
{
   Smart_Data *sd;
   int w, h;

   E_SMART_OBJ_GET(sd, obj, E_OBJ_NAME);

   sd->aspect = EMOTION_ASPECT_CUSTOM;
   sd->crop.l = -l;
   sd->crop.r = -r;
   sd->crop.t = -t;
   sd->crop.b = -b;
   evas_object_geometry_get(obj, NULL, NULL, &w, &h);
   _emotion_object_aspect_border_apply(obj, sd, w, h);
}

EAPI void
emotion_object_border_get(const Evas_Object *obj, int *l, int *r, int *t, int *b)
{
   Smart_Data *sd;

   E_SMART_OBJ_GET(sd, obj, E_OBJ_NAME);
   *l = -sd->crop.l;
   *r = -sd->crop.r;
   *t = -sd->crop.t;
   *b = -sd->crop.b;
}

EAPI void
emotion_object_bg_color_set(Evas_Object *obj, int r, int g, int b, int a)
{
   Smart_Data *sd;

   E_SMART_OBJ_GET(sd, obj, E_OBJ_NAME);

   evas_object_color_set(sd->bg, r, g, b, a);

   if (!evas_object_visible_get(obj))
     return;

   if (a > 0)
     evas_object_show(sd->bg);
   else
     evas_object_hide(sd->bg);
}

EAPI void
emotion_object_bg_color_get(const Evas_Object *obj, int *r, int *g, int *b, int *a)
{
   Smart_Data *sd;

   E_SMART_OBJ_GET(sd, obj, E_OBJ_NAME);
   evas_object_color_get(sd->bg, r, g, b, a);
}

EAPI void
emotion_object_keep_aspect_set(Evas_Object *obj, Emotion_Aspect a)
{
   Smart_Data *sd;
   int w, h;

   E_SMART_OBJ_GET(sd, obj, E_OBJ_NAME);

   if (a == sd->aspect)
     return;

   sd->aspect = a;
   evas_object_geometry_get(obj, NULL, NULL, &w, &h);
   _emotion_object_aspect_border_apply(obj, sd, w, h);
}

EAPI Emotion_Aspect
emotion_object_keep_aspect_get(const Evas_Object *obj)
{
   Smart_Data *sd;

   E_SMART_OBJ_GET_RETURN(sd, obj, E_OBJ_NAME, EMOTION_ASPECT_KEEP_NONE);

   return sd->aspect;
}

EAPI void
emotion_object_play_set(Evas_Object *obj, Eina_Bool play)
{
   Smart_Data *sd;

   E_SMART_OBJ_GET(sd, obj, E_OBJ_NAME);
   DBG("play=" FMT_UCHAR ", was=" FMT_UCHAR, play, sd->play);
   if (play == sd->play) return;
   if (!sd->module) return;
   if (!sd->video_data) return;
   if (!sd->open)
     {
        sd->remember_play = play;
        return;
     }
   sd->play = play;
   sd->remember_play = play;
   if (sd->state != EMOTION_WAKEUP) emotion_object_suspend_set(obj, EMOTION_WAKEUP);
   if (sd->play) sd->module->play(sd->video_data, sd->pos);
   else sd->module->stop(sd->video_data);
}

EAPI Eina_Bool
emotion_object_play_get(const Evas_Object *obj)
{
   Smart_Data *sd;

   E_SMART_OBJ_GET_RETURN(sd, obj, E_OBJ_NAME, 0);
   if (!sd->video_data) return EINA_FALSE;

   return sd->play;
}

EAPI void
emotion_object_position_set(Evas_Object *obj, double sec)
{
   Smart_Data *sd;

   E_SMART_OBJ_GET(sd, obj, E_OBJ_NAME);
   DBG("sec=%f", sec);
   if (!sd->module) return;
   if (!sd->video_data) return;
   if (!sd->open)
     {
        sd->remember_jump = sec;
        return ;
     }
   sd->remember_jump = 0;
   sd->seek_pos = sec;
   sd->seek = 1;
   sd->pos = sd->seek_pos;
   if (sd->job) ecore_job_del(sd->job);
   sd->job = ecore_job_add(_pos_set_job, obj);
}

EAPI double
emotion_object_position_get(const Evas_Object *obj)
{
   Smart_Data *sd;

   E_SMART_OBJ_GET_RETURN(sd, obj, E_OBJ_NAME, 0.0);
   if (!sd->module) return 0.0;
   if (!sd->video_data) return 0.0;
   if (!sd->module->pos_get) return 0.0;
   sd->pos = sd->module->pos_get(sd->video_data);
   return sd->pos;
}

EAPI double
emotion_object_buffer_size_get(const Evas_Object *obj)
{
   Smart_Data *sd;

   E_SMART_OBJ_GET_RETURN(sd, obj, E_OBJ_NAME, 1.0);
   if (!sd->module) return 1.0;
   if (!sd->video_data) return 1.0;
   if (!sd->module->buffer_size_get) return 1.0;
   return sd->module->buffer_size_get(sd->video_data);
}

EAPI Eina_Bool
emotion_object_seekable_get(const Evas_Object *obj)
{
   Smart_Data *sd;

   E_SMART_OBJ_GET_RETURN(sd, obj, E_OBJ_NAME, 0);
   if (!sd->module) return EINA_FALSE;
   if (!sd->video_data) return EINA_FALSE;
   return sd->module->seekable(sd->video_data);
}

EAPI Eina_Bool
emotion_object_video_handled_get(const Evas_Object *obj)
{
   Smart_Data *sd;

   E_SMART_OBJ_GET_RETURN(sd, obj, E_OBJ_NAME, 0);
   if (!sd->module) return EINA_FALSE;
   if (!sd->video_data) return EINA_FALSE;
   return sd->module->video_handled(sd->video_data);
}

EAPI Eina_Bool
emotion_object_audio_handled_get(const Evas_Object *obj)
{
   Smart_Data *sd;

   E_SMART_OBJ_GET_RETURN(sd, obj, E_OBJ_NAME, 0);
   if (!sd->module) return EINA_FALSE;
   if (!sd->video_data) return EINA_FALSE;
   return sd->module->audio_handled(sd->video_data);
}

EAPI double
emotion_object_play_length_get(const Evas_Object *obj)
{
   Smart_Data *sd;

   E_SMART_OBJ_GET_RETURN(sd, obj, E_OBJ_NAME, 0.0);
   if (!sd->module) return 0.0;
   if (!sd->video_data) return 0.0;
   sd->len = sd->module->len_get(sd->video_data);
   return sd->len;
}

EAPI void
emotion_object_size_get(const Evas_Object *obj, int *iw, int *ih)
{
   Smart_Data *sd;

   if (iw) *iw = 0;
   if (ih) *ih = 0;
   E_SMART_OBJ_GET(sd, obj, E_OBJ_NAME);
   if (iw) *iw = sd->video.w;
   if (ih) *ih = sd->video.h;
}

EAPI void
emotion_object_smooth_scale_set(Evas_Object *obj, Eina_Bool smooth)
{
   Smart_Data *sd;

   E_SMART_OBJ_GET(sd, obj, E_OBJ_NAME);
   evas_object_image_smooth_scale_set(sd->obj, smooth);
}

EAPI Eina_Bool
emotion_object_smooth_scale_get(const Evas_Object *obj)
{
   Smart_Data *sd;

   E_SMART_OBJ_GET_RETURN(sd, obj, E_OBJ_NAME, 0);
   return evas_object_image_smooth_scale_get(sd->obj);
}

EAPI double
emotion_object_ratio_get(const Evas_Object *obj)
{
   Smart_Data *sd;

   E_SMART_OBJ_GET_RETURN(sd, obj, E_OBJ_NAME, 1.0);
   if (!sd->module) return 0.0;
   if (!sd->video_data) return 0.0;
   return sd->ratio;
}

/*
 * Send a control event to the DVD.
 */
EAPI void
emotion_object_event_simple_send(Evas_Object *obj, Emotion_Event ev)
{
   Smart_Data *sd;

   E_SMART_OBJ_GET(sd, obj, E_OBJ_NAME);
   if (!sd->module) return;
   if (!sd->video_data) return;
   sd->module->event_feed(sd->video_data, ev);
}

EAPI void
emotion_object_audio_volume_set(Evas_Object *obj, double vol)
{
   Smart_Data *sd;

   E_SMART_OBJ_GET(sd, obj, E_OBJ_NAME);
   DBG("vol=%f", vol);
   if (!sd->module) return;
   if (!sd->video_data) return;
   sd->module->audio_channel_volume_set(sd->video_data, vol);
}

EAPI double
emotion_object_audio_volume_get(const Evas_Object *obj)
{
   Smart_Data *sd;

   E_SMART_OBJ_GET_RETURN(sd, obj, E_OBJ_NAME, 1.0);
   if (!sd->module) return 0.0;
   if (!sd->video_data) return 0.0;
   return sd->module->audio_channel_volume_get(sd->video_data);
}

EAPI void
emotion_object_audio_mute_set(Evas_Object *obj, Eina_Bool mute)
{
   Smart_Data *sd;

   E_SMART_OBJ_GET(sd, obj, E_OBJ_NAME);
   DBG("mute=" FMT_UCHAR, mute);
   if (!sd->module) return;
   if (!sd->video_data) return;
   sd->module->audio_channel_mute_set(sd->video_data, mute);
}

EAPI Eina_Bool
emotion_object_audio_mute_get(const Evas_Object *obj)
{
   Smart_Data *sd;

   E_SMART_OBJ_GET_RETURN(sd, obj, E_OBJ_NAME, 0);
   if (!sd->module) return EINA_FALSE;
   if (!sd->video_data) return EINA_FALSE;
   return sd->module->audio_channel_mute_get(sd->video_data);
}

EAPI int
emotion_object_audio_channel_count(const Evas_Object *obj)
{
   Smart_Data *sd;

   E_SMART_OBJ_GET_RETURN(sd, obj, E_OBJ_NAME, 0);
   if (!sd->module) return 0;
   if (!sd->video_data) return 0;
   return sd->module->audio_channel_count(sd->video_data);
}

EAPI const char *
emotion_object_audio_channel_name_get(const Evas_Object *obj, int channel)
{
   Smart_Data *sd;

   E_SMART_OBJ_GET_RETURN(sd, obj, E_OBJ_NAME, NULL);
   if (!sd->module) return NULL;
   if (!sd->video_data) return NULL;
   return sd->module->audio_channel_name_get(sd->video_data, channel);
}

EAPI void
emotion_object_audio_channel_set(Evas_Object *obj, int channel)
{
   Smart_Data *sd;

   E_SMART_OBJ_GET(sd, obj, E_OBJ_NAME);
   DBG("channel=%d", channel);
   if (!sd->module) return;
   if (!sd->video_data) return;
   sd->module->audio_channel_set(sd->video_data, channel);
}

EAPI int
emotion_object_audio_channel_get(const Evas_Object *obj)
{
   Smart_Data *sd;

   E_SMART_OBJ_GET_RETURN(sd, obj, E_OBJ_NAME, 0);
   if (!sd->module) return 0;
   if (!sd->video_data) return 0;
   return sd->module->audio_channel_get(sd->video_data);
}

EAPI void
emotion_object_video_mute_set(Evas_Object *obj, Eina_Bool mute)
{
   Smart_Data *sd;

   E_SMART_OBJ_GET(sd, obj, E_OBJ_NAME);
   DBG("mute=" FMT_UCHAR, mute);
   if (!sd->module) return;
   if (!sd->video_data) return;
   sd->module->video_channel_mute_set(sd->video_data, mute);
}

EAPI Eina_Bool
emotion_object_video_mute_get(const Evas_Object *obj)
{
   Smart_Data *sd;

   E_SMART_OBJ_GET_RETURN(sd, obj, E_OBJ_NAME, 0);
   if (!sd->module) return EINA_FALSE;
   if (!sd->video_data) return EINA_FALSE;
   return sd->module->video_channel_mute_get(sd->video_data);
}

EAPI int
emotion_object_video_channel_count(const Evas_Object *obj)
{
   Smart_Data *sd;

   E_SMART_OBJ_GET_RETURN(sd, obj, E_OBJ_NAME, 0);
   if (!sd->module) return EINA_FALSE;
   if (!sd->video_data) return EINA_FALSE;
   return sd->module->video_channel_count(sd->video_data);
}

EAPI const char *
emotion_object_video_channel_name_get(const Evas_Object *obj, int channel)
{
   Smart_Data *sd;

   E_SMART_OBJ_GET_RETURN(sd, obj, E_OBJ_NAME, NULL);
   if (!sd->module) return NULL;
   if (!sd->video_data) return NULL;
   return sd->module->video_channel_name_get(sd->video_data, channel);
}

EAPI void
emotion_object_video_channel_set(Evas_Object *obj, int channel)
{
   Smart_Data *sd;

   E_SMART_OBJ_GET(sd, obj, E_OBJ_NAME);
   DBG("channel=%d", channel);
   if (!sd->module) return;
   if (!sd->video_data) return;
   sd->module->video_channel_set(sd->video_data, channel);
}

EAPI int
emotion_object_video_channel_get(const Evas_Object *obj)
{
   Smart_Data *sd;

   E_SMART_OBJ_GET_RETURN(sd, obj, E_OBJ_NAME, 0);
   if (!sd->module) return 0;
   if (!sd->video_data) return 0;
   return sd->module->video_channel_get(sd->video_data);
}

EAPI void
emotion_object_spu_mute_set(Evas_Object *obj, Eina_Bool mute)
{
   Smart_Data *sd;

   E_SMART_OBJ_GET(sd, obj, E_OBJ_NAME);
   DBG("mute=" FMT_UCHAR, mute);
   if (!sd->module) return;
   if (!sd->video_data) return;
   sd->module->spu_channel_mute_set(sd->video_data, mute);
}

EAPI Eina_Bool
emotion_object_spu_mute_get(const Evas_Object *obj)
{
   Smart_Data *sd;

   E_SMART_OBJ_GET_RETURN(sd, obj, E_OBJ_NAME, 0);
   if (!sd->module) return EINA_FALSE;
   if (!sd->video_data) return EINA_FALSE;
   return sd->module->spu_channel_mute_get(sd->video_data);
}

EAPI int
emotion_object_spu_channel_count(const Evas_Object *obj)
{
   Smart_Data *sd;

   E_SMART_OBJ_GET_RETURN(sd, obj, E_OBJ_NAME, 0);
   if (!sd->module) return 0;
   if (!sd->video_data) return 0;
   return sd->module->spu_channel_count(sd->video_data);
}

EAPI const char *
emotion_object_spu_channel_name_get(const Evas_Object *obj, int channel)
{
   Smart_Data *sd;

   E_SMART_OBJ_GET_RETURN(sd, obj, E_OBJ_NAME, NULL);
   if (!sd->module) return NULL;
   if (!sd->video_data) return NULL;
   return sd->module->spu_channel_name_get(sd->video_data, channel);
}

EAPI void
emotion_object_spu_channel_set(Evas_Object *obj, int channel)
{
   Smart_Data *sd;

   E_SMART_OBJ_GET(sd, obj, E_OBJ_NAME);
   DBG("channel=%d", channel);
   if (!sd->module) return;
   if (!sd->video_data) return;
   sd->module->spu_channel_set(sd->video_data, channel);
}

EAPI int
emotion_object_spu_channel_get(const Evas_Object *obj)
{
   Smart_Data *sd;

   E_SMART_OBJ_GET_RETURN(sd, obj, E_OBJ_NAME, 0);
   if (!sd->module) return 0;
   if (!sd->video_data) return 0;
   return sd->module->spu_channel_get(sd->video_data);
}

EAPI int
emotion_object_chapter_count(const Evas_Object *obj)
{
   Smart_Data *sd;

   E_SMART_OBJ_GET_RETURN(sd, obj, E_OBJ_NAME, 0);
   if (!sd->module) return 0;
   if (!sd->video_data) return 0;
   return sd->module->chapter_count(sd->video_data);
}

EAPI void
emotion_object_chapter_set(Evas_Object *obj, int chapter)
{
   Smart_Data *sd;

   E_SMART_OBJ_GET(sd, obj, E_OBJ_NAME);
   DBG("chapter=%d", chapter);
   if (!sd->module) return;
   if (!sd->video_data) return;
   sd->module->chapter_set(sd->video_data, chapter);
}

EAPI int
emotion_object_chapter_get(const Evas_Object *obj)
{
   Smart_Data *sd;

   E_SMART_OBJ_GET_RETURN(sd, obj, E_OBJ_NAME, 0);
   if (!sd->module) return 0;
   if (!sd->video_data) return 0;
   return sd->module->chapter_get(sd->video_data);
}

EAPI const char *
emotion_object_chapter_name_get(const Evas_Object *obj, int chapter)
{
   Smart_Data *sd;

   E_SMART_OBJ_GET_RETURN(sd, obj, E_OBJ_NAME, NULL);
   if (!sd->module) return NULL;
   if (!sd->video_data) return NULL;
   return sd->module->chapter_name_get(sd->video_data, chapter);
}

EAPI void
emotion_object_play_speed_set(Evas_Object *obj, double speed)
{
   Smart_Data *sd;

   E_SMART_OBJ_GET(sd, obj, E_OBJ_NAME);
   DBG("speed=%f", speed);
   if (!sd->module) return;
   if (!sd->video_data) return;
   sd->module->speed_set(sd->video_data, speed);
}

EAPI double
emotion_object_play_speed_get(const Evas_Object *obj)
{
   Smart_Data *sd;

   E_SMART_OBJ_GET_RETURN(sd, obj, E_OBJ_NAME, 0.0);
   if (!sd->module) return 0.0;
   if (!sd->video_data) return 0.0;
   return sd->module->speed_get(sd->video_data);
}

EAPI void
emotion_object_eject(Evas_Object *obj)
{
   Smart_Data *sd;

   E_SMART_OBJ_GET(sd, obj, E_OBJ_NAME);
   if (!sd->module) return;
   if (!sd->video_data) return;
   sd->module->eject(sd->video_data);
}

EAPI const char *
emotion_object_title_get(const Evas_Object *obj)
{
   Smart_Data *sd;

   E_SMART_OBJ_GET_RETURN(sd, obj, E_OBJ_NAME, NULL);
   return sd->title;
}

EAPI const char *
emotion_object_progress_info_get(const Evas_Object *obj)
{
   Smart_Data *sd;

   E_SMART_OBJ_GET_RETURN(sd, obj, E_OBJ_NAME, NULL);
   return sd->progress.info;
}

EAPI double
emotion_object_progress_status_get(const Evas_Object *obj)
{
   Smart_Data *sd;

   E_SMART_OBJ_GET_RETURN(sd, obj, E_OBJ_NAME, 0.0);
   return sd->progress.stat;
}

EAPI const char *
emotion_object_ref_file_get(const Evas_Object *obj)
{
   Smart_Data *sd;

   E_SMART_OBJ_GET_RETURN(sd, obj, E_OBJ_NAME, NULL);
   return sd->ref.file;
}

EAPI int
emotion_object_ref_num_get(const Evas_Object *obj)
{
   Smart_Data *sd;

   E_SMART_OBJ_GET_RETURN(sd, obj, E_OBJ_NAME, 0);
   return sd->ref.num;
}

EAPI int
emotion_object_spu_button_count_get(const Evas_Object *obj)
{
   Smart_Data *sd;

   E_SMART_OBJ_GET_RETURN(sd, obj, E_OBJ_NAME, 0);
   return sd->spu.button_num;
}

EAPI int
emotion_object_spu_button_get(const Evas_Object *obj)
{
   Smart_Data *sd;

   E_SMART_OBJ_GET_RETURN(sd, obj, E_OBJ_NAME, 0);
   return sd->spu.button;
}

EAPI const char *
emotion_object_meta_info_get(const Evas_Object *obj, Emotion_Meta_Info meta)
{
   Smart_Data *sd;

   E_SMART_OBJ_GET_RETURN(sd, obj, E_OBJ_NAME, NULL);
   if (!sd->module) return NULL;
   if (!sd->video_data) return NULL;
   switch (meta)
     {
      case EMOTION_META_INFO_TRACK_TITLE:
	return sd->module->meta_get(sd->video_data, META_TRACK_TITLE);
      case EMOTION_META_INFO_TRACK_ARTIST:
	return sd->module->meta_get(sd->video_data, META_TRACK_ARTIST);
      case EMOTION_META_INFO_TRACK_ALBUM:
	return sd->module->meta_get(sd->video_data, META_TRACK_ALBUM);
      case EMOTION_META_INFO_TRACK_YEAR:
	return sd->module->meta_get(sd->video_data, META_TRACK_YEAR);
      case EMOTION_META_INFO_TRACK_GENRE:
	return sd->module->meta_get(sd->video_data, META_TRACK_GENRE);
      case EMOTION_META_INFO_TRACK_COMMENT:
	return sd->module->meta_get(sd->video_data, META_TRACK_COMMENT);
      case EMOTION_META_INFO_TRACK_DISC_ID:
	return sd->module->meta_get(sd->video_data, META_TRACK_DISCID);
      default:
	break;
     }
   return NULL;
}

EAPI void
emotion_object_vis_set(Evas_Object *obj, Emotion_Vis visualization)
{
   Smart_Data *sd;

   E_SMART_OBJ_GET(sd, obj, E_OBJ_NAME);
   DBG("visualization=%d", visualization);
   if (!sd->module) return;
   if (!sd->video_data) return;
   if (!sd->module->vis_set) return;
   sd->module->vis_set(sd->video_data, visualization);
}

EAPI Emotion_Vis
emotion_object_vis_get(const Evas_Object *obj)
{
   Smart_Data *sd;

   E_SMART_OBJ_GET_RETURN(sd, obj, E_OBJ_NAME, EMOTION_VIS_NONE);
   if (!sd->module) return EMOTION_VIS_NONE;
   if (!sd->video_data) return EMOTION_VIS_NONE;
   if (!sd->module->vis_get) return EMOTION_VIS_NONE;
   return sd->module->vis_get(sd->video_data);
}

EAPI Eina_Bool
emotion_object_vis_supported(const Evas_Object *obj, Emotion_Vis visualization)
{
   Smart_Data *sd;

   E_SMART_OBJ_GET_RETURN(sd, obj, E_OBJ_NAME, 0);
   if (!sd->module) return EINA_FALSE;
   if (!sd->video_data) return EINA_FALSE;
   if (!sd->module->vis_supported) return EINA_FALSE;
   return sd->module->vis_supported(sd->video_data, visualization);
}

EAPI void
emotion_object_priority_set(Evas_Object *obj, Eina_Bool priority)
{
   Smart_Data *sd;

   E_SMART_OBJ_GET(sd, obj, E_OBJ_NAME);
   if (!sd->module) return ;
   if (!sd->video_data) return ;
   if (!sd->module->priority_set) return ;
   sd->module->priority_set(sd->video_data, priority);
}

EAPI Eina_Bool
emotion_object_priority_get(const Evas_Object *obj)
{
   Smart_Data *sd;

   E_SMART_OBJ_GET_RETURN(sd, obj, E_OBJ_NAME, 0);
   if (!sd->module) return EINA_FALSE;
   if (!sd->video_data) return EINA_FALSE;
   if (!sd->module->priority_get) return EINA_FALSE;
   return sd->module->priority_get(sd->video_data);
}

#ifdef HAVE_EIO
static void
_eio_load_xattr_cleanup(Smart_Data *sd, Eio_File *handler)
{
   if (handler == sd->load_xattr) sd->load_xattr = NULL;

   EINA_REFCOUNT_UNREF(sd)
     _smart_data_free(sd);
}

static void
_eio_load_xattr_done(void *data, Eio_File *handler, double xattr_double)
{
   Smart_Data *sd = data;

   emotion_object_position_set(evas_object_smart_parent_get(sd->obj), xattr_double);
   evas_object_smart_callback_call(evas_object_smart_parent_get(sd->obj), SIG_POSITION_LOAD_SUCCEED, NULL);
   _eio_load_xattr_cleanup(sd, handler);
}

static void
_eio_load_xattr_error(void *data, Eio_File *handler, int err __UNUSED__)
{
   Smart_Data *sd = data;

   evas_object_smart_callback_call(evas_object_smart_parent_get(sd->obj), SIG_POSITION_LOAD_FAILED, NULL);
   _eio_load_xattr_cleanup(sd, handler);
}
#endif

EAPI void
emotion_object_last_position_load(Evas_Object *obj)
{
   Smart_Data *sd;
   const char *tmp;
#ifndef HAVE_EIO
   double xattr;
#endif

   E_SMART_OBJ_GET(sd, obj, E_OBJ_NAME);
   if (!sd->file) return ;

   if (!strncmp(sd->file, "file://", 7))
     tmp = sd->file + 7;
   else if (!strstr(sd->file, "://"))
     tmp = sd->file;
   else
     return ;

#ifdef HAVE_EIO
   if (sd->load_xattr) return ;

   EINA_REFCOUNT_REF(sd);

   sd->load_xattr = eio_file_xattr_double_get(tmp,
					      "user.e.time_seek",
					      _eio_load_xattr_done,
					      _eio_load_xattr_error,
					      sd);
#else
   if (eina_xattr_double_get(tmp, "user.e.time_seek", &xattr))
     {
        emotion_object_position_set(obj, xattr);
        evas_object_smart_callback_call(obj, SIG_POSITION_LOAD_SUCCEED, NULL);
     }
   else
     {
        evas_object_smart_callback_call(obj, SIG_POSITION_LOAD_FAILED, NULL);
     }
#endif
}

#ifdef HAVE_EIO
static void
_eio_save_xattr_done(void *data, Eio_File *handler __UNUSED__)
{
   Smart_Data *sd = data;

   evas_object_smart_callback_call(sd->obj, SIG_POSITION_SAVE_SUCCEED, NULL);
}

static void
_eio_save_xattr_error(void *data, Eio_File *handler __UNUSED__, int err __UNUSED__)
{
   Smart_Data *sd = data;

   evas_object_smart_callback_call(sd->obj, SIG_POSITION_SAVE_FAILED, NULL);
}
#endif

EAPI void
emotion_object_last_position_save(Evas_Object *obj)
{
   Smart_Data *sd;
   const char *tmp;

   E_SMART_OBJ_GET(sd, obj, E_OBJ_NAME);
   if (!sd->file) return ;

   if (!strncmp(sd->file, "file://", 7))
     tmp = sd->file + 7;
   else if (!strstr(sd->file, "://"))
     tmp = sd->file;
   else
     return ;

#ifdef HAVE_EIO
   eio_file_xattr_double_set(tmp, "user.e.time_seek", emotion_object_position_get(obj), 0,
			     _eio_save_xattr_done, _eio_save_xattr_error, sd);
#else
   if (eina_xattr_double_set(tmp, "user.e.time_seek", emotion_object_position_get(obj), 0))
     evas_object_smart_callback_call(obj, SIG_POSITION_SAVE_SUCCEED, NULL);
   else
     evas_object_smart_callback_call(obj, SIG_POSITION_SAVE_FAILED, NULL);
#endif
}

EAPI void
emotion_object_suspend_set(Evas_Object *obj, Emotion_Suspend state)
{
   Smart_Data *sd;

   E_SMART_OBJ_GET(sd, obj, E_OBJ_NAME);
   switch (state)
     {
      case EMOTION_WAKEUP:
         /* Restore the rendering pipeline, offset and everything back to play again (this will be called automatically by play_set) */
      case EMOTION_SLEEP:
         /* This destroy some part of the rendering pipeline */
      case EMOTION_DEEP_SLEEP:
         /* This destroy all the rendering pipeline and just keep the last rendered image (fullscreen) */
      case EMOTION_HIBERNATE:
         /* This destroy all the rendering pipeline and keep 1/4 of the last rendered image */
      default:
         break;
     }

   sd->state = state;
}

EAPI Emotion_Suspend
motion_object_suspend_get(Evas_Object *obj)
{
   Smart_Data *sd;

   E_SMART_OBJ_GET_RETURN(sd, obj, E_OBJ_NAME, EMOTION_WAKEUP);
   return sd->state;
}

/*****************************/
/* Utility calls for modules */
/*****************************/

EAPI void *
_emotion_video_get(const Evas_Object *obj)
{
   Smart_Data *sd;

   E_SMART_OBJ_GET_RETURN(sd, obj, E_OBJ_NAME, NULL);
   return sd->video_data;
}

static Eina_Bool
_emotion_frame_anim(void *data)
{
   Evas_Object *obj = data;
   Smart_Data *sd;

   E_SMART_OBJ_GET_RETURN(sd, obj, E_OBJ_NAME, EINA_FALSE);

   evas_object_image_pixels_dirty_set(sd->obj, 1);
   evas_object_smart_callback_call(obj, SIG_FRAME_DECODE, NULL);
   sd->anim = NULL;

   return EINA_FALSE;
}

EAPI void
_emotion_frame_new(Evas_Object *obj)
{
   Smart_Data *sd;

   E_SMART_OBJ_GET(sd, obj, E_OBJ_NAME);

   if (!sd->anim) sd->anim = ecore_animator_add(_emotion_frame_anim, obj);
}

EAPI void
_emotion_video_pos_update(Evas_Object *obj, double pos, double len)
{
   Smart_Data *sd;
   int npos = 0, nlen = 0;

   E_SMART_OBJ_GET(sd, obj, E_OBJ_NAME);
   if (pos != sd->pos) npos = 1;
   if (len != sd->len) nlen = 1;
   sd->pos = pos;
   sd->len = len;
   if (npos) evas_object_smart_callback_call(obj, SIG_POSITION_UPDATE, NULL);
   if (nlen) evas_object_smart_callback_call(obj, SIG_LENGTH_CHANGE, NULL);
}

EAPI void
_emotion_frame_resize(Evas_Object *obj, int w, int h, double ratio)
{
   Smart_Data *sd;
   double tmp;
   int changed = 0;

   E_SMART_OBJ_GET(sd, obj, E_OBJ_NAME);
   if ((w != sd->video.w) || (h != sd->video.h))
     {
        sd->video.w = w;
        sd->video.h = h;
        _emotion_image_data_zero(sd->obj);
	changed = 1;
     }
   if (h > 0) tmp  = (double)w / (double)h;
   else tmp = 1.0;
   if (ratio != tmp) tmp = ratio;
   if (tmp != sd->ratio)
     {
	sd->ratio = tmp;
	changed = 1;
     }
   if (changed)
     {
	evas_object_size_hint_request_set(obj, w, h);
	evas_object_smart_callback_call(obj, SIG_FRAME_RESIZE, NULL);
	evas_object_geometry_get(obj, NULL, NULL, &w, &h);
	_emotion_object_aspect_border_apply(obj, sd, w, h);
     }
}

EAPI void
_emotion_decode_stop(Evas_Object *obj)
{
   Smart_Data *sd;

   E_SMART_OBJ_GET(sd, obj, E_OBJ_NAME);
   if (sd->play)
     {
	sd->play = 0;
	evas_object_smart_callback_call(obj, SIG_DECODE_STOP, NULL);
     }
}

EAPI void
_emotion_open_done(Evas_Object *obj)
{
   Smart_Data *sd;

   E_SMART_OBJ_GET(sd, obj, E_OBJ_NAME);
   sd->open = 1;

   if (sd->remember_jump)
     emotion_object_position_set(obj, sd->remember_jump);
   if (sd->remember_play != sd->play)
     emotion_object_play_set(obj, sd->remember_play);
   evas_object_smart_callback_call(obj, SIG_OPEN_DONE, NULL);
}

EAPI void
_emotion_playback_started(Evas_Object *obj)
{
   evas_object_smart_callback_call(obj, SIG_PLAYBACK_STARTED, NULL);
}

EAPI void
_emotion_playback_finished(Evas_Object *obj)
{
   evas_object_smart_callback_call(obj, SIG_PLAYBACK_FINISHED, NULL);
}

EAPI void
_emotion_audio_level_change(Evas_Object *obj)
{
   evas_object_smart_callback_call(obj, SIG_AUDIO_LEVEL_CHANGE, NULL);
}

EAPI void
_emotion_channels_change(Evas_Object *obj)
{
   Smart_Data *sd;

   E_SMART_OBJ_GET(sd, obj, E_OBJ_NAME);
   evas_object_smart_callback_call(obj, SIG_CHANNELS_CHANGE, NULL);
}

EAPI void
_emotion_title_set(Evas_Object *obj, char *title)
{
   Smart_Data *sd;

   E_SMART_OBJ_GET(sd, obj, E_OBJ_NAME);
   free(sd->title);
   sd->title = strdup(title);
   evas_object_smart_callback_call(obj, SIG_TITLE_CHANGE, NULL);
}

EAPI void
_emotion_progress_set(Evas_Object *obj, char *info, double st)
{
   Smart_Data *sd;

   E_SMART_OBJ_GET(sd, obj, E_OBJ_NAME);
   free(sd->progress.info);
   sd->progress.info = strdup(info);
   sd->progress.stat = st;
   evas_object_smart_callback_call(obj, SIG_PROGRESS_CHANGE, NULL);
}

EAPI void
_emotion_file_ref_set(Evas_Object *obj, const char *file, int num)
{
   Smart_Data *sd;

   E_SMART_OBJ_GET(sd, obj, E_OBJ_NAME);
   free(sd->ref.file);
   sd->ref.file = strdup(file);
   sd->ref.num = num;
   evas_object_smart_callback_call(obj, SIG_REF_CHANGE, NULL);
}

EAPI void
_emotion_spu_button_num_set(Evas_Object *obj, int num)
{
   Smart_Data *sd;

   E_SMART_OBJ_GET(sd, obj, E_OBJ_NAME);
   sd->spu.button_num = num;
   evas_object_smart_callback_call(obj, SIG_BUTTON_NUM_CHANGE, NULL);
}

EAPI void
_emotion_spu_button_set(Evas_Object *obj, int button)
{
   Smart_Data *sd;

   E_SMART_OBJ_GET(sd, obj, E_OBJ_NAME);
   sd->spu.button = button;
   evas_object_smart_callback_call(obj, SIG_BUTTON_CHANGE, NULL);
}

EAPI void
_emotion_seek_done(Evas_Object *obj)
{
   Smart_Data *sd;

   E_SMART_OBJ_GET(sd, obj, E_OBJ_NAME);
   if (sd->seeking)
     {
        sd->seeking = 0;
        if (sd->seek) emotion_object_position_set(obj, sd->seek_pos);
     }
}

EAPI void
_emotion_frame_refill(Evas_Object *obj, double w, double h)
{
   Smart_Data *sd;

   E_SMART_OBJ_GET(sd, obj, E_OBJ_NAME);
   if (sd->fill.w != w || sd->fill.h != h)
     {
        Evas_Coord ow, oh;

        evas_object_geometry_get(obj, NULL, NULL, &ow, &oh);
        if (w <= 0 || h <= 0)
          {
             double scale_w, scale_h;

             sd->fill.w = -1;
             sd->fill.h = -1;

             scale_w = (double) ow / (double)(sd->video.w - sd->crop.l - sd->crop.r);
             scale_h = (double) oh / (double)(sd->video.h - sd->crop.t - sd->crop.b);

             evas_object_image_fill_set(sd->obj, 0, 0, scale_w * sd->video.w, scale_h * sd->video.h);
          }
        else
          {
             sd->fill.w = w;
             sd->fill.h = h;

             evas_object_image_fill_set(sd->obj, 0, 0, w * ow, h * oh);
          }
     }
}

/****************************/
/* Internal object routines */
/****************************/

static void
_mouse_move(void *data, Evas *ev __UNUSED__, Evas_Object *obj, void *event_info)
{
   Evas_Event_Mouse_Move *e;
   Smart_Data *sd;
   int x, y, iw, ih;
   Evas_Coord ox, oy, ow, oh;

   e = event_info;
   sd = data;
   if (!sd->module) return;
   if (!sd->video_data) return;
   evas_object_geometry_get(obj, &ox, &oy, &ow, &oh);
   evas_object_image_size_get(obj, &iw, &ih);
   if ((iw < 1) || (ih < 1)) return;
   x = (((int)e->cur.canvas.x - ox) * iw) / ow;
   y = (((int)e->cur.canvas.y - oy) * ih) / oh;
   sd->module->event_mouse_move_feed(sd->video_data, x, y);
}

static void
_mouse_down(void *data, Evas *ev __UNUSED__, Evas_Object *obj, void *event_info)
{
   Evas_Event_Mouse_Down *e;
   Smart_Data *sd;
   int x, y, iw, ih;
   Evas_Coord ox, oy, ow, oh;

   e = event_info;
   sd = data;
   if (!sd->module) return;
   if (!sd->video_data) return;
   evas_object_geometry_get(obj, &ox, &oy, &ow, &oh);
   evas_object_image_size_get(obj, &iw, &ih);
   if ((iw < 1) || (ih < 1)) return;
   x = (((int)e->canvas.x - ox) * iw) / ow;
   y = (((int)e->canvas.y - oy) * ih) / oh;
   sd->module->event_mouse_button_feed(sd->video_data, 1, x, y);
}

static void
_pos_set_job(void *data)
{
   Evas_Object *obj;
   Smart_Data *sd;

   obj = data;
   E_SMART_OBJ_GET(sd, obj, E_OBJ_NAME);
   sd->job = NULL;
   if (sd->seeking) return;
   if (sd->seek)
     {
        sd->seeking = 1;
	sd->module->pos_set(sd->video_data, sd->seek_pos);
	sd->seek = 0;
     }
}

/* called by evas when it needs pixels for the image object */
static void
_pixels_get(void *data, Evas_Object *obj)
{
   int iw, ih, w, h;
   Smart_Data *sd;
   Emotion_Format format;
   unsigned char *bgra_data;

   sd = data;
   sd->module->video_data_size_get(sd->video_data, &w, &h);
   w = (w >> 1) << 1;
   h = (h >> 1) << 1;

   evas_object_image_colorspace_set(obj, EVAS_COLORSPACE_YCBCR422P601_PL);
   evas_object_image_alpha_set(obj, 0);
   evas_object_image_size_set(obj, w, h);
   iw = w;
   ih = h;

   if ((iw <= 1) || (ih <= 1))
     {
        _emotion_image_data_zero(sd->obj);
	evas_object_image_pixels_dirty_set(obj, 0);
     }
   else
     {
	format = sd->module->format_get(sd->video_data);
	if ((format == EMOTION_FORMAT_YV12) || (format == EMOTION_FORMAT_I420))
	  {
	     unsigned char **rows;

	     evas_object_image_colorspace_set(obj, EVAS_COLORSPACE_YCBCR422P601_PL);
	     rows = evas_object_image_data_get(obj, 1);
	     if (rows)
	       {
		  if (sd->module->yuv_rows_get(sd->video_data, iw, ih,
					       rows,
					       &rows[ih],
					       &rows[ih + (ih / 2)]))
		    evas_object_image_data_update_add(obj, 0, 0, iw, ih);
	       }
	     evas_object_image_data_set(obj, rows);
	     evas_object_image_pixels_dirty_set(obj, 0);
	  }
	else if (format == EMOTION_FORMAT_BGRA)
	  {
	     evas_object_image_colorspace_set(obj, EVAS_COLORSPACE_ARGB8888);
	     if (sd->module->bgra_data_get(sd->video_data, &bgra_data))
	       {
		  evas_object_image_data_set(obj, bgra_data);
		  evas_object_image_pixels_dirty_set(obj, 0);
	       }
	  }
     }
}

/*******************************************/
/* Internal smart object required routines */
/*******************************************/
#ifdef EMOTION_STATIC_BUILD_XINE
Eina_Bool xine_module_init(void);
#endif
#ifdef EMOTION_STATIC_BUILD_GSTREAMER
Eina_Bool gstreamer_module_init(void);
#endif
#ifdef EMOTION_STATIC_BUILD_GENERIC
Eina_Bool generic_module_init(void);
#endif

static void
_smart_init(void)
{
   char *path;

   if (smart) return;
     {
	eina_init();

        _log_domain = eina_log_domain_register("emotion", EINA_COLOR_LIGHTCYAN);
        if (_log_domain < 0)
          {
             EINA_LOG_CRIT("Could not register log domain 'emotion'");
             eina_shutdown();
             return;
          }

	_backends = eina_hash_string_small_new(free);

	_modules = eina_module_list_get(NULL, PACKAGE_LIB_DIR "/emotion/", 0, NULL, NULL);

	path = eina_module_environment_path_get("HOME", "/.emotion/");
	_modules = eina_module_list_get(_modules, path, 0, NULL, NULL);
	if (path) free(path);

	path = eina_module_environment_path_get("EMOTION_MODULES_DIR", "/emotion/");
	_modules = eina_module_list_get(_modules, path, 0, NULL, NULL);
	if (path) free(path);

	path = eina_module_symbol_path_get(emotion_object_add, "/emotion/");
	_modules = eina_module_list_get(_modules, path, 0, NULL, NULL);
	if (path) free(path);

	if (!_modules)
	  {
	     ERR("No module found!");
	     return;
	  }

	eina_module_list_load(_modules);

	/* Init static module */
#ifdef EMOTION_STATIC_BUILD_XINE
	xine_module_init();
#endif
#ifdef EMOTION_STATIC_BUILD_GSTREAMER
	gstreamer_module_init();
#endif
#ifdef EMOTION_STATIC_BUILD_GENERIC
	generic_module_init();
#endif

	static Evas_Smart_Class sc =
	  EVAS_SMART_CLASS_INIT_NAME_VERSION(E_OBJ_NAME);
	if (!sc.add)
	  {
	     sc.add = _smart_add;
	     sc.del = _smart_del;
	     sc.move = _smart_move;
	     sc.resize = _smart_resize;
	     sc.show = _smart_show;
	     sc.hide = _smart_hide;
	     sc.color_set = _smart_color_set;
	     sc.clip_set = _smart_clip_set;
	     sc.clip_unset = _smart_clip_unset;
	     sc.callbacks = _smart_callbacks;
	  }
        smart = evas_smart_class_new(&sc);
     }
}

static void
_smart_add(Evas_Object * obj)
{
   Smart_Data *sd;
   unsigned int *pixel;

   sd = calloc(1, sizeof(Smart_Data));
   if (!sd) return;
   EINA_REFCOUNT_INIT(sd);
   sd->state = EMOTION_WAKEUP;
   sd->obj = evas_object_image_add(evas_object_evas_get(obj));
   sd->bg = evas_object_rectangle_add(evas_object_evas_get(obj));
   evas_object_color_set(sd->bg, 0, 0, 0, 0);
   evas_object_event_callback_add(sd->obj, EVAS_CALLBACK_MOUSE_MOVE, _mouse_move, sd);
   evas_object_event_callback_add(sd->obj, EVAS_CALLBACK_MOUSE_DOWN, _mouse_down, sd);
   evas_object_image_pixels_get_callback_set(sd->obj, _pixels_get, sd);
   evas_object_smart_member_add(sd->obj, obj);
   evas_object_smart_member_add(sd->bg, obj);
   evas_object_lower(sd->bg);
   sd->ratio = 1.0;
   sd->spu.button = -1;
   sd->fill.w = -1;
   sd->fill.h = -1;
   evas_object_image_alpha_set(sd->obj, 0);
   pixel = evas_object_image_data_get(sd->obj, 1);
   if (pixel)
     {
	*pixel = 0xff000000;
	evas_object_image_data_set(obj, pixel);
     }
   evas_object_smart_data_set(obj, sd);

   ecore_init();
}

static void
_smart_del(Evas_Object * obj)
{
   Smart_Data *sd;

   sd = evas_object_smart_data_get(obj);
   if (!sd) return;
   EINA_REFCOUNT_UNREF(sd)
     _smart_data_free(sd);
}

static void
_smart_move(Evas_Object * obj, Evas_Coord x, Evas_Coord y)
{
   Smart_Data *sd;
   int w, h;

   sd = evas_object_smart_data_get(obj);
   if (!sd) return;

   evas_object_geometry_get(obj, NULL, NULL, &w, &h);
   _clipper_position_size_update(obj, x, y, w, h, sd->video.w, sd->video.h);
   evas_object_move(sd->bg, x, y);
}

static void
_smart_resize(Evas_Object * obj, Evas_Coord w, Evas_Coord h)
{
   Smart_Data *sd;

   sd = evas_object_smart_data_get(obj);
   if (!sd) return;

   _emotion_object_aspect_border_apply(obj, sd, w, h);
   evas_object_resize(sd->bg, w, h);
}

static void
_smart_show(Evas_Object * obj)
{
   Smart_Data *sd;
   int a;

   sd = evas_object_smart_data_get(obj);
   if (!sd) return;
   evas_object_show(sd->obj);
   if (sd->crop.clipper)
     evas_object_show(sd->crop.clipper);

   evas_object_color_get(sd->bg, NULL, NULL, NULL, &a);
   if (a > 0)
     evas_object_show(sd->bg);
}

static void
_smart_hide(Evas_Object * obj)
{
   Smart_Data *sd;

   sd = evas_object_smart_data_get(obj);
   if (!sd) return;
   evas_object_hide(sd->obj);
   if (sd->crop.clipper)
     evas_object_hide(sd->crop.clipper);
   evas_object_hide(sd->bg);
}

static void
_smart_color_set(Evas_Object * obj, int r, int g, int b, int a)
{
   Smart_Data *sd;

   sd = evas_object_smart_data_get(obj);
   if (!sd) return;
   evas_object_color_set(sd->obj, r, g, b, a);
   evas_object_color_set(sd->crop.clipper, r, g, b, a);
}

static void
_smart_clip_set(Evas_Object * obj, Evas_Object * clip)
{
   Smart_Data *sd;

   sd = evas_object_smart_data_get(obj);
   if (!sd) return;
   if (sd->crop.clipper)
     evas_object_clip_set(sd->crop.clipper, clip);
   else
     evas_object_clip_set(sd->obj, clip);
   evas_object_clip_set(sd->bg, clip);
}

static void
_smart_clip_unset(Evas_Object * obj)
{
   Smart_Data *sd;

   sd = evas_object_smart_data_get(obj);
   if (!sd) return;
   if (sd->crop.clipper)
     evas_object_clip_unset(sd->crop.clipper);
   else
     evas_object_clip_unset(sd->obj);
   evas_object_clip_unset(sd->bg);
}

