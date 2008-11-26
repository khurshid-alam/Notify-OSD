/* 
 * python.c - Embedded Python interpreter
 *
 * This module helps us prototype part of our application
 * 
 * This code is based on Demo/embed/demo.c
 *
 */

#include <gtk/gtk.h>
#include "Python.h"

void initxyzzy(void); /* Forward */

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
	char *file = basename(strdup(filename));
	remove_suffix(file, ".py");

	PyObject *pname = PyString_FromString(file);
	PyObject *pmodule = PyImport_Import(pname);
	Py_DECREF(pname);

	if (pmodule == NULL) {
		PyErr_Print();
		warn("%s: could not load python module: %s",
		    __func__, filename);
		return (NULL);
	}
}

void
python_exit()
{
	/* Exit, cleaning up the interpreter */
	Py_Exit(0);
	/*NOTREACHED*/
}

/* A static module */

/* 'self' is not used */
static PyObject *
xyzzy_foo(PyObject *self, PyObject *args)
{
	if (!PyArg_ParseTuple(args, ":numargs"))
		return NULL;
	return PyInt_FromLong(42L);
}

static PyMethodDef xyzzy_methods[] = {
	{"foo",		xyzzy_foo,	METH_VARARGS, "Return something"},
	{NULL,		NULL}		/* sentinel */
};

void
initxyzzy(void)
{
	PyImport_AddModule("xyzzy");
	Py_InitModule("xyzzy", xyzzy_methods);
}
