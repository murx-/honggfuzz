/*
 *
 * honggfuzz - python mutator
 * -----------------------------------------
 *
 * Author: Tillmann Osswald <swiecki@google.com>
 *
 *
 * Licensed under the Apache License, Version 2.0 (the "License"); you may
 * not use this file except in compliance with the License. You may obtain
 * a copy of the License at
 *
 * http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or
 * implied. See the License for the specific language governing
 * permissions and limitations under the License.
 *
 */


#include "pythonMutate.h"

#define PY_SSIZE_T_CLEAN
#include "input.h"


#include <inttypes.h>
#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/mman.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <unistd.h>
#include <libgen.h>
#include <Python.h>

#include "libhfcommon/common.h"
#include "libhfcommon/log.h"
#include "libhfcommon/util.h"
#include "libhfcommon/files.h"
#include "honggfuzz.h"


typedef struct {
    bool init;
    PyObject* pModule;
    PyObject* pFuncInit;
    PyObject* pFuncFuzz;
} pythonMutate_t;

static pythonMutate_t pythonMutate_obj;
static bool inited = false;
static bool wait = false;

bool pythonMutate_init(run_t* run) {
    wait = true;
    LOG_D("Initializing python with module: %s", run->global->exe.pythonMutator)

    char *dirc = strdup(run->global->exe.pythonMutator);
    char *dname = dirname(dirc);

    const char *bname = files_basename(run->global->exe.pythonMutator);
    *rindex(bname, '.') = 0;

    setenv("PYTHONPATH", dname, 1);

    Py_Initialize();
    PyObject* pName = PyUnicode_FromString(bname);
    pythonMutate_obj.pModule = PyImport_Import(pName);
    //pythonMutate_obj.pModule = PyImport_ImportModule(run->global->exe.pythonMutator);
    Py_DECREF(pName);

    if (pythonMutate_obj.pModule != NULL) {
        LOG_D("Check if functions are callable")
        // Check if the python functions are callable
        pythonMutate_obj.pFuncInit = PyObject_GetAttrString(pythonMutate_obj.pModule, "init");
        if (!pythonMutate_obj.pFuncInit || !PyCallable_Check(pythonMutate_obj.pFuncInit)) {
            LOG_F("init() not callable in %s.", run->global->exe.pythonMutator);
        }
        LOG_D("init() is callable in %s", run->global->exe.pythonMutator)
        pythonMutate_obj.pFuncFuzz = PyObject_GetAttrString(pythonMutate_obj.pModule, "fuzz");
        if (!pythonMutate_obj.pFuncFuzz || !PyCallable_Check(pythonMutate_obj.pFuncFuzz)) {
            LOG_F("fuzz() not callable in %s.", run->global->exe.pythonMutator);
        }
        LOG_D("fuzz() is callable in %s", run->global->exe.pythonMutator)

        PyObject *funcInitRetVal = PyObject_CallObject(pythonMutate_obj.pFuncInit, NULL);

        if (funcInitRetVal == NULL) {
            PyErr_Print();
            LOG_F("init() in python module %s did not retun correctly! ", run->global->exe.pythonMutator);
        }

        Py_DECREF(funcInitRetVal);

        inited=true;
        wait=false;

        return true;
    }
    fprintf(stderr, "%s ", "python ");
    PyErr_Print();
    LOG_F("Failed to initialize python module %s", run->global->exe.pythonMutator);
}

bool pythonMutateFile(run_t* run) {
    LOG_D("python mutator %s called", run->global->exe.pythonMutator)
    if (!inited && !wait) {
        pythonMutate_init(run);
    }
    LOG_D("Using python mutator")


    PyObject *funcFuzzArguments, *funcFuzzBuffer, *funcFuzzRetVal;

    funcFuzzArguments = PyTuple_New(1);

    funcFuzzBuffer = PyByteArray_FromStringAndSize((char*)&run->dynamicFile[0], run->dynamicFileSz);
    if (!funcFuzzArguments) {
        Py_DECREF(funcFuzzArguments);
        Py_DECREF(funcFuzzBuffer);
        PyErr_Print();
        LOG_E("Cannot convert dynamicFile to Bytes")
        return false;
    }
    PyTuple_SetItem(funcFuzzArguments, 0, funcFuzzBuffer);

    funcFuzzRetVal = PyObject_CallObject(pythonMutate_obj.pFuncFuzz, funcFuzzArguments);

    Py_DECREF(funcFuzzBuffer);
    Py_DECREF(funcFuzzArguments);
    if (!funcFuzzRetVal) {
        PyErr_Print();
        LOG_E("Mutation failed!")
        Py_DECREF(funcFuzzRetVal);
        Py_DECREF(funcFuzzBuffer);
        return false;
    }
    size_t funcFuzzRetLen = PyByteArray_Size(funcFuzzRetVal);
    if (funcFuzzRetLen > run->global->mutate.maxFileSz) {
        Py_DECREF(funcFuzzRetVal);
        Py_DECREF(funcFuzzBuffer);
        LOG_D("Output too big!")
        return true;
    }

    input_setSize(run, funcFuzzRetLen);
    char * bytes = PyBytes_AsString(funcFuzzRetVal);
    if (PyErr_Occurred()) {
        PyErr_Print();
        Py_DECREF(funcFuzzRetVal);
        Py_DECREF(funcFuzzBuffer);
        LOG_E("Failed to convert python return value to bytes")
        return false;
    }

    memmove(&run->dynamicFile[0], bytes, funcFuzzRetLen);
    Py_DECREF(funcFuzzRetVal);
    Py_DECREF(funcFuzzBuffer);


    return true;
}
