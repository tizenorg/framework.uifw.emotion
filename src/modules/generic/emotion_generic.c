#ifdef HAVE_CONFIG_H
# include "config.h"
#endif
#include <sys/mman.h>
#include <sys/stat.h>
#include <sys/time.h>
#include <sys/types.h>
#include <fcntl.h>
#include <unistd.h>
#include <Eina.h>
#include <Evas.h>

#include "Emotion.h"
#include "emotion_private.h"
#include "emotion_generic.h"

static Eina_Prefix *pfx = NULL;

static int _emotion_generic_log_domain = -1;
#define DBG(...) EINA_LOG_DOM_DBG(_emotion_generic_log_domain, __VA_ARGS__)
#define INF(...) EINA_LOG_DOM_INFO(_emotion_generic_log_domain, __VA_ARGS__)
#define WRN(...) EINA_LOG_DOM_WARN(_emotion_generic_log_domain, __VA_ARGS__)
#define ERR(...) EINA_LOG_DOM_ERR(_emotion_generic_log_domain, __VA_ARGS__)
#define CRITICAL(...) EINA_LOG_DOM_CRIT(_emotion_generic_log_domain, __VA_ARGS__)


struct _default_players {
   const char *name;
   const char *cmdline;
};

static struct _default_players players[] = {
#ifdef EMOTION_BUILD_GENERIC_VLC
       { "vlc", "em_generic_vlc" },
#endif
       { NULL, NULL }
};

static const char *
_get_player(const char *name)
{
   const char *selected_name = NULL;
   const char *libdir = eina_prefix_lib_get(pfx);
   static char buf[PATH_MAX];
   int i;

   if (name)
     {
        for (i = 0; players[i].name; i++)
          {
             if (!strcmp(players[i].name, name))
               {
                  selected_name = players[i].cmdline;
                  break;
               }
          }
     }

   if ((!selected_name) && (name))
     selected_name = name;

   if (selected_name)
     {
        const char *cmd;

        if (selected_name[0] == '/') cmd = selected_name;
        else
          {
             snprintf(buf, sizeof(buf), "%s/emotion/utils/%s",
                      libdir, selected_name);
             cmd = buf;
          }

        DBG("Try generic player '%s'", cmd);
        if (access(cmd, R_OK | X_OK) == 0)
          {
             INF("Using generic player '%s'", cmd);
             return cmd;
          }
     }

   for (i = 0; players[i].name; i++)
     {
        snprintf(buf, sizeof(buf), "%s/emotion/utils/%s",
                 libdir, players[i].cmdline);
        DBG("Try generic player '%s'", buf);
        if (access(buf, R_OK | X_OK) == 0)
          {
             INF("Using fallback player '%s'", buf);
             return buf;
          }
     }

   ERR("no generic player found, given name='%s'", name ? name : "");
   return NULL;
}

static void
_player_send_cmd(Emotion_Generic_Video *ev, int cmd)
{
   if (cmd >= EM_CMD_LAST)
     {
	ERR("invalid command to player.");
	return;
     }
   write(ev->fd_write, &cmd, sizeof(cmd));
}

static void
_player_send_int(Emotion_Generic_Video *ev, int number)
{
   write(ev->fd_write, &number, sizeof(number));
}

static void
_player_send_float(Emotion_Generic_Video *ev, float number)
{
   write(ev->fd_write, &number, sizeof(number));
}

static void
_player_send_str(Emotion_Generic_Video *ev, const char *str, Eina_Bool stringshared)
{
   int len;

   if (stringshared)
     len = eina_stringshare_strlen(str) + 1;
   else
     len = strlen(str) + 1;
   write(ev->fd_write, &len, sizeof(len));
   write(ev->fd_write, str, len);
}

static Eina_Bool
_create_shm_data(Emotion_Generic_Video *ev, const char *shmname)
{
   int shmfd;
   int npages;
   size_t size;
   Emotion_Generic_Video_Shared *vs;

   shmfd = shm_open(shmname, O_CREAT | O_RDWR | O_TRUNC, 0777);
   if (shmfd == -1)
     {
	ERR("player: could not open shm: %s", shmname);
	ERR("player: %s", strerror(errno));
	return 0;
     }
   size = 3 * (ev->w * ev->h * DEFAULTPITCH) + sizeof(*vs);

   npages = (int)(size / getpagesize()) + 1;
   size = npages * getpagesize();

   if (ftruncate(shmfd, size))
     {
	ERR("error when allocating shared memory (size = %zd): "
	    "%s", size, strerror(errno));
	shm_unlink(shmname);
	return EINA_FALSE;
     }
   vs = mmap(NULL, size, PROT_READ|PROT_WRITE, MAP_SHARED, shmfd, 0);
   if (vs == MAP_FAILED)
     {
	ERR("error when mapping shared memory.\n");
	return EINA_FALSE;
     }

   vs->size = size;
   vs->width = ev->w;
   vs->height = ev->h;
   vs->pitch = DEFAULTPITCH;
   vs->frame.emotion = 0;
   vs->frame.player = 1;
   vs->frame.last = 2;
   vs->frame.next = 2;
   vs->frame_drop = 0;
   sem_init(&vs->lock, 1, 1);
   ev->frame.frames[0] = (unsigned char *)vs + sizeof(*vs);
   ev->frame.frames[1] = (unsigned char *)vs + sizeof(*vs) + vs->height * vs->width * vs->pitch;
   ev->frame.frames[2] = (unsigned char *)vs + sizeof(*vs) + 2 * vs->height * vs->width * vs->pitch;

   if (ev->shared)
     munmap(ev->shared, ev->shared->size);
   ev->shared = vs;

   return EINA_TRUE;
}

static void
_player_new_frame(Emotion_Generic_Video *ev)
{
   if (!ev->file_ready)
     return;
   _emotion_frame_new(ev->obj);
}

static void
_file_open(Emotion_Generic_Video *ev)
{
   INF("Opening file: %s", ev->filename);
   ev->drop = 0;

   if (!ev->ready || !ev->filename)
     return;
   _player_send_cmd(ev, EM_CMD_FILE_SET);
   _player_send_str(ev, ev->filename, EINA_TRUE);
}

static void
_player_file_set_done(Emotion_Generic_Video *ev)
{
   if (ev->file_changed)
     {
	_file_open(ev);
	ev->file_changed = EINA_FALSE;
	return;
     }

   if (!_create_shm_data(ev, ev->shmname))
     {
	ERR("could not create shared memory.");
	return;
     }
   _player_send_cmd(ev, EM_CMD_FILE_SET_DONE);
}

static void
_player_ready(Emotion_Generic_Video *ev)
{
   INF("received: player ready.");

   ev->initializing = EINA_FALSE;
   ev->ready = EINA_TRUE;

   if (!ev->filename)
     return;

   _file_open(ev);
}

static Eina_Bool
_player_cmd_param_read(Emotion_Generic_Video *ev, void *param, size_t size)
{
   ssize_t done, todo, i;

   /* When a parameter must be read, we cannot make sure it will be entirely
    * available. Thus we store the bytes that could be read in a temp buffer,
    * and when more data is read we try to complete the buffer and finally use
    * the read value.
    */
   if (!ev->cmd.tmp)
     {
	ev->cmd.tmp = malloc(size);
	ev->cmd.i = 0;
	ev->cmd.total = size;
     }

   todo = ev->cmd.total - ev->cmd.i;
   i = ev->cmd.i;
   done = read(ev->fd_read, &ev->cmd.tmp[i], todo);

   if (done < 0 &&  errno != EINTR && errno != EAGAIN)
     {
	if (ev->cmd.tmp)
	  {
	     free(ev->cmd.tmp);
	     ev->cmd.tmp = NULL;
	  }
	ERR("problem when reading parameter from pipe.");
	ev->cmd.type = -1;
	return EINA_FALSE;
     }

   if (done == todo)
     {
	memcpy(param, ev->cmd.tmp, size);
	free(ev->cmd.tmp);
	ev->cmd.tmp = NULL;
	return EINA_TRUE;
     }

   if (done > 0)
     ev->cmd.i += done;

   return EINA_FALSE;
}

static void
_player_frame_resize(Emotion_Generic_Video *ev)
{
   int w, h;

   w = ev->cmd.param.size.width;
   h = ev->cmd.param.size.height;

   INF("received frame resize: %dx%d", w, h);
   ev->w = w;
   ev->h = h;
   ev->ratio = (float)w / h;

   if (ev->opening)
     return;

   _emotion_frame_resize(ev->obj, ev->w, ev->h, ev->ratio);
}

static void
_player_length_changed(Emotion_Generic_Video *ev)
{
   float length = ev->cmd.param.f_num;

   INF("received length changed: %0.3f", length);

   ev->len = length;
   _emotion_video_pos_update(ev->obj, ev->pos, ev->len);
}

static void
_player_position_changed(Emotion_Generic_Video *ev)
{
   float position = ev->cmd.param.f_num;

   INF("received position changed: %0.3f", position);

   ev->pos = position;
   _emotion_video_pos_update(ev->obj, ev->pos, ev->len);

   if (ev->len == 0)
     return;

   float progress = ev->pos / ev->len;
   char buf[16];
   snprintf(buf, sizeof(buf), "%0.1f%%", progress * 100);

   _emotion_progress_set(ev->obj, buf, progress);
}

static void
_player_seekable_changed(Emotion_Generic_Video *ev)
{
   int seekable = ev->cmd.param.i_num;

   INF("received seekable changed: %d", seekable);

   seekable = !!seekable;

   ev->seekable = seekable;
}

static void
_audio_channels_free(Emotion_Generic_Video *ev)
{
   int i;
   for (i = 0; i < ev->audio_channels_count; i++)
     eina_stringshare_del(ev->audio_channels[i].name);
   free(ev->audio_channels);
   ev->audio_channels_count = 0;
}

static void
_video_channels_free(Emotion_Generic_Video *ev)
{
   int i;
   for (i = 0; i < ev->video_channels_count; i++)
     eina_stringshare_del(ev->video_channels[i].name);
   free(ev->video_channels);
   ev->video_channels_count = 0;
}

static void
_spu_channels_free(Emotion_Generic_Video *ev)
{
   int i;
   for (i = 0; i < ev->spu_channels_count; i++)
     eina_stringshare_del(ev->spu_channels[i].name);
   free(ev->spu_channels);
   ev->spu_channels_count = 0;
}

static void
_player_tracks_info(Emotion_Generic_Video *ev, Emotion_Generic_Channel **channels, int *count, int *current)
{
   Emotion_Generic_Channel *pchannels;
   int i;

   *count = ev->cmd.param.track.total;
   *current = ev->cmd.param.track.current;
   pchannels = ev->cmd.param.track.channels;

   INF("number of tracks: %d (current = %d):", *count, *current);
   for (i = 0; i < *count; i++)
     {
	INF("\tchannel %d: %s", pchannels[i].id, pchannels[i].name);
     }

   *channels = pchannels;
}

static void
_player_audio_tracks_info(Emotion_Generic_Video *ev)
{
   INF("Receiving audio channels:");
   if (ev->audio_channels_count)
     _audio_channels_free(ev);

   _player_tracks_info(ev, &ev->audio_channels, &ev->audio_channels_count,
		       &ev->audio_channel_current);
}

static void
_player_video_tracks_info(Emotion_Generic_Video *ev)
{
   INF("Receiving video channels:");
   if (ev->video_channels_count)
     _video_channels_free(ev);

   _player_tracks_info(ev, &ev->video_channels, &ev->video_channels_count,
		       &ev->video_channel_current);
}

static void
_player_spu_tracks_info(Emotion_Generic_Video *ev)
{
   INF("Receiving spu channels:");
   if (ev->spu_channels_count)
     _spu_channels_free(ev);

   _player_tracks_info(ev, &ev->spu_channels, &ev->spu_channels_count,
		       &ev->spu_channel_current);
}

static void
_player_meta_info_free(Emotion_Generic_Video *ev)
{
   eina_stringshare_replace(&ev->meta.title, NULL);
   eina_stringshare_replace(&ev->meta.artist, NULL);
   eina_stringshare_replace(&ev->meta.album, NULL);
   eina_stringshare_replace(&ev->meta.year, NULL);
   eina_stringshare_replace(&ev->meta.genre, NULL);
   eina_stringshare_replace(&ev->meta.comment, NULL);
   eina_stringshare_replace(&ev->meta.disc_id, NULL);
   eina_stringshare_replace(&ev->meta.count, NULL);
}

static void
_player_meta_info_read(Emotion_Generic_Video *ev)
{
   INF("Receiving meta info:");
   _player_meta_info_free(ev);
   ev->meta.title = ev->cmd.param.meta.title;
   ev->meta.artist = ev->cmd.param.meta.artist;
   ev->meta.album = ev->cmd.param.meta.album;
   ev->meta.year = ev->cmd.param.meta.year;
   ev->meta.genre = ev->cmd.param.meta.genre;
   ev->meta.comment = ev->cmd.param.meta.comment;
   ev->meta.disc_id = ev->cmd.param.meta.disc_id;
   ev->meta.count = ev->cmd.param.meta.count;
   INF("title: '%s'", ev->meta.title);
   INF("artist: '%s'", ev->meta.artist);
   INF("album: '%s'", ev->meta.album);
   INF("year: '%s'", ev->meta.year);
   INF("genre: '%s'", ev->meta.genre);
   INF("comment: '%s'", ev->meta.comment);
   INF("disc_id: '%s'", ev->meta.disc_id);
   INF("count: '%s'", ev->meta.count);
}

static void
_player_file_closed(Emotion_Generic_Video *ev)
{
   INF("Closed previous file.");
   sem_destroy(&ev->shared->lock);

   ev->closing = EINA_FALSE;

   if (ev->opening)
     _file_open(ev);
}

static void
_player_open_done(Emotion_Generic_Video *ev)
{
   int success;

   success = ev->cmd.param.i_num;
   shm_unlink(ev->shmname);

   if (ev->file_changed)
     {
	_file_open(ev);
	ev->file_changed = EINA_FALSE;
	return;
     }

   ev->opening = EINA_FALSE;
   if (!success)
     {
	ERR("Could not open file.");
	return;
     }

   ev->file_ready = EINA_TRUE;

   _emotion_open_done(ev->obj);

   if (ev->play)
     {
	_player_send_cmd(ev, EM_CMD_PLAY);
	_player_send_float(ev, ev->pos);
     }

   _player_send_cmd(ev, EM_CMD_VOLUME_SET);
   _player_send_float(ev, ev->volume);

   _player_send_cmd(ev, EM_CMD_SPEED_SET);
   _player_send_float(ev, ev->speed);

   int mute = ev->audio_mute;
   _player_send_cmd(ev, EM_CMD_AUDIO_MUTE_SET);
   _player_send_int(ev, mute);

   mute = ev->video_mute;
   _player_send_cmd(ev, EM_CMD_VIDEO_MUTE_SET);
   _player_send_int(ev, mute);

   mute = ev->spu_mute;
   _player_send_cmd(ev, EM_CMD_SPU_MUTE_SET);
   _player_send_int(ev, mute);

   INF("Open done");
}

static void
_player_cmd_process(Emotion_Generic_Video *ev)
{
   switch (ev->cmd.type) {
      case EM_RESULT_INIT:
	 _player_ready(ev);
	 break;
      case EM_RESULT_FRAME_NEW:
	 _player_new_frame(ev);
	 break;
      case EM_RESULT_FILE_SET:
	 _player_file_set_done(ev);
	 break;
      case EM_RESULT_FILE_SET_DONE:
	 _player_open_done(ev);
	 break;
      case EM_RESULT_FILE_CLOSE:
	 _player_file_closed(ev);
	 break;
      case EM_RESULT_PLAYBACK_STOPPED:
	 _emotion_playback_finished(ev->obj);
	 break;
      case EM_RESULT_FRAME_SIZE:
	 _player_frame_resize(ev);
	 break;
      case EM_RESULT_LENGTH_CHANGED:
	 _player_length_changed(ev);
	 break;
      case EM_RESULT_POSITION_CHANGED:
	 _player_position_changed(ev);
	 break;
      case EM_RESULT_SEEKABLE_CHANGED:
	 _player_seekable_changed(ev);
	 break;
      case EM_RESULT_AUDIO_TRACK_INFO:
	 _player_audio_tracks_info(ev);
	 break;
      case EM_RESULT_VIDEO_TRACK_INFO:
	 _player_video_tracks_info(ev);
	 break;
      case EM_RESULT_SPU_TRACK_INFO:
	 _player_spu_tracks_info(ev);
	 break;
      case EM_RESULT_META_INFO:
	 _player_meta_info_read(ev);
	 break;
      default:
	 WRN("received wrong command: %d", ev->cmd.type);
   }

   ev->cmd.type = -1;
}

static void
_player_cmd_single_int_process(Emotion_Generic_Video *ev)
{
   if (!_player_cmd_param_read(ev, &ev->cmd.param.i_num, sizeof(ev->cmd.param.i_num)))
     return;

   _player_cmd_process(ev);
}

static void
_player_cmd_single_float_process(Emotion_Generic_Video *ev)
{
   if (!_player_cmd_param_read(ev, &ev->cmd.param.f_num, sizeof(ev->cmd.param.f_num)))
     return;

   _player_cmd_process(ev);
}

static void
_player_cmd_double_int_process(Emotion_Generic_Video *ev)
{
   int param;

   if (ev->cmd.num_params == 0)
     {
	ev->cmd.num_params = 2;
	ev->cmd.cur_param = 0;
	ev->cmd.param.size.width = 0;
	ev->cmd.param.size.height = 0;
     }

   if (!_player_cmd_param_read(ev, &param, sizeof(param)))
     return;

   if (ev->cmd.cur_param == 0)
     ev->cmd.param.size.width = param;
   else
     ev->cmd.param.size.height = param;

   ev->cmd.cur_param++;
   if (ev->cmd.cur_param == ev->cmd.num_params)
     _player_cmd_process(ev);
}

static void
_player_cmd_track_info(Emotion_Generic_Video *ev)
{
   int param;
   int i;

   if (ev->cmd.num_params == 0)
     {
	ev->cmd.cur_param = 0;
	ev->cmd.num_params = 2;
	ev->cmd.param.track.channels = NULL;
	ev->cmd.s_len = -1;
     }

   while (ev->cmd.cur_param < 2)
     {
	if (!_player_cmd_param_read(ev, &param, sizeof(param)))
	  return;

	if (ev->cmd.cur_param == 0)
	  ev->cmd.param.track.current = param;
	else
	  {
	     ev->cmd.param.track.total = param;
	     ev->cmd.num_params += param * 2;
	     ev->cmd.param.track.channels =
		calloc(param, sizeof(*ev->cmd.param.track.channels));
	  }
	ev->cmd.cur_param++;
     }

   if (ev->cmd.cur_param == ev->cmd.num_params)
     {
	_player_cmd_process(ev);
	return;
     }

   i = (ev->cmd.cur_param - 2) / 2;
   if ((ev->cmd.cur_param % 2) == 0) // reading track id
     {
	if (!_player_cmd_param_read(ev, &param, sizeof(param)))
	  return;
	ev->cmd.param.track.channels[i].id = param;
	ev->cmd.cur_param++;
     }
   else // reading track name
     {
	char buf[PATH_MAX];

	if (ev->cmd.s_len == -1)
	  {
	     if (!_player_cmd_param_read(ev, &param, sizeof(param)))
	       return;
	     ev->cmd.s_len = param;
	  }

	if (!_player_cmd_param_read(ev, buf, ev->cmd.s_len))
	  return;
	ev->cmd.param.track.channels[i].name = 
	   eina_stringshare_add_length(buf, ev->cmd.s_len);
	ev->cmd.cur_param++;
	ev->cmd.s_len = -1;
     }

   if (ev->cmd.cur_param == ev->cmd.num_params)
     _player_cmd_process(ev);
}

static void
_player_cmd_meta_info(Emotion_Generic_Video *ev)
{
   int param;
   const char *info;
   char buf[PATH_MAX];

   if (ev->cmd.num_params == 0)
     {
	ev->cmd.cur_param = 0;
	ev->cmd.num_params = 8;
	ev->cmd.param.meta.title = NULL;
	ev->cmd.param.meta.artist = NULL;
	ev->cmd.param.meta.album = NULL;
	ev->cmd.param.meta.year = NULL;
	ev->cmd.param.meta.genre = NULL;
	ev->cmd.param.meta.comment = NULL;
	ev->cmd.param.meta.disc_id = NULL;
	ev->cmd.param.meta.count = NULL;
	ev->cmd.s_len = -1;
     }

   if (ev->cmd.s_len == -1)
     {
	if (!_player_cmd_param_read(ev, &param, sizeof(param)))
	  return;
	ev->cmd.s_len = param;
     }

   if (!_player_cmd_param_read(ev, buf, ev->cmd.s_len))
     return;

   info = eina_stringshare_add_length(buf, ev->cmd.s_len);
   ev->cmd.s_len = -1;

   if (ev->cmd.cur_param == 0)
     ev->cmd.param.meta.title = info;
   else if (ev->cmd.cur_param == 1)
     ev->cmd.param.meta.artist = info;
   else if (ev->cmd.cur_param == 2)
     ev->cmd.param.meta.album = info;
   else if (ev->cmd.cur_param == 3)
     ev->cmd.param.meta.year = info;
   else if (ev->cmd.cur_param == 4)
     ev->cmd.param.meta.genre = info;
   else if (ev->cmd.cur_param == 5)
     ev->cmd.param.meta.comment = info;
   else if (ev->cmd.cur_param == 6)
     ev->cmd.param.meta.disc_id = info;
   else if (ev->cmd.cur_param == 7)
     ev->cmd.param.meta.count = info;

   ev->cmd.cur_param++;

   if (ev->cmd.cur_param == 8)
     _player_cmd_process(ev);
}

static void
_player_cmd_read(Emotion_Generic_Video *ev)
{
   if (ev->cmd.type < 0)
     {
	if (!_player_cmd_param_read(ev, &ev->cmd.type, sizeof(ev->cmd.type)))
	  return;
	ev->cmd.num_params = 0;
     }

   switch (ev->cmd.type) {
      case EM_RESULT_INIT:
      case EM_RESULT_FILE_SET:
      case EM_RESULT_PLAYBACK_STOPPED:
      case EM_RESULT_FILE_CLOSE:
      case EM_RESULT_FRAME_NEW:
	 _player_cmd_process(ev);
	 break;
      case EM_RESULT_FILE_SET_DONE:
      case EM_RESULT_SEEKABLE_CHANGED:
	 _player_cmd_single_int_process(ev);
	 break;
      case EM_RESULT_LENGTH_CHANGED:
      case EM_RESULT_POSITION_CHANGED:
	 _player_cmd_single_float_process(ev);
	 break;
      case EM_RESULT_FRAME_SIZE:
	 _player_cmd_double_int_process(ev);
	 break;
      case EM_RESULT_AUDIO_TRACK_INFO:
      case EM_RESULT_VIDEO_TRACK_INFO:
      case EM_RESULT_SPU_TRACK_INFO:
	 _player_cmd_track_info(ev);
	 break;
      case EM_RESULT_META_INFO:
	 _player_cmd_meta_info(ev);
	 break;

      default:
	 WRN("received wrong command: %d", ev->cmd.type);
	 ev->cmd.type = -1;
   }
}

static Eina_Bool
_player_cmd_handler_cb(void *data, Ecore_Fd_Handler *fd_handler)
{
   Emotion_Generic_Video *ev = data;

   if (ecore_main_fd_handler_active_get(fd_handler, ECORE_FD_ERROR))
     {
	ERR("an error occurred on fd_read %d.", ev->fd_read);
	return ECORE_CALLBACK_CANCEL;
     }

   _player_cmd_read(ev);

   return ECORE_CALLBACK_RENEW;
}

static Eina_Bool
_player_data_cb(void *data, int type __UNUSED__, void *event)
{
   Ecore_Exe_Event_Data *ev = event;
   Emotion_Generic_Video *evideo = data;
   int i;

   if (ev->exe != evideo->player.exe)
     {
        ERR("slave != ev->exe");
	return ECORE_CALLBACK_DONE;
     }

   for (i = 0; ev->lines[i].line; i++)
     INF("received input from player: \"%s\"", ev->lines[i].line);

   return ECORE_CALLBACK_DONE;
}

static Eina_Bool
_player_add_cb(void *data, int type __UNUSED__, void *event)
{
   Ecore_Exe_Event_Add *event_add = event;
   Ecore_Exe *player = event_add->exe;
   Emotion_Generic_Video *ev = data;

   if (ev->player.exe != player)
     {
	ERR("ev->player != player.");
	return ECORE_CALLBACK_DONE;
     }

   _player_send_cmd(ev, EM_CMD_INIT);
   _player_send_str(ev, ev->shmname, EINA_TRUE);

   return ECORE_CALLBACK_DONE;
}

static Eina_Bool
_player_del_cb(void *data, int type __UNUSED__, void *event __UNUSED__)
{
   Emotion_Generic_Video *ev = data;
   ERR("player died.");

   ev->player.exe = NULL;
   ev->ready = EINA_FALSE;
   ev->file_ready = EINA_FALSE;
   ecore_main_fd_handler_del(ev->fd_handler);
   close(ev->fd_read);
   close(ev->fd_write);
   ev->fd_read = -1;
   ev->fd_write = -1;
   _emotion_decode_stop(ev->obj);

   return ECORE_CALLBACK_DONE;
}

static Eina_Bool
_player_exec(Emotion_Generic_Video *ev)
{
   int pipe_out[2];
   int pipe_in[2];
   char buf[PATH_MAX];

   if (pipe(pipe_out) == -1)
     {
	ERR("could not create pipe for communication emotion -> player: %s", strerror(errno));
	return EINA_FALSE;
     }

   if (pipe(pipe_in) == -1)
     {
	ERR("could not create pipe for communication player -> emotion: %s", strerror(errno));
	close(pipe_out[0]);
	close(pipe_out[1]);
	return EINA_FALSE;
     }

   snprintf(buf, sizeof(buf), "%s %d %d\n", ev->cmdline, pipe_out[0], pipe_in[1]);

   ev->player.exe = ecore_exe_pipe_run(
      buf,
      ECORE_EXE_PIPE_READ | ECORE_EXE_PIPE_WRITE |
      ECORE_EXE_PIPE_READ_LINE_BUFFERED | ECORE_EXE_NOT_LEADER,
      ev);

   INF("created pipe emotion -> player: %d -> %d\n", pipe_out[1], pipe_out[0]);
   INF("created pipe player -> emotion: %d -> %d\n", pipe_in[1], pipe_in[0]);

   close(pipe_in[1]);
   close(pipe_out[0]);

   if (!ev->player.exe)
     {
	close(pipe_in[0]);
	close(pipe_out[1]);
	return EINA_FALSE;
     }

   ev->fd_read = pipe_in[0];
   ev->fd_write = pipe_out[1];

   ev->fd_handler = ecore_main_fd_handler_add(
      ev->fd_read, ECORE_FD_READ | ECORE_FD_ERROR, _player_cmd_handler_cb, ev,
      NULL, NULL);

   return EINA_TRUE;
}

static Eina_Bool
_fork_and_exec(Evas_Object *obj __UNUSED__, Emotion_Generic_Video *ev)
{
   char shmname[256];
   struct timeval tv;

   gettimeofday(&tv, NULL);
   snprintf(shmname, sizeof(shmname), "/em-generic-shm_%d_%d",
	    (int)tv.tv_sec, (int)tv.tv_usec);

   ev->shmname = eina_stringshare_add(shmname);

   ev->player_add = ecore_event_handler_add(
      ECORE_EXE_EVENT_ADD, _player_add_cb, ev);
   ev->player_del = ecore_event_handler_add(
      ECORE_EXE_EVENT_DEL, _player_del_cb, ev);
   ev->player_data = ecore_event_handler_add(
      ECORE_EXE_EVENT_DATA, _player_data_cb, ev);


   if (!_player_exec(ev))
     {
        ERR("could not start player.");
        return EINA_FALSE;
     }

   ev->initializing = EINA_TRUE;

   return EINA_TRUE;
}

static unsigned char
em_init(Evas_Object *obj, void **emotion_video, Emotion_Module_Options *opt)
{
   Emotion_Generic_Video *ev;
   const char *player;

   if (!emotion_video) return 0;
   player = _get_player(opt ? opt->player : NULL);
   if (!player) return 0;

   ev = (Emotion_Generic_Video *)calloc(1, sizeof(*ev));
   if (!ev) return 0;

   ev->fd_read = -1;
   ev->fd_write = -1;
   ev->speed = 1.0;
   ev->volume = 0.5;
   ev->audio_mute = EINA_FALSE;
   ev->cmd.type = -1;

   ev->obj = obj;
   ev->cmdline = eina_stringshare_add(player);
   *emotion_video = ev;

   return _fork_and_exec(obj, ev);
}

static int
em_shutdown(void *data)
{
   Emotion_Generic_Video *ev = data;

   if (!ev) return 0;

   if (ev->player.exe)
     {
	ecore_exe_terminate(ev->player.exe);
	ecore_exe_free(ev->player.exe);
	ev->player.exe = NULL;
     }

   if (ev->shared)
     munmap(ev->shared, ev->shared->size);

   if (ev->fd_read >= 0)
     close(ev->fd_read);
   if (ev->fd_write >= 0)
     close(ev->fd_write);
   if (ev->fd_handler)
     ecore_main_fd_handler_del(ev->fd_handler);

   eina_stringshare_del(ev->cmdline);
   eina_stringshare_del(ev->shmname);

   ecore_event_handler_del(ev->player_add);
   ecore_event_handler_del(ev->player_data);
   ecore_event_handler_del(ev->player_del);

   return 1;
}

static unsigned char
em_file_open(const char *file, Evas_Object *obj __UNUSED__, void *data)
{
   Emotion_Generic_Video *ev = data;
   INF("file set: %s", file);
   if (!ev) return 0;

   eina_stringshare_replace(&ev->filename, file);

   ev->pos = 0;
   ev->w = 0;
   ev->h = 0;
   ev->ratio = 1;
   ev->len = 0;

   if (ev->ready && ev->opening)
     {
	INF("file changed while opening.");
	ev->file_changed = EINA_TRUE;
	return 1;
     }

   ev->opening = EINA_TRUE;

   if (!ev->closing)
     _file_open(ev);

   return 1;
}

static void
em_file_close(void *data)
{
   Emotion_Generic_Video *ev = data;

   if (!ev || !ev->filename) return;

   INF("file close: %s", ev->filename);

   eina_stringshare_replace(&ev->filename, NULL);

   ev->file_ready = EINA_FALSE;
   _audio_channels_free(ev);
   _video_channels_free(ev);
   _spu_channels_free(ev);
   _player_meta_info_free(ev);

   if (ev->opening)
     return;

   _player_send_cmd(ev, EM_CMD_FILE_CLOSE);
   ev->closing = EINA_TRUE;
}

static Emotion_Format
em_format_get(void *ef __UNUSED__)
{
   return EMOTION_FORMAT_BGRA;
}

static void
em_video_data_size_get(void *data, int *w, int *h)
{
   Emotion_Generic_Video *ev = data;

   if (!ev) return;
   if (w) *w = ev->w;
   if (h) *h = ev->h;
}

static void
em_play(void *data, double pos)
{
   Emotion_Generic_Video *ev = data;

   if (!ev)
     return;

   ev->play = EINA_TRUE;
   INF("play: %0.3f", pos);

   if (ev->initializing || ev->opening)
     return;

   if (ev->ready)
     {
	_player_send_cmd(ev, EM_CMD_PLAY);
	_player_send_float(ev, ev->pos);
	return;
     }

   if (!_player_exec(ev))
     ERR("could not start player.");
}

static void
em_stop(void *data)
{
   Emotion_Generic_Video *ev = data;

   if (!ev)
     return;

   ev->play = EINA_FALSE;

   if (!ev->file_ready)
     return;

   _player_send_cmd(ev, EM_CMD_STOP);
   _emotion_decode_stop(ev->obj);
}

static void
em_size_get(void *data, int *w, int *h)
{
   Emotion_Generic_Video *ev = data;
   if (w) *w = ev->w;
   if (h) *h = ev->h;
}

static void
em_pos_set(void *data, double pos)
{
   Emotion_Generic_Video *ev = data;
   float position = pos;

   if (!ev->file_ready)
     return;

   _player_send_cmd(ev, EM_CMD_POSITION_SET);
   _player_send_float(ev, position);
   _emotion_seek_done(ev->obj);
}

static double
em_len_get(void *data)
{
   Emotion_Generic_Video *ev = data;
   return ev->len;
}

static int
em_fps_num_get(void *data)
{
   Emotion_Generic_Video *ev = data;
   return (int)(ev->fps * 1000.0);
}

static int
em_fps_den_get(void *ef __UNUSED__)
{
   return 1000;
}

static double
em_fps_get(void *data)
{
   Emotion_Generic_Video *ev = data;
   return ev->fps;
}

static double
em_pos_get(void *data)
{
   Emotion_Generic_Video *ev = data;
   return ev->pos;
}

static void
em_vis_set(void *ef __UNUSED__, Emotion_Vis vis __UNUSED__)
{
}

static Emotion_Vis
em_vis_get(void *data)
{
   Emotion_Generic_Video *ev = data;
   return ev->vis;
}

static Eina_Bool
em_vis_supported(void *ef __UNUSED__, Emotion_Vis vis __UNUSED__)
{
   return EINA_FALSE;
}

static double
em_ratio_get(void *data)
{
   Emotion_Generic_Video *ev = data;
   return ev->ratio;
}

static int em_video_handled(void *ef __UNUSED__)
{
   fprintf(stderr, "video handled!\n");
   return 1;
}

static int em_audio_handled(void *ef __UNUSED__)
{
   fprintf(stderr, "audio handled!\n");
   return 1;
}

static int em_seekable(void *data)
{
   Emotion_Generic_Video *ev = data;
   return ev->seekable;
}

static void em_frame_done(void *ef __UNUSED__)
{
}

static int
em_yuv_rows_get(void *data __UNUSED__, int w __UNUSED__, int h __UNUSED__, unsigned char **yrows __UNUSED__, unsigned char **urows __UNUSED__, unsigned char **vrows __UNUSED__)
{
   return 0;
}

static int
em_bgra_data_get(void *data, unsigned char **bgra_data)
{
   Emotion_Generic_Video *ev = data;

   if (!ev || !ev->file_ready)
     return 0;

   // lock frame here
   sem_wait(&ev->shared->lock);

   // send current frame to emotion
   if (ev->shared->frame.emotion != ev->shared->frame.last)
     {
	ev->shared->frame.next = ev->shared->frame.emotion;
	ev->shared->frame.emotion = ev->shared->frame.last;
     }
   *bgra_data = ev->frame.frames[ev->shared->frame.emotion];

   if (ev->shared->frame_drop > 1)
     WRN("dropped frames: %d", ev->shared->frame_drop - 1);
   ev->shared->frame_drop = 0;

   // unlock frame here
   sem_post(&ev->shared->lock);
   ev->drop = 0;

   return 1;
}

static void
em_event_feed(void *ef __UNUSED__, int event __UNUSED__)
{
}

static void
em_event_mouse_button_feed(void *ef __UNUSED__, int button __UNUSED__, int x __UNUSED__, int y __UNUSED__)
{
}

static void
em_event_mouse_move_feed(void *ef __UNUSED__, int x __UNUSED__, int y __UNUSED__)
{
}

static int
em_video_channel_count(void *data)
{
   Emotion_Generic_Video *ev = data;
   return ev->video_channels_count;
}

static void
em_video_channel_set(void *data, int channel)
{
   Emotion_Generic_Video *ev = data;

   if (channel < 0 || channel >= ev->video_channels_count)
     {
	WRN("video channel out of range.");
	return;
     }

   _player_send_cmd(ev, EM_CMD_VIDEO_TRACK_SET);
   _player_send_int(ev, ev->video_channels[channel].id);
   ev->video_channel_current = channel;
}

static int
em_video_channel_get(void *data)
{
   Emotion_Generic_Video *ev = data;
   return ev->video_channel_current;
}

static const char *
em_video_channel_name_get(void *data, int channel)
{
   Emotion_Generic_Video *ev = data;

   if (channel < 0 || channel >= ev->video_channels_count)
     {
	WRN("video channel out of range.");
	return NULL;
     }

   return ev->video_channels[channel].name;
}

static void
em_video_channel_mute_set(void *data, int mute)
{
   Emotion_Generic_Video *ev = data;

   ev->video_mute = !!mute;

   if (!ev || !ev->file_ready)
     return;

   _player_send_cmd(ev, EM_CMD_VIDEO_MUTE_SET);
   _player_send_int(ev, mute);
}

static int
em_video_channel_mute_get(void *data)
{
   Emotion_Generic_Video *ev = data;
   return ev->video_mute;
}

static int
em_audio_channel_count(void *data)
{
   Emotion_Generic_Video *ev = data;
   return ev->audio_channels_count;
}

static void
em_audio_channel_set(void *data, int channel)
{
   Emotion_Generic_Video *ev = data;

   if (channel < 0 || channel >= ev->audio_channels_count)
     {
	WRN("audio channel out of range.");
	return;
     }

   _player_send_cmd(ev, EM_CMD_AUDIO_TRACK_SET);
   _player_send_int(ev, ev->audio_channels[channel].id);
   ev->audio_channel_current = channel;
}

static int
em_audio_channel_get(void *data)
{
   Emotion_Generic_Video *ev = data;
   return ev->audio_channel_current;
}

static const char *
em_audio_channel_name_get(void *data, int channel)
{
   Emotion_Generic_Video *ev = data;

   if (channel < 0 || channel >= ev->audio_channels_count)
     {
	WRN("audio channel out of range.");
	return NULL;
     }

   return ev->audio_channels[channel].name;
}

static void
em_audio_channel_mute_set(void *data, int mute)
{
   Emotion_Generic_Video *ev = data;

   ev->audio_mute = !!mute;

   if (!ev || !ev->file_ready)
     return;

   _player_send_cmd(ev, EM_CMD_AUDIO_MUTE_SET);
   _player_send_int(ev, mute);
}

static int
em_audio_channel_mute_get(void *data)
{
   Emotion_Generic_Video *ev = data;
   return ev->audio_mute;
}

static void
em_audio_channel_volume_set(void *data, double vol)
{
   Emotion_Generic_Video *ev = data;

   if (vol > 1.0) vol = 1.0;
   if (vol < 0.0) vol = 0.0;

   ev->volume = vol;

   if (!ev || !ev->file_ready)
     return;

   _player_send_cmd(ev, EM_CMD_VOLUME_SET);
   _player_send_float(ev, ev->volume);
}

static double
em_audio_channel_volume_get(void *data)
{
   Emotion_Generic_Video *ev = data;
   return ev->volume;
}

static int
em_spu_channel_count(void *data)
{
   Emotion_Generic_Video *ev = data;
   return ev->spu_channels_count;
}

static void
em_spu_channel_set(void *data, int channel)
{
   Emotion_Generic_Video *ev = data;

   if (channel < 0 || channel >= ev->spu_channels_count)
     {
	WRN("spu channel out of range.");
	return;
     }

   _player_send_cmd(ev, EM_CMD_SPU_TRACK_SET);
   _player_send_int(ev, ev->spu_channels[channel].id);
   ev->spu_channel_current = channel;
}

static int
em_spu_channel_get(void *data)
{
   Emotion_Generic_Video *ev = data;
   return ev->spu_channel_current;
}

static const char *
em_spu_channel_name_get(void *data, int channel)
{
   Emotion_Generic_Video *ev = data;

   if (channel < 0 || channel >= ev->spu_channels_count)
     {
	WRN("spu channel out of range.");
	return NULL;
     }

   return ev->spu_channels[channel].name;
}

static void
em_spu_channel_mute_set(void *data, int mute)
{
   Emotion_Generic_Video *ev = data;

   ev->spu_mute = !!mute;

   if (!ev || !ev->file_ready)
     return;

   _player_send_cmd(ev, EM_CMD_SPU_MUTE_SET);
   _player_send_int(ev, mute);
}

static int
em_spu_channel_mute_get(void *data)
{
   Emotion_Generic_Video *ev = data;
   return ev->spu_mute;
}

static int
em_chapter_count(void *ef __UNUSED__)
{
   int num = 0;
   return num;
}

static void
em_chapter_set(void *ef __UNUSED__, int chapter __UNUSED__)
{
}

static int
em_chapter_get(void *ef __UNUSED__)
{
   int num = 0;
   return num;
}

static const char *
em_chapter_name_get(void *ef __UNUSED__, int chapter __UNUSED__)
{
   return NULL;
}

static void
em_speed_set(void *data, double speed)
{
   Emotion_Generic_Video *ev = data;
   float rate = speed;
   ev->speed = rate;

   if (!ev || !ev->file_ready)
     return;

   _player_send_cmd(ev, EM_CMD_SPEED_SET);
   _player_send_float(ev, rate);
}

static double
em_speed_get(void *data)
{
   Emotion_Generic_Video *ev = data;
   return (double)ev->speed;
}

static int
em_eject(void *ef __UNUSED__)
{
   return 1;
}

static const char *
em_meta_get(void *data, int meta)
{
   Emotion_Generic_Video *ev = data;

   switch (meta) {
      case EMOTION_META_INFO_TRACK_TITLE:
	 return ev->meta.title;
      case EMOTION_META_INFO_TRACK_ARTIST:
	 return ev->meta.artist;
      case EMOTION_META_INFO_TRACK_ALBUM:
	 return ev->meta.album;
      case EMOTION_META_INFO_TRACK_YEAR:
	 return ev->meta.year;
      case EMOTION_META_INFO_TRACK_GENRE:
	 return ev->meta.genre;
      case EMOTION_META_INFO_TRACK_COMMENT:
	 return ev->meta.comment;
      case EMOTION_META_INFO_TRACK_DISC_ID:
	 return ev->meta.disc_id;
      case EMOTION_META_INFO_TRACK_COUNT:
	 return ev->meta.count;
   }

   return NULL;
}

static Emotion_Video_Module em_module =
{
   em_init, /* init */
   em_shutdown, /* shutdown */
   em_file_open, /* file_open */
   em_file_close, /* file_close */
   em_play, /* play */
   em_stop, /* stop */
   em_size_get, /* size_get */
   em_pos_set, /* pos_set */
   em_len_get, /* len_get */
   em_fps_num_get, /* fps_num_get */
   em_fps_den_get, /* fps_den_get */
   em_fps_get, /* fps_get */
   em_pos_get, /* pos_get */
   em_vis_set, /* vis_set */
   em_vis_get, /* vis_get */
   em_vis_supported, /* vis_supported */
   em_ratio_get, /* ratio_get */
   em_video_handled, /* video_handled */
   em_audio_handled, /* audio_handled */
   em_seekable, /* seekable */
   em_frame_done, /* frame_done */
   em_format_get, /* format_get */
   em_video_data_size_get, /* video_data_size_get */
   em_yuv_rows_get, /* yuv_rows_get */
   em_bgra_data_get, /* bgra_data_get */
   em_event_feed, /* event_feed */
   em_event_mouse_button_feed, /* event_mouse_button_feed */
   em_event_mouse_move_feed, /* event_mouse_move_feed */
   em_video_channel_count, /* video_channel_count */
   em_video_channel_set, /* video_channel_set */
   em_video_channel_get, /* video_channel_get */
   em_video_channel_name_get, /* video_channel_name_get */
   em_video_channel_mute_set, /* video_channel_mute_set */
   em_video_channel_mute_get, /* video_channel_mute_get */
   em_audio_channel_count, /* audio_channel_count */
   em_audio_channel_set, /* audio_channel_set */
   em_audio_channel_get, /* audio_channel_get */
   em_audio_channel_name_get, /* audio_channel_name_get */
   em_audio_channel_mute_set, /* audio_channel_mute_set */
   em_audio_channel_mute_get, /* audio_channel_mute_get */
   em_audio_channel_volume_set, /* audio_channel_volume_set */
   em_audio_channel_volume_get, /* audio_channel_volume_get */
   em_spu_channel_count, /* spu_channel_count */
   em_spu_channel_set, /* spu_channel_set */
   em_spu_channel_get, /* spu_channel_get */
   em_spu_channel_name_get, /* spu_channel_name_get */
   em_spu_channel_mute_set, /* spu_channel_mute_set */
   em_spu_channel_mute_get, /* spu_channel_mute_get */
   em_chapter_count, /* chapter_count */
   em_chapter_set, /* chapter_set */
   em_chapter_get, /* chapter_get */
   em_chapter_name_get, /* chapter_name_get */
   em_speed_set, /* speed_set */
   em_speed_get, /* speed_get */
   em_eject, /* eject */
   em_meta_get, /* meta_get */
   NULL, /* priority_set */
   NULL, /* priority_get */
   NULL /* handle */
};

static Eina_Bool
module_open(Evas_Object *obj, const Emotion_Video_Module **module, void **video, Emotion_Module_Options *opt)
{
   if (!module)	{
	return EINA_FALSE;
   }

   if (_emotion_generic_log_domain < 0)
     {
        eina_threads_init();
        eina_log_threads_enable();
        _emotion_generic_log_domain = eina_log_domain_register
          ("emotion-generic", EINA_COLOR_LIGHTCYAN);
        if (_emotion_generic_log_domain < 0)
          {
             EINA_LOG_CRIT("Could not register log domain 'emotion-generic'");
             return EINA_FALSE;
          }
     }


   if (!em_module.init(obj, video, opt))	{
	return EINA_FALSE;
   }

   *module = &em_module;

   return EINA_TRUE;
}

static void module_close(Emotion_Video_Module *module __UNUSED__, void *video)
{
	em_module.shutdown(video);
}


Eina_Bool
generic_module_init(void)
{
   if (!pfx)
     {
        pfx = eina_prefix_new(NULL, emotion_object_add,
                              "EMOTION", "emotion", NULL,
                              PACKAGE_BIN_DIR,
                              PACKAGE_LIB_DIR,
                              PACKAGE_DATA_DIR,
                              "");
        if (!pfx) return EINA_FALSE;
     }
   return _emotion_module_register("generic", module_open, module_close);
}

static void
generic_module_shutdown(void)
{
   if (pfx)
     {
        eina_prefix_free(pfx);
        pfx = NULL;
     }
   _emotion_module_unregister("generic");
}

#ifndef EMOTION_STATIC_BUILD_GENERIC

EINA_MODULE_INIT(generic_module_init);
EINA_MODULE_SHUTDOWN(generic_module_shutdown);

#endif

