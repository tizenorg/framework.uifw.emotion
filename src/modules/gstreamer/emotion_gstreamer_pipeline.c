/*
 * vim:ts=8:sw=3:sts=8:noexpandtab:cino=>5n-3f0^-2{2
 */
#include <unistd.h>
#include <fcntl.h>

#include "emotion_private.h"
#include "emotion_gstreamer.h"
#include "emotion_gstreamer_pipeline.h"


gboolean
emotion_pipeline_pause(GstElement *pipeline)
{
   GstStateChangeReturn res;

   res = gst_element_set_state((pipeline), GST_STATE_PAUSED);
   if (res == GST_STATE_CHANGE_FAILURE)
     {
	g_print("Emotion-Gstreamer ERROR: could not pause\n");
	return 0;
     }

   res = gst_element_get_state((pipeline), NULL, NULL, GST_CLOCK_TIME_NONE);
   if (res != GST_STATE_CHANGE_SUCCESS)
     {
	g_print("Emotion-Gstreamer ERROR: could not complete pause\n");
	return 0;
     }

   return 1;
}

/* Send the video frame to the evas object */
void
cb_handoff(GstElement *fakesrc,
           GstBuffer  *buffer,
           GstPad     *pad,
           gpointer    user_data)
{
   GstQuery *query;
   void *buf[2];

   Emotion_Gstreamer_Video *ev = (Emotion_Gstreamer_Video *)user_data;
   if (!ev)
     return;

   if (!ev->video_mute)
     {
	if (!ev->obj_data)
	  ev->obj_data = malloc(GST_BUFFER_SIZE(buffer) * sizeof(void));

	memcpy(ev->obj_data, GST_BUFFER_DATA(buffer), GST_BUFFER_SIZE(buffer));
	buf[0] = GST_BUFFER_DATA(buffer);
	buf[1] = buffer;
	ecore_pipe_write(ev->pipe, buf, sizeof(buf));
     }
   else
     {
	Emotion_Audio_Sink *asink;
	asink = (Emotion_Audio_Sink *)eina_list_nth(ev->audio_sinks, ev->audio_sink_nbr);
	_emotion_video_pos_update(ev->obj, ev->position, asink->length_time);
     }

   query = gst_query_new_position(GST_FORMAT_TIME);
   if (gst_pad_query(gst_pad_get_peer(pad), query))
     {
	gint64 position;

	gst_query_parse_position(query, NULL, &position);
	ev->position = (double)position / (double)GST_SECOND;
     }
   gst_query_unref(query);
}

void
file_new_decoded_pad_cb(GstElement *decodebin,
                        GstPad     *new_pad,
                        gboolean    last,
                        gpointer    user_data)
{
   Emotion_Gstreamer_Video *ev;
   GstCaps *caps;
   gchar   *str;
   unsigned int index;

   ev = (Emotion_Gstreamer_Video *)user_data;
   caps = gst_pad_get_caps(new_pad);
   str = gst_caps_to_string(caps);
   /* video stream */
   if (g_str_has_prefix(str, "video/"))
     {
	Emotion_Video_Sink *vsink;
	GstElement         *queue;
	GstPad             *videopad;

	vsink = (Emotion_Video_Sink *)calloc(1, sizeof(Emotion_Video_Sink));
	if (!vsink) return;
	ev->video_sinks = eina_list_append(ev->video_sinks, vsink);
	if (eina_error_get())
	  {
	     free(vsink);
	     return;
	  }

	queue = gst_element_factory_make("queue", NULL);
	vsink->sink = gst_element_factory_make("fakesink", "videosink");
	gst_bin_add_many(GST_BIN(ev->pipeline), queue, vsink->sink, NULL);
	gst_element_link(queue, vsink->sink);
	videopad = gst_element_get_pad(queue, "sink");
	gst_pad_link(new_pad, videopad);
	gst_object_unref(videopad);
	if (eina_list_count(ev->video_sinks) == 1)
	  {
	     ev->ratio = (double)vsink->width / (double)vsink->height;
	  }
	gst_element_set_state(queue, GST_STATE_PAUSED);
	gst_element_set_state(vsink->sink, GST_STATE_PAUSED);
     }
   /* audio stream */
   else if (g_str_has_prefix(str, "audio/"))
     {
	Emotion_Audio_Sink *asink;
	GstPad             *audiopad;

	asink = (Emotion_Audio_Sink *)calloc(1, sizeof(Emotion_Audio_Sink));
	if (!asink) return;
	ev->audio_sinks = eina_list_append(ev->audio_sinks, asink);
	if (eina_error_get())
	  {
	     free(asink);
	     return;
	  }

	index = eina_list_count(ev->audio_sinks);
	asink->sink = emotion_audio_sink_create(ev, index);
	gst_bin_add(GST_BIN(ev->pipeline), asink->sink);
	audiopad = gst_element_get_pad(asink->sink, "sink");
	gst_pad_link(new_pad, audiopad);
	gst_element_set_state(asink->sink, GST_STATE_PAUSED);
     }

   free(str);
}

Emotion_Video_Sink *
emotion_video_sink_new(Emotion_Gstreamer_Video *ev)
{
   Emotion_Video_Sink *vsink;

   if (!ev) return NULL;

   vsink = (Emotion_Video_Sink *)calloc(1, sizeof(Emotion_Video_Sink));
   if (!vsink) return NULL;

   ev->video_sinks = eina_list_append(ev->video_sinks, vsink);
   if (eina_error_get())
     {
	free(vsink);
	return NULL;
     }
   return vsink;
}

void
emotion_video_sink_free(Emotion_Gstreamer_Video *ev, Emotion_Video_Sink *vsink)
{
   if (!ev || !vsink) return;

   ev->video_sinks = eina_list_remove(ev->video_sinks, vsink);
	free(vsink);
}

Emotion_Video_Sink *
emotion_visualization_sink_create(Emotion_Gstreamer_Video *ev, Emotion_Audio_Sink *asink)
{
   Emotion_Video_Sink *vsink;

   if (!ev) return NULL;

   vsink = emotion_video_sink_new(ev);
   if (!vsink) return NULL;

   vsink->sink = gst_bin_get_by_name(GST_BIN(asink->sink), "vissink1");
   if (!vsink->sink)
     {
	emotion_video_sink_free(ev, vsink);
	return NULL;
     }
   vsink->width = 320;
   vsink->height = 200;
   ev->ratio = (double)vsink->width / (double)vsink->height;
   vsink->fps_num = 25;
   vsink->fps_den = 1;
   vsink->fourcc = GST_MAKE_FOURCC('A', 'R', 'G', 'B');
   vsink->length_time = asink->length_time;

   g_object_set(G_OBJECT(vsink->sink), "sync", TRUE, NULL);
   g_object_set(G_OBJECT(vsink->sink), "signal-handoffs", TRUE, NULL);
   g_signal_connect(G_OBJECT(vsink->sink),
		    "handoff",
		    G_CALLBACK(cb_handoff), ev);
   return vsink;
}

int
emotion_pipeline_cdda_track_count_get(void *video)
{
   Emotion_Gstreamer_Video *ev;
   GstBus                  *bus;
   guint                    tracks_count = 0;
   gboolean                 done;

   ev = (Emotion_Gstreamer_Video *)video;
   if (!ev) return tracks_count;

   done = FALSE;
   bus = gst_element_get_bus(ev->pipeline);
   if (!bus) return tracks_count;

   while (!done)
     {
	GstMessage *message;

	message = gst_bus_pop(bus);
	if (message == NULL)
	  /* All messages read, we're done */
	  break;

	switch (GST_MESSAGE_TYPE(message))
	  {
	   case GST_MESSAGE_TAG:
		{
		   GstTagList *tags;

		   gst_message_parse_tag(message, &tags);

		   gst_tag_list_get_uint(tags, GST_TAG_TRACK_COUNT, &tracks_count);
		   if (tracks_count) done = TRUE;
		   break;
		}
	   case GST_MESSAGE_ERROR:
	   default:
	      break;
	  }
	gst_message_unref(message);
     }

   gst_object_unref(GST_OBJECT(bus));

   return tracks_count;
}

const char *
emotion_visualization_element_name_get(Emotion_Vis visualisation)
{
   switch (visualisation)
     {
      case EMOTION_VIS_NONE:
	 return NULL;
      case EMOTION_VIS_GOOM:
	 return "goom";
      case EMOTION_VIS_LIBVISUAL_BUMPSCOPE:
	 return "libvisual_bumpscope";
      case EMOTION_VIS_LIBVISUAL_CORONA:
	 return "libvisual_corona";
      case EMOTION_VIS_LIBVISUAL_DANCING_PARTICLES:
	 return "libvisual_dancingparticles";
      case EMOTION_VIS_LIBVISUAL_GDKPIXBUF:
	 return "libvisual_gdkpixbuf";
      case EMOTION_VIS_LIBVISUAL_G_FORCE:
	 return "libvisual_G-Force";
      case EMOTION_VIS_LIBVISUAL_GOOM:
	 return "libvisual_goom";
      case EMOTION_VIS_LIBVISUAL_INFINITE:
	 return "libvisual_infinite";
      case EMOTION_VIS_LIBVISUAL_JAKDAW:
	 return "libvisual_jakdaw";
      case EMOTION_VIS_LIBVISUAL_JESS:
	 return "libvisual_jess";
      case EMOTION_VIS_LIBVISUAL_LV_ANALYSER:
	 return "libvisual_lv_analyzer";
      case EMOTION_VIS_LIBVISUAL_LV_FLOWER:
	 return "libvisual_lv_flower";
      case EMOTION_VIS_LIBVISUAL_LV_GLTEST:
	 return "libvisual_lv_gltest";
      case EMOTION_VIS_LIBVISUAL_LV_SCOPE:
	 return "libvisual_lv_scope";
      case EMOTION_VIS_LIBVISUAL_MADSPIN:
	 return "libvisual_madspin";
      case EMOTION_VIS_LIBVISUAL_NEBULUS:
	 return "libvisual_nebulus";
      case EMOTION_VIS_LIBVISUAL_OINKSIE:
	 return "libvisual_oinksie";
      case EMOTION_VIS_LIBVISUAL_PLASMA:
	 return "libvisual_plazma";
      default:
	 return "goom";
     }
}

static GstElement *
emotion_visualization_bin_create(Emotion_Gstreamer_Video *ev, int index)
{
   const char *vis_name;
   char buf[64];
   GstElement *vis, *visbin, *queue, *conv, *cspace, *sink;
   GstPad *vispad;
   GstCaps *caps;

   if (ev->vis == EMOTION_VIS_NONE)
     return NULL;

   vis_name = emotion_visualization_element_name_get(ev->vis);
   if (!vis_name)
     return NULL;

   g_snprintf(buf, sizeof(buf), "vis%d", index);
   vis = gst_element_factory_make(vis_name, buf);
   if (!vis)
     return NULL;

   g_snprintf(buf, sizeof(buf), "visbin%d", index);
   visbin = gst_bin_new(buf);

   queue = gst_element_factory_make("queue", NULL);
   conv = gst_element_factory_make("audioconvert", NULL);
   cspace = gst_element_factory_make("ffmpegcolorspace", NULL);
   g_snprintf(buf, sizeof(buf), "vissink%d", index);
   sink = gst_element_factory_make("fakesink", buf);

   if ((!visbin) || (!queue) || (!conv) || (!cspace) || (!sink))
     goto error;

   gst_bin_add_many(GST_BIN(visbin), queue, conv, vis, cspace, sink, NULL);
   gst_element_link_many(queue, conv, vis, cspace, NULL);
   caps = gst_caps_new_simple("video/x-raw-rgb",
			      "bpp", G_TYPE_INT, 32,
			      "width", G_TYPE_INT, 320,
			      "height", G_TYPE_INT, 200,
			      NULL);
   gst_element_link_filtered(cspace, sink, caps);

   vispad = gst_element_get_pad(queue, "sink");
   gst_element_add_pad(visbin, gst_ghost_pad_new("sink", vispad));
   gst_object_unref(vispad);

   return visbin;

 error:
   if (vis)
     gst_object_unref(vis);
   if (visbin)
     gst_object_unref(visbin);
   if (queue)
     gst_object_unref(queue);
   if (conv)
     gst_object_unref(conv);
   if (cspace)
     gst_object_unref(cspace);
   if (sink)
     gst_object_unref(sink);

   return NULL;
}

static GstElement *
emotion_audio_bin_create(Emotion_Gstreamer_Video *ev, int index)
{
   GstElement *audiobin, *queue, *conv, *resample, *volume, *sink;
   GstPad *audiopad;
   double vol;

   audiobin = gst_bin_new(NULL);
   queue = gst_element_factory_make("queue", NULL);
   conv = gst_element_factory_make("audioconvert", NULL);
   resample = gst_element_factory_make("audioresample", NULL);
   volume = gst_element_factory_make("volume", "volume");

   if (index == 1)
     sink = gst_element_factory_make("autoaudiosink", NULL);
   else
     /* XXX hack: use a proper mixer element here */
     sink = gst_element_factory_make("fakesink", NULL);

   if ((!audiobin) || (!queue) || (!conv) || (!resample) || (!volume) || (!sink))
     goto error;

   g_object_get(volume, "volume", &vol, NULL);
   ev->volume = vol;

   gst_bin_add_many(GST_BIN(audiobin),
		    queue, conv, resample, volume, sink, NULL);
   gst_element_link_many(queue, conv, resample, volume, sink, NULL);

   audiopad = gst_element_get_pad(queue, "sink");
   gst_element_add_pad(audiobin, gst_ghost_pad_new("sink", audiopad));
   gst_object_unref(audiopad);

   return audiobin;

 error:
   if (audiobin)
     gst_object_unref(audiobin);
   if (queue)
     gst_object_unref(queue);
   if (conv)
     gst_object_unref(conv);
   if (resample)
     gst_object_unref(resample);
   if (volume)
     gst_object_unref(volume);
   if (sink)
     gst_object_unref(sink);

   return NULL;
}


GstElement *
emotion_audio_sink_create(Emotion_Gstreamer_Video *ev, int index)
{
   gchar       buf[128];
   GstElement *bin;
   GstElement *audiobin;
   GstElement *visbin = NULL;
   GstElement *tee;
   GstPad     *teepad;
   GstPad     *binpad;

   audiobin = emotion_audio_bin_create(ev, index);
   if (!audiobin)
     return NULL;

   bin = gst_bin_new(NULL);
   if (!bin)
     {
	gst_object_unref(audiobin);
	return NULL;
     }

   g_snprintf(buf, 128, "tee%d", index);
   tee = gst_element_factory_make("tee", buf);

   visbin = emotion_visualization_bin_create(ev, index);

   gst_bin_add_many(GST_BIN(bin), tee, audiobin, visbin, NULL);

   binpad = gst_element_get_pad(audiobin, "sink");
   teepad = gst_element_get_request_pad(tee, "src%d");
   gst_pad_link(teepad, binpad);
   gst_object_unref(teepad);
   gst_object_unref(binpad);

   if (visbin)
     {
	binpad = gst_element_get_pad(visbin, "sink");
	teepad = gst_element_get_request_pad(tee, "src%d");
	gst_pad_link(teepad, binpad);
	gst_object_unref(teepad);
	gst_object_unref(binpad);
     }

   teepad = gst_element_get_pad(tee, "sink");
   gst_element_add_pad(bin, gst_ghost_pad_new("sink", teepad));
   gst_object_unref(teepad);

   return bin;
}

void
emotion_streams_sinks_get(Emotion_Gstreamer_Video *ev, GstElement *decoder)
{
   GstIterator *it;
   Eina_List   *alist;
   Eina_List   *vlist;
   gpointer     data;

   alist = ev->audio_sinks;
   vlist = ev->video_sinks;

   it = gst_element_iterate_src_pads(decoder);
   while (gst_iterator_next(it, &data) == GST_ITERATOR_OK)
     {
	GstPad  *pad;
	GstCaps *caps;
	gchar   *str;

	pad = GST_PAD(data);

	caps = gst_pad_get_caps(pad);
	str = gst_caps_to_string(caps);
	g_print("caps !! %s\n", str);

	/* video stream */
	if (g_str_has_prefix(str, "video/"))
	  {
	     Emotion_Video_Sink *vsink;

	     vsink = (Emotion_Video_Sink *)eina_list_data_get(vlist);
	     vlist = eina_list_next(vlist);

	     emotion_video_sink_fill(vsink, pad, caps);
	     ev->ratio = (double)vsink->width / (double)vsink->height;

	  }
	/* audio stream */
	else if (g_str_has_prefix(str, "audio/"))
	  {
	     Emotion_Audio_Sink *asink;
	     unsigned int index;

	     asink = (Emotion_Audio_Sink *)eina_list_data_get(alist);
	     alist = eina_list_next(alist);

	     emotion_audio_sink_fill(asink, pad, caps);

	     for (index = 0; asink != eina_list_nth(ev->audio_sinks, index) ; index++)
	       ;

	     if (eina_list_count(ev->video_sinks) == 0)
	       {
		  if (index == 1)
		    {
		       Emotion_Video_Sink *vsink;

		       vsink = emotion_visualization_sink_create(ev, asink);
		       if (!vsink) goto finalize;
		    }
	       }
	     else
	       {
		  gchar       buf[128];
		  GstElement *visbin;

		  g_snprintf(buf, 128, "visbin%d", index);
		  visbin = gst_bin_get_by_name(GST_BIN(ev->pipeline), buf);
		  if (visbin)
		    {
		       GstPad *srcpad;
		       GstPad *sinkpad;

		       sinkpad = gst_element_get_pad(visbin, "sink");
		       srcpad = gst_pad_get_peer(sinkpad);
		       gst_pad_unlink(srcpad, sinkpad);

		       gst_object_unref(srcpad);
		       gst_object_unref(sinkpad);
		    }
	       }
	  }
finalize:
	gst_caps_unref(caps);
	g_free(str);
	gst_object_unref(pad);
     }
   gst_iterator_free(it);
}

void
emotion_video_sink_fill(Emotion_Video_Sink *vsink, GstPad *pad, GstCaps *caps)
{
   GstStructure *structure;
   GstQuery     *query;
   const GValue *val;
   gchar        *str;

   structure = gst_caps_get_structure(caps, 0);
   str = gst_caps_to_string(caps);

   gst_structure_get_int(structure, "width", &vsink->width);
   gst_structure_get_int(structure, "height", &vsink->height);

   vsink->fps_num = 1;
   vsink->fps_den = 1;
   val = gst_structure_get_value(structure, "framerate");
   if (val)
     {
	vsink->fps_num = gst_value_get_fraction_numerator(val);
	vsink->fps_den = gst_value_get_fraction_denominator(val);
     }
   if (g_str_has_prefix(str, "video/x-raw-yuv"))
     {
	val = gst_structure_get_value(structure, "format");
	vsink->fourcc = gst_value_get_fourcc(val);
     }
   else if (g_str_has_prefix(str, "video/x-raw-rgb"))
     vsink->fourcc = GST_MAKE_FOURCC('A', 'R', 'G', 'B');
   else
     vsink->fourcc = 0;

   query = gst_query_new_duration(GST_FORMAT_TIME);
   if (gst_pad_query(pad, query))
     {
	gint64 time;

	gst_query_parse_duration(query, NULL, &time);
	vsink->length_time = (double)time / (double)GST_SECOND;
     }
   g_free(str);
   gst_query_unref(query);
}

void
emotion_audio_sink_fill(Emotion_Audio_Sink *asink, GstPad *pad, GstCaps *caps)
{
   GstStructure *structure;
   GstQuery     *query;

   structure = gst_caps_get_structure(caps, 0);

   gst_structure_get_int(structure, "channels", &asink->channels);
   gst_structure_get_int(structure, "rate", &asink->samplerate);

   query = gst_query_new_duration(GST_FORMAT_TIME);
   if (gst_pad_query(pad, query))
     {
	gint64 time;

	gst_query_parse_duration(query, NULL, &time);
	asink->length_time = (double)time / (double)GST_SECOND;
     }
   gst_query_unref(query);
}
