#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include <stdlib.h>
#include <glib.h>
#include <locale.h>

#include "clutter-hack.h"

static guint clutter_default_fps        = 60;

guint
clutter_get_default_frame_rate (void)
{
	return clutter_default_fps;
}

guint clutter_debug_flags = 0;  /* global clutter debug flag */

#ifdef CLUTTER_ENABLE_DEBUG
static const GDebugKey clutter_debug_keys[] = {
  { "misc", CLUTTER_DEBUG_MISC },
  { "actor", CLUTTER_DEBUG_ACTOR },
  { "texture", CLUTTER_DEBUG_TEXTURE },
  { "event", CLUTTER_DEBUG_EVENT },
  { "paint", CLUTTER_DEBUG_PAINT },
  { "gl", CLUTTER_DEBUG_GL },
  { "alpha", CLUTTER_DEBUG_ALPHA },
  { "behaviour", CLUTTER_DEBUG_BEHAVIOUR },
  { "pango", CLUTTER_DEBUG_PANGO },
  { "backend", CLUTTER_DEBUG_BACKEND },
  { "scheduler", CLUTTER_DEBUG_SCHEDULER },
  { "script", CLUTTER_DEBUG_SCRIPT },
  { "shader", CLUTTER_DEBUG_SHADER },
  { "multistage", CLUTTER_DEBUG_MULTISTAGE },
};
#endif /* CLUTTER_ENABLE_DEBUG */

gboolean
clutter_get_debug_enabled (void)
{
#ifdef CLUTTER_ENABLE_DEBUG
  return clutter_debug_flags != 0;
#else
  return FALSE;
#endif
}

void
clutter_threads_enter (void)
{
	return;
}

void
clutter_threads_leave (void)
{
	return;
}

static guint clutter_main_loop_level    = 0;
static GSList *main_loops               = NULL;
static gboolean clutter_is_initialized  = FALSE;


/**
 * clutter_main:
 *
 * Starts the Clutter mainloop.
 */
void
clutter_main (void)
{
  GMainLoop *loop;

#if 0
  /* Make sure there is a context */
  CLUTTER_CONTEXT ();
#endif

  if (!clutter_is_initialized)
    {
      g_warning ("Called clutter_main() but Clutter wasn't initialised.  "
		 "You must call clutter_init() first.");
      return;
    }

  CLUTTER_MARK ();

  clutter_main_loop_level++;

  loop = g_main_loop_new (NULL, TRUE);
  main_loops = g_slist_prepend (main_loops, loop);

#ifdef HAVE_CLUTTER_FRUITY
  /* clutter fruity creates an application that forwards events and manually
   * spins the mainloop
   */
  clutter_fruity_main ();
#else
  if (g_main_loop_is_running (main_loops->data))
    {
      clutter_threads_leave ();
      g_main_loop_run (loop);
      clutter_threads_enter ();
    }
#endif

  main_loops = g_slist_remove (main_loops, loop);

  g_main_loop_unref (loop);

  clutter_main_loop_level--;

  CLUTTER_MARK ();
}

ClutterInitError
clutter_init (int    *argc,
              char ***argv)
{
	if (!clutter_is_initialized)
	{
		/* initialise GLib type system */
		g_type_init ();

		clutter_is_initialized = TRUE;
	}

	return CLUTTER_INIT_SUCCESS;
}


/* -------------------------------------------------------------------------- */

/* auto-generated code taken from clutter-enum-types.c */

/* enumerations from "./clutter-timeline.h" */
#include "./clutter-timeline.h"
GType
clutter_timeline_direction_get_type(void) {
  static GType etype = 0;
  if (G_UNLIKELY (!etype))
    {
      static const GEnumValue values[] = {
        { CLUTTER_TIMELINE_FORWARD, "CLUTTER_TIMELINE_FORWARD", "forward" },
        { CLUTTER_TIMELINE_BACKWARD, "CLUTTER_TIMELINE_BACKWARD", "backward" },
        { 0, NULL, NULL }
      };
      etype = g_enum_register_static (g_intern_static_string ("ClutterTimelineDirection"), values);
    }
  return etype;
}

/* auto-generated code taken from clutter-marshal.c */

#define g_marshal_value_peek_string(v)   (v)->data[0].v_pointer
#define g_marshal_value_peek_int(v)      (v)->data[0].v_int

/* VOID:STRING,INT (./clutter-marshal.list:12) */
void
clutter_marshal_VOID__STRING_INT (GClosure     *closure,
                                  GValue       *return_value G_GNUC_UNUSED,
                                  guint         n_param_values,
                                  const GValue *param_values,
                                  gpointer      invocation_hint G_GNUC_UNUSED,
                                  gpointer      marshal_data)
{
  typedef void (*GMarshalFunc_VOID__STRING_INT) (gpointer     data1,
                                                 gpointer     arg_1,
                                                 gint         arg_2,
                                                 gpointer     data2);
  register GMarshalFunc_VOID__STRING_INT callback;
  register GCClosure *cc = (GCClosure*) closure;
  register gpointer data1, data2;

  g_return_if_fail (n_param_values == 3);

  if (G_CCLOSURE_SWAP_DATA (closure))
    {
      data1 = closure->data;
      data2 = g_value_peek_pointer (param_values + 0);
    }
  else
    {
      data1 = g_value_peek_pointer (param_values + 0);
      data2 = closure->data;
    }
  callback = (GMarshalFunc_VOID__STRING_INT) (marshal_data ? marshal_data : cc->callback);

  callback (data1,
            g_marshal_value_peek_string (param_values + 1),
            g_marshal_value_peek_int (param_values + 2),
            data2);
}

/* VOID:INT,INT (./clutter-marshal.list:6) */
void
clutter_marshal_VOID__INT_INT (GClosure     *closure,
                               GValue       *return_value G_GNUC_UNUSED,
                               guint         n_param_values,
                               const GValue *param_values,
                               gpointer      invocation_hint G_GNUC_UNUSED,
                               gpointer      marshal_data)
{
  typedef void (*GMarshalFunc_VOID__INT_INT) (gpointer     data1,
                                              gint         arg_1,
                                              gint         arg_2,
                                              gpointer     data2);
  register GMarshalFunc_VOID__INT_INT callback;
  register GCClosure *cc = (GCClosure*) closure;
  register gpointer data1, data2;

  g_return_if_fail (n_param_values == 3);

  if (G_CCLOSURE_SWAP_DATA (closure))
    {
      data1 = closure->data;
      data2 = g_value_peek_pointer (param_values + 0);
    }
  else
    {
      data1 = g_value_peek_pointer (param_values + 0);
      data2 = closure->data;
    }
  callback = (GMarshalFunc_VOID__INT_INT) (marshal_data ? marshal_data : cc->callback);

  callback (data1,
            g_marshal_value_peek_int (param_values + 1),
            g_marshal_value_peek_int (param_values + 2),
            data2);
}

/* UINT:VOID (./clutter-marshal.list:2) */
void
clutter_marshal_UINT__VOID (GClosure     *closure,
                            GValue       *return_value G_GNUC_UNUSED,
                            guint         n_param_values,
                            const GValue *param_values,
                            gpointer      invocation_hint G_GNUC_UNUSED,
                            gpointer      marshal_data)
{
  typedef guint (*GMarshalFunc_UINT__VOID) (gpointer     data1,
                                            gpointer     data2);
  register GMarshalFunc_UINT__VOID callback;
  register GCClosure *cc = (GCClosure*) closure;
  register gpointer data1, data2;
  guint v_return;

  g_return_if_fail (return_value != NULL);
  g_return_if_fail (n_param_values == 1);

  if (G_CCLOSURE_SWAP_DATA (closure))
    {
      data1 = closure->data;
      data2 = g_value_peek_pointer (param_values + 0);
    }
  else
    {
      data1 = g_value_peek_pointer (param_values + 0);
      data2 = closure->data;
    }
  callback = (GMarshalFunc_UINT__VOID) (marshal_data ? marshal_data : cc->callback);

  v_return = callback (data1,
                       data2);

  g_value_set_uint (return_value, v_return);
}

