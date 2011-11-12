//
//     Copyright 2011, Kay Hayen, mailto:kayhayen@gmx.de
//
//     Part of "Nuitka", an optimizing Python compiler that is compatible and
//     integrates with CPython, but also works on its own.
//
//     If you submit Kay Hayen patches to this software in either form, you
//     automatically grant him a copyright assignment to the code, or in the
//     alternative a BSD license to the code, should your jurisdiction prevent
//     this. Obviously it won't affect code that comes to him indirectly or
//     code you don't submit to him.
//
//     This is to reserve my ability to re-license the code at any time, e.g.
//     the PSF. With this version of Nuitka, using it for Closed Source will
//     not be allowed.
//
//     This program is free software: you can redistribute it and/or modify
//     it under the terms of the GNU General Public License as published by
//     the Free Software Foundation, version 3 of the License.
//
//     This program is distributed in the hope that it will be useful,
//     but WITHOUT ANY WARRANTY; without even the implied warranty of
//     MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
//     GNU General Public License for more details.
//
//     You should have received a copy of the GNU General Public License
//     along with this program.  If not, see <http://www.gnu.org/licenses/>.
//
//     Please leave the whole of this copyright notice intact.
//

#include "nuitka/prelude.hpp"

#include "nuitka/compiled_method.hpp"

// Needed for offsetof
#include <stddef.h>

// tp_descr_get slot, bind a function to an object.
static PyObject *Nuitka_Function_descr_get( PyObject *function, PyObject *object, PyObject *klass )
{
#ifdef OLD
    return PyMethod_New( function, object == Py_None ? NULL : object, klass );
#else
    assert( Nuitka_Function_Check( function ) );

    return Nuitka_Method_New(
        (Nuitka_FunctionObject *)function,
        object == Py_None ? NULL : object,
        klass
    );
#endif
}

 // tp_repr slot, decide how a function shall be output
static PyObject *Nuitka_Function_tp_repr( Nuitka_FunctionObject *function )
{
    return PyString_FromFormat( "<compiled function %s at %p>", PyString_AsString( function->m_name ), function );
}

static PyObject *Nuitka_Function_tp_call( Nuitka_FunctionObject *function, PyObject *args, PyObject *kw )
{
    if ( function->m_has_args )
    {
       return ((PyCFunctionWithKeywords)function->m_code)( (PyObject *)function->m_context, args, kw );
    }
    else
    {
       return ((PyNoArgsFunction)function->m_code)( (PyObject *)function->m_context );
    }
}

static long Nuitka_Function_tp_traverse( PyObject *function, visitproc visit, void *arg )
{
    // TODO: Identify the impact of not visiting owned objects and/or if it could be NULL
    // instead. The methodobject visits its self and module. I understand this is probably
    // so that back references of this function to its upper do not make it stay in the
    // memory. A specific test if that works might be needed.
    return 0;
}

static int Nuitka_Function_tp_compare( Nuitka_FunctionObject *a, Nuitka_FunctionObject *b )
{
    if ( a->m_counter == b->m_counter )
    {
       return 0;
    }
    else
    {
       return a->m_counter < b->m_counter ? -1 : 1;
    }
}

static long Nuitka_Function_tp_hash( Nuitka_FunctionObject *function )
{
    return function->m_counter;
}

static PyObject *Nuitka_Function_get_name( Nuitka_FunctionObject *object )
{
    return INCREASE_REFCOUNT( object->m_name );
}

static int Nuitka_Function_set_name( Nuitka_FunctionObject *object, PyObject *value )
{
    if (value == NULL || PyString_Check( value ) == 0)
    {
       PyErr_Format( PyExc_TypeError, "__name__ must be set to a string object" );
       return -1;
    }

    PyObject *old = object->m_name;
    Py_DECREF( old );
    object->m_name = INCREASE_REFCOUNT( value );

    return 0;
}

static PyObject *Nuitka_Function_get_doc( Nuitka_FunctionObject *object )
{
    return INCREASE_REFCOUNT( object->m_doc );
}

static int Nuitka_Function_set_doc( Nuitka_FunctionObject *object, PyObject *value )
{
    if ( value == Py_None || value == NULL )
    {
       object->m_doc = Py_None;
    }
    else if ( PyString_Check( value ) == 0 )
    {
       PyErr_Format( PyExc_TypeError, "__name__ must be set to a string object" );
       return -1;
    }
    else
    {
       object->m_doc = INCREASE_REFCOUNT( value );
    }

    return 0;
}

static PyObject *Nuitka_Function_get_dict( Nuitka_FunctionObject *object )
{
    if ( object->m_dict == NULL )
    {
        object->m_dict = PyDict_New();
    }

    return INCREASE_REFCOUNT( object->m_dict );
}

static int Nuitka_Function_set_dict( Nuitka_FunctionObject *object, PyObject *value )
{
    if (value == NULL)
    {
        PyErr_Format( PyExc_TypeError, "function's dictionary may not be deleted");
        return -1;
    }

    if ( PyDict_Check(value) )
    {
        PyObject *old = object->m_dict;
        object->m_dict = INCREASE_REFCOUNT( value );
        Py_XDECREF( old );

        return 0;
    }
    else
    {
        PyErr_SetString( PyExc_TypeError, "setting function's dictionary to a non-dict" );
        return -1;
    }
}

static PyObject *Nuitka_Function_get_code( Nuitka_FunctionObject *object )
{
    return INCREASE_REFCOUNT( (PyObject *)object->m_code_object );
}

static int Nuitka_Function_set_code( Nuitka_FunctionObject *object, PyObject *value )
{
    PyErr_Format( PyExc_RuntimeError, "__code__ is not writable in Nuitka" );
    return -1;
}

static int Nuitka_Function_set_globals( Nuitka_FunctionObject *object, PyObject *value )
{
    PyErr_Format( PyExc_TypeError, "readonly attribute" );
    return -1;
}

static PyObject *Nuitka_Function_get_globals( Nuitka_FunctionObject *object )
{
    return INCREASE_REFCOUNT( PyModule_GetDict( object->m_module ) );
}

static int Nuitka_Function_set_module( Nuitka_FunctionObject *object, PyObject *value )
{
    if ( object->m_dict == NULL )
    {
       object->m_dict = PyDict_New();
    }

    return PyDict_SetItemString( object->m_dict, (char * )"__module__", value );
}

static PyObject *Nuitka_Function_get_module( Nuitka_FunctionObject *object )
{
    if ( object->m_dict )
    {
        PyObject *result = PyDict_GetItemString( object->m_dict, (char * )"__module__" );

        if ( result != NULL )
        {
            return INCREASE_REFCOUNT( result );
        }
    }

    int res = Nuitka_Function_set_module( object, PyString_FromString( PyModule_GetName( object->m_module ) ) );

    assert( res == 0 );

    return PyDict_GetItemString( object->m_dict, (char * )"__module__" );
}


static PyGetSetDef Nuitka_Function_getset[] =
{
   { (char *)"func_name",    (getter)Nuitka_Function_get_name,    (setter)Nuitka_Function_set_name, NULL },
   { (char *)"__name__" ,    (getter)Nuitka_Function_get_name,    (setter)Nuitka_Function_set_name, NULL },
   { (char *)"func_doc",     (getter)Nuitka_Function_get_doc,     (setter)Nuitka_Function_set_doc, NULL },
   { (char *)"__doc__" ,     (getter)Nuitka_Function_get_doc,     (setter)Nuitka_Function_set_doc, NULL },
   { (char *)"func_dict",    (getter)Nuitka_Function_get_dict,    (setter)Nuitka_Function_set_dict, NULL },
   { (char *)"__dict__",     (getter)Nuitka_Function_get_dict,    (setter)Nuitka_Function_set_dict, NULL },
   { (char *)"func_code",    (getter)Nuitka_Function_get_code,    (setter)Nuitka_Function_set_code, NULL },
   { (char *)"__code__",     (getter)Nuitka_Function_get_code,    (setter)Nuitka_Function_set_code, NULL },
   { (char *)"func_globals", (getter)Nuitka_Function_get_globals, (setter)Nuitka_Function_set_globals, NULL },
   { (char *)"__globals__",  (getter)Nuitka_Function_get_globals, (setter)Nuitka_Function_set_globals, NULL },
   { (char *)"__module__",   (getter)Nuitka_Function_get_module,  (setter)Nuitka_Function_set_module, NULL },
   { NULL }
};


static void Nuitka_Function_tp_dealloc( Nuitka_FunctionObject *function )
{
    Nuitka_GC_UnTrack( function );

    if ( function->m_weakrefs != NULL )
    {
        PyObject_ClearWeakRefs( (PyObject *)function );
    }

    Py_DECREF( function->m_name );
    Py_XDECREF( function->m_dict );

    if ( function->m_context )
    {
        function->m_cleanup( function->m_context );
    }

    PyObject_GC_Del( function );
}

PyTypeObject Nuitka_Function_Type =
{
    PyVarObject_HEAD_INIT(&PyType_Type, 0)
    "compiled_function",                            // tp_name
    sizeof(Nuitka_FunctionObject),                  // tp_basicsize
    0,                                              // tp_itemsize
    (destructor)Nuitka_Function_tp_dealloc,         // tp_dealloc
    0,                                              // tp_print
    0,                                              // tp_getattr
    0,                                              // tp_setattr
    (cmpfunc)Nuitka_Function_tp_compare,            // tp_compare
    (reprfunc)Nuitka_Function_tp_repr,              // tp_repr
    0,                                              // tp_as_number
    0,                                              // tp_as_sequence
    0,                                              // tp_as_mapping
    (hashfunc)Nuitka_Function_tp_hash,              // tp_hash
    (ternaryfunc)Nuitka_Function_tp_call,           // tp_call
    0,                                              // tp_str
    PyObject_GenericGetAttr,                        // tp_getattro
    0,                                              // tp_setattro
    0,                                              // tp_as_buffer
    Py_TPFLAGS_DEFAULT | Py_TPFLAGS_HAVE_GC | Py_TPFLAGS_HAVE_WEAKREFS, // tp_flags
    0,                                              // tp_doc
    (traverseproc)Nuitka_Function_tp_traverse,      // tp_traverse
    0,                                              // tp_clear
    0,                                              // tp_richcompare
    offsetof( Nuitka_FunctionObject, m_weakrefs ),  // tp_weaklistoffset
    0,                                              // tp_iter
    0,                                              // tp_iternext
    0,                                              // tp_methods
    0,                                              // tp_members
    Nuitka_Function_getset,                         // tp_getset
    0,                                              // tp_base
    0,                                              // tp_dict
    Nuitka_Function_descr_get,                      // tp_descr_get
    0,                                              // tp_descr_set
    offsetof( Nuitka_FunctionObject, m_dict ),      // tp_dictoffset
    0,                                              // tp_init
    0,                                              // tp_alloc
    0,                                              // tp_new
    0,                                              // tp_free
    0,                                              // tp_is_gc
    0,                                              // tp_bases
    0,                                              // tp_mro
    0,                                              // tp_cache
    0,                                              // tp_subclasses
    0,                                              // tp_weaklist
    0,                                              // tp_del
};

static inline PyObject *make_kfunction( void *code, method_arg_parser mparse, PyObject *name, PyCodeObject *code_object, PyObject *module, PyObject *doc, bool has_args, void *context, releaser cleanup )
{
    Nuitka_FunctionObject *result = PyObject_GC_New( Nuitka_FunctionObject, &Nuitka_Function_Type );

    if (unlikely( result == NULL ))
    {
        PyErr_Format( PyExc_RuntimeError, "cannot create function %s", PyString_AsString( name ) );
        throw _PythonException();
    }

    result->m_code = code;
    result->m_has_args = has_args;
    result->m_method_arg_parser = mparse;

    result->m_name = INCREASE_REFCOUNT( name );

    result->m_context = context;
    result->m_cleanup = cleanup;

    result->m_code_object = code_object;

    result->m_module = module;
    result->m_doc    = doc;

    result->m_dict   = NULL;
    result->m_weakrefs = NULL;

    static long Nuitka_Function_counter = 0;
    result->m_counter = Nuitka_Function_counter++;

    Nuitka_GC_Track( result );
    return (PyObject *)result;
}

// Make a function without context.
PyObject *Nuitka_Function_New( function_arg_parser fparse, method_arg_parser mparse, PyObject *name, PyCodeObject *code_object, PyObject *module, PyObject *doc )
{
    return make_kfunction( (void *)fparse, mparse, name, code_object, module, doc, true, NULL, NULL );
}

// Make a function with context.
PyObject *Nuitka_Function_New( function_arg_parser fparse, method_arg_parser mparse, PyObject *name, PyCodeObject *code_object, PyObject *module, PyObject *doc, void *context, releaser cleanup )
{
    return make_kfunction( (void *)fparse, mparse, name, code_object, module, doc, true, context, cleanup );
}

// Make a function that is only a yielder, no args.
PyObject *Nuitka_Function_New( argless_code code, PyObject *name, PyObject *module, PyObject *doc, void *context, releaser cleanup )
{
    return make_kfunction( (void *)code, NULL, name, NULL, module, doc, false, context, cleanup );
}