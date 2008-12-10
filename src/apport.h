/* this module should send a report via apport when an application
   is trying to use elements of the spec we don't support anymore
*/

void
apport_report (const gchar* app_name,
	       const gchar* summary,
	       gchar**      actions,
	       gint         timeout);
