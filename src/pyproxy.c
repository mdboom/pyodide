#include <Python.h>
#include <emscripten.h>

#include "hiwire.h"
#include "js2python.h"
#include "python2js.h"

void
_py_decref(int ptrobj)
{
  PyObject* pyobj = (PyObject*)ptrobj;
  Py_DECREF(pyobj);
}

int
_pyobject_delattr(int x, int b)
{
  return PyObject_DelAttr((PyObject*)x, (PyObject*)b);
}

EM_JS(int, pyproxy_new, (int ptrobj), {
  var target = function(){};
  target['$$'] = { ptr : ptrobj, type : 'PyProxy' };
  return Module.hiwire_new_value(new Proxy(target, Module.PyProxy));
});

EM_JS(int, pyproxy_init, (), {
  // clang-format off
  Module.PyProxy = {
    getPtr: function(jsobj) {
      var ptr = jsobj['$$']['ptr'];
      if (ptr === null) {
        throw new Error("Object has already been destroyed");
      }
      return ptr;
    },

    isPyProxy: function(jsobj) {
      return jsobj['$$'] !== undefined && jsobj['$$']['type'] === 'PyProxy';
    },

    addExtraKeys: function(result) {
      result.push('toString');
      result.push('prototype');
      result.push('arguments');
      result.push('caller');
    },

    isExtensible: function() { return true },

    has: function (jsobj, jskey) {
      var ptrobj = this.getPtr(jsobj);
      var pykey = Module.js2python(jskey);
      var result = _PyObject_HasAttr(ptrobj, pykey) ? true : false;
      __py_decref(pykey);
      return result;
    },

    get: function (jsobj, jskey) {
      ptrobj = this.getPtr(jsobj);
      if (jskey === 'toString') {
        return function() {
          if (self.pyodide.repr === undefined) {
            self.pyodide.repr = self.pyodide.pyimport('repr');
          }
          return self.pyodide.repr(jsobj);
        }
      } else if (jskey === '$$') {
        return jsobj['$$'];
      } else if (jskey === 'destroy') {
        return function() {
          __py_decref(ptrobj);
          jsobj['$$']['ptr'] = null;
        }
      } else if (jskey === 'apply') {
        return function(jsthis, jsargs) {
          return this.apply(jsobj, jsthis, jsargs);
        }
      }
      ptrobj = this.getPtr(jsobj);
      var pykey = Module.js2python(jskey);
      var pyattr = _PyObject_GetAttr(ptrobj, pykey);
      __py_decref(pykey);
      if (pyattr == 0) {
        _PyErr_Clear();
        return undefined;
      }
      var idattr = _python2js(pyattr);
      __py_decref(pyattr);
      var jsresult = Module.hiwire_get_value(idattr);
      Module.hiwire_decref(idattr);
      return jsresult;
    },

    set: function (jsobj, jskey, jsval) {
      ptrobj = this.getPtr(jsobj);
      var pykey = Module.js2python(jskey);
      var pyval = Module.js2python(jsval);
      var result = _PyObject_SetAttr(ptrobj, pykey, pyval);
      __py_decref(pykey);
      __py_decref(pyval);
      if (result !== 0) {
        _pythonexc2js();
      }
      return jsval;
    },

    deleteProperty: function (jsobj, jskey) {
      ptrobj = this.getPtr(jsobj);
      var pykey = Module.js2python(jskey);
      var result = __pyobject_delattr(ptrobj, pykey);
      __py_decref(pykey);
      if (result !== 0) {
        _pythonexc2js();
      }
      return undefined;
    },

    ownKeys: function (jsobj) {
      ptrobj = this.getPtr(jsobj);
      var pydir = _PyObject_Dir(ptrobj);
      if (pydir === 0) {
        return _pythonexc2js();
      }
      var result = [];
      var n = _PyList_Size(pydir);
      for (var i = 0; i < n; ++i) {
        var pyentry = _PyList_GetItem(pydir, i);
        var identry = _python2js(pyentry);
        var jsentry = Module.hiwire_get_value(identry);
        result.push(jsentry);
        Module.hiwire_decref(identry);
      }
      __py_decref(pydir);
      this.addExtraKeys(result);
      return result;
    },

    enumerate: function (jsobj) {
      return this.ownKeys(jsobj);
    },

    apply: function (jsobj, jsthis, jsargs) {
      ptrobj = this.getPtr(jsobj);
      var pyargs = _PyTuple_New(jsargs.length);
      for (var i = 0; i < jsargs.length; ++i) {
        var pyitem = Module.js2python(jsargs[i]);
        _PyTuple_SetItem(pyargs, i, pyitem);
      }
      var pyresult = _PyObject_Call(ptrobj, pyargs, 0);
      __py_decref(pyargs);
      if (pyresult === 0) {
        return _pythonexc2js();
      }
      var idresult = _python2js(pyresult);
      var jsresult = Module.hiwire_get_value(idresult);
      __py_decref(pyresult);
      Module.hiwire_decref(idresult);
      return jsresult;
    },
  };

  return 0;
// clang-format on
});
