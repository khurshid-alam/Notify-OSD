#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <egg/egg-hack.h>
#include <egg/egg-alpha.h>

static void
fade_cb (EggTimeline *timeline,
	 gint frame_no,
	 EggAlpha *alpha)
{
	g_debug ("new frame %d, alpha=%d",
		 frame_no,
		 egg_alpha_get_alpha (alpha));
}

static void
completed_cb (EggTimeline *timeline,
              gpointer *state)
{
	g_debug ("completed");
	exit (EXIT_SUCCESS);
}

int
main (int argc, char **argv)
{
  EggTimeline *timeline;
  EggAlpha *alpha;

  egg_init (&argc, &argv);

  timeline = egg_timeline_new_for_duration (700);
  alpha = egg_alpha_new_full (timeline,
				  EGG_ALPHA_SINE_DEC,
				  NULL,
				  NULL);
  g_signal_connect (G_OBJECT(timeline),
		    "completed",
		    G_CALLBACK(completed_cb),
		    NULL);
  g_signal_connect (G_OBJECT(timeline),
		    "new-frame",
		    G_CALLBACK(fade_cb),
		    alpha);

  egg_timeline_start (timeline);

  egg_main ();

  return EXIT_FAILURE;
}
