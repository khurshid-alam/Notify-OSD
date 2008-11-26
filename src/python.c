/* 
 * python.c - Embedded Python interpreter
 *
 * This module helps us prototype part of our application
 * 
 * This code is based on Demo/embed/demo.c
 *
 */

#include <err.h>
#include <libgen.h>
#include <gtk/gtk.h>
#include "Python.h"
#include "stack-display.h"

void
python_init(int argc, char *argv[])
{
	Py_SetProgramName(argv[0]);

	Py_InitializeEx(0);

	PyRun_SimpleString("import sys");
	PyRun_SimpleString("sys.path.append(\"/home/dbarth/devel/alsdorf/src\")");

	/* Add hooks into internal modules */
	stack_init_python_interface();
}

/* Taken from: basename -- strip directory and suffix from filenames
   Copyright (C) 1990-1997, 1999-2003 Free Software Foundation, Inc.

   GPL Code
*/
static void
remove_suffix(char *name, const char *suffix)
{
  char *np;
  const char *sp;

  np = name + strlen(name);
  sp = suffix + strlen(suffix);

  while (np > name && sp > suffix)
    if (*--np != *--sp)
      return;
  if (np > name)
    *np = '\0';
}

int
python_load_script(char *filename)
{
	char *file = basename(g_strdup(filename));
	remove_suffix(file, ".py");

	PyObject *pname = PyString_FromString(file);
	PyObject *pmodule = PyImport_Import(pname);
	Py_DECREF(pname);

	if (pmodule == NULL) {
		PyErr_Print();
		warn("%s: could not load python module: %s",
		    __func__, filename);
		return 0;
	}
	return 1;
}

void
python_exit()
{
	/* Exit, cleaning up the interpreter */
	Py_Exit(0);
	/*NOTREACHED*/
}

