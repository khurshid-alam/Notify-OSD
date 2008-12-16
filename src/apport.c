#include <glib.h>

void
apport_report (const gchar* app_name,
	       const gchar* summary,
	       gchar**      actions,
	       gint         timeout)
{
	/* call /usr/share/apport/notification_hook
	   passing all the arguments in the request
	*/

}
