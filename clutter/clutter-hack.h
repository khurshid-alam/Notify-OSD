#ifndef __CLUTTER_HACK_H__
#define __CLUTTER_HACK_H__

#include <stdlib.h>
#include <stdio.h>
#include <string.h>

#ifdef HAVE_UNISTD_H
#include <unistd.h>
#endif

#include <math.h>

#include <glib.h>

// #include "clutter-backend.h"
// #include "clutter-event.h"
// #include "clutter-feature.h"
// #include "clutter-id-pool.h"
// #include "clutter-stage-manager.h"
// #include "clutter-stage-window.h"
// #include "clutter-stage.h"
// #include "pango/pangoclutter.h"

#include "clutter-debug.h"
#include "clutter-fixed.h"
#include "clutter-units.h"
#include "clutter-timeline.h"
#include "clutter-timeout-pool.h"

#define I_(str)  (g_intern_static_string ((str)))
#define CLUTTER_PRIORITY_TIMELINE       (G_PRIORITY_DEFAULT + 30)

#include <glib-object.h>

G_BEGIN_DECLS


#define CLUTTER_PARAM_READABLE  \
        G_PARAM_READABLE | G_PARAM_STATIC_NAME | G_PARAM_STATIC_NICK | G_PARAM_STATIC_BLURB
#define CLUTTER_PARAM_WRITABLE  \
        G_PARAM_WRITABLE | G_PARAM_STATIC_NAME | G_PARAM_STATIC_NICK | G_PARAM_STATIC_BLURB
#define CLUTTER_PARAM_READWRITE \
        G_PARAM_READABLE | G_PARAM_WRITABLE | G_PARAM_STATIC_NAME | G_PARAM_STATIC_NICK |G_PARAM_STATIC_BLURB

guint
clutter_get_default_frame_rate (void);

void
clutter_threads_enter (void);

void
clutter_threads_leave (void);

typedef enum {
  CLUTTER_INIT_SUCCESS        =  1,
  CLUTTER_INIT_ERROR_UNKNOWN  =  0,
  CLUTTER_INIT_ERROR_THREADS  = -1,
  CLUTTER_INIT_ERROR_BACKEND  = -2,
  CLUTTER_INIT_ERROR_INTERNAL = -3
} ClutterInitError;

ClutterInitError clutter_init             (int          *argc,
                                           char       ***argv);
void             clutter_main                       (void);


/* enumerations from "./clutter-timeline.h" */
GType clutter_timeline_direction_get_type (void) G_GNUC_CONST;
#define CLUTTER_TYPE_TIMELINE_DIRECTION (clutter_timeline_direction_get_type())

/* VOID:STRING,INT (./clutter-marshal.list:12) */
extern void clutter_marshal_VOID__STRING_INT (GClosure     *closure,
                                              GValue       *return_value,
                                              guint         n_param_values,
                                              const GValue *param_values,
                                              gpointer      invocation_hint,
                                              gpointer      marshal_data);

/* VOID:VOID (./clutter-marshal.list:13) */
#define clutter_marshal_VOID__VOID	g_cclosure_marshal_VOID__VOID

/* VOID:INT (./clutter-marshal.list:4) */
#define clutter_marshal_VOID__INT	g_cclosure_marshal_VOID__INT

/* UINT:VOID (./clutter-marshal.list:2) */
extern void clutter_marshal_UINT__VOID (GClosure     *closure,
                                        GValue       *return_value,
                                        guint         n_param_values,
                                        const GValue *param_values,
                                        gpointer      invocation_hint,
                                        gpointer      marshal_data);

G_END_DECLS

#endif /* !__CLUTTER_HACK_H__ */

