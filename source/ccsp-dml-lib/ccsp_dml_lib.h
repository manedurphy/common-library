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

#include <cjson/cJSON.h>
#include <rbus/rbus.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "ansc_platform.h"

#define MAX_PARAM_NAME_LEN 256

typedef struct child_parameter {
  char name[MAX_PARAM_NAME_LEN];
  char type[MAX_PARAM_NAME_LEN];
  bool writable;
} child_parameter_t;

typedef struct parent_object {
  char name[MAX_PARAM_NAME_LEN];
  child_parameter_t *parameters;
  size_t child_parameter_count;
} parent_object_t;

// Linked list node for parent objects
typedef struct parent_object_node {
  parent_object_t parent;
  struct parent_object_node *next;
} parent_object_node_t;

/*
 * @brief Initialize the CCSP DML library
 *
 * @param component_name The name of the CCSP component
 * @param json_file_path The path to the JSON file containing the data model
 */
int ccsp_dml_init(const char *component_name, const char *json_file_path,
                  rbusGetHandler_t get_handler, rbusSetHandler_t set_handler);

/*
 * @brief Shutdown the CCSP DML library
 */
void ccsp_dml_shutdown(void);

/*
 * @brief Gets a parent object by name
 *
 * @param name The name of the parent object
 *
 * @return A pointer to the parent object, or NULL if not found
 */
parent_object_t *get_parent_object(const char *name);

#endif /* __CCSP_DML_LIB_H__ */
