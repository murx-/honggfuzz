/*
 *
 * honggfuzz - mutating input with python for fuzzing
 * -----------------------------------------
 *
 * Author: Tillmann Osswald
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

#if defined(_HF_PYTHON)

#ifndef _HF_PYTHON_MUTATE_H_
#define _HF_PYTHON_MUTATE_H_

#include "honggfuzz.h"

extern bool pythonMutateFile(run_t* run);
extern bool pythonMutate_init(run_t* run);

#endif // define _HF_PYTHON_MUTATE_H_

#endif // defined(_HF_PYTHON)