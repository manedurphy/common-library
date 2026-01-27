/*
 * If not stated otherwise in this file or this component's LICENSE file the
 * following copyright and licenses apply:
 *
 * Copyright 2024 RDK Management
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 * http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#ifndef __CCSP_DML_LIB_H__
#define __CCSP_DML_LIB_H__

#include <cJSON.h>
#include <rbus/rbus.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "ansc_platform.h"

/*
 * @brief Initialize the CCSP DML library
 *
 * @param component_name The name of the CCSP component
 * @param json_file_path The path to the JSON file containing the data model
 */
int ccsp_dml_init(const char *component_name, const char *json_file_path);

#endif /* __CCSP_DML_LIB_H__ */
