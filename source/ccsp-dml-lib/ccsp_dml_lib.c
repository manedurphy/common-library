#include "ccsp_dml_lib.h"

// Global RBUS handle
static rbusHandle_t g_rbus_handle = NULL;

// Store parent objects and their parameters in a linked list
static parent_object_node_t *parent_objects = NULL;

static void process_object_recursive(cJSON *obj, const char *path,
                                     rbusGetHandler_t get_handler,
                                     rbusSetHandler_t set_handler) {
  if (!obj || !cJSON_IsObject(obj)) {
    return;
  }

  parent_object_t parent_object;
  memset(parent_object.name, 0, sizeof(parent_object.name));
  strncpy(parent_object.name, path, strlen(path));
  //     (parent_object_t *)malloc(sizeof(parent_object_t));
  // if (parent_object == NULL) {
  //   fprintf(stderr, "Memory allocation failed for parent object\n");
  //   return;
  // }

  // memset(parent_object, 0, sizeof(parent_object_t));
  // strlcpy(parent_object->name, path, sizeof(parent_object->name));

  cJSON *child = obj->child;
  while (child) {
    if (strcmp(child->string, "List_Of_Def") == 0) {
      // Process the list of parameter definitions for a parent object

      if (!cJSON_IsArray(child)) {
        CcspTraceError(("List_Of_Def is not an array for %s\n", path));
        continue;
      }

      int array_size = cJSON_GetArraySize(child);
      parent_object.child_parameter_count = (size_t)array_size;
      parent_object.parameters =
          (child_parameter_t *)malloc(array_size * sizeof(child_parameter_t));
      if (parent_object.parameters == NULL) {
        fprintf(stderr, "Memory allocation failed for parameters of %s\n",
                path);
        return;
      }
      memset(parent_object.parameters, 0,
             array_size * sizeof(child_parameter_t));

      for (int i = 0; i < array_size; i++) {
        cJSON *param_obj = cJSON_GetArrayItem(child, i);
        if (!param_obj) {
          continue;
        }

        cJSON *param = param_obj->child;
        if (!param || !param->string) {
          continue;
        }

        char full_param_name[MAX_PARAM_NAME_LEN];
        snprintf(full_param_name, sizeof(full_param_name), "%s.%s", path,
                 param->string);

        cJSON *type_obj = cJSON_GetObjectItem(param, "type");

        if (!type_obj || !cJSON_IsString(type_obj)) {
          CcspTraceError(("Missing or invalid type for %s\n", full_param_name));
          continue;
        }

        // TODO: Not sure this is needed
        // rbusValueType_t rbus_type = RBUS_NONE;
        // if (strcmp(type_obj->valuestring, "boolean") == 0) {
        //   rbus_type = RBUS_BOOLEAN;
        // } else if (strcmp(type_obj->valuestring, "uint32_t") == 0) {
        //   rbus_type = RBUS_UINT32;
        // } else if (strcmp(type_obj->valuestring, "string") == 0) {
        //   rbus_type = RBUS_STRING;
        // }

        cJSON *writable_obj = cJSON_GetObjectItem(param, "writable");
        bool writable = writable_obj && cJSON_IsTrue(writable_obj);

        // Prepare RBUS data element
        rbusDataElement_t dataElement;
        memset(&dataElement, 0, sizeof(dataElement));

        dataElement.name = strdup(full_param_name);
        dataElement.type = RBUS_ELEMENT_TYPE_PROPERTY;
        dataElement.cbTable.getHandler = get_handler;
        dataElement.cbTable.setHandler = writable ? set_handler : NULL;

        CcspTraceInfo(("Registering RBUS element: %s (writable=%d)\n",
                       full_param_name, writable));

        // Register element with RBUS
        rbusError_t rc = rbus_regDataElements(g_rbus_handle, 1, &dataElement);
        if (rc != RBUS_ERROR_SUCCESS) {
          CcspTraceError(("Failed to register %s: %s\n", full_param_name,
                          rbusError_ToString(rc)));
          continue;
        }

        // Store parameter as a child of the parent object
        strncpy(parent_object.parameters[i].name, full_param_name,
                strlen(full_param_name));
        strncpy(parent_object.parameters[i].type, type_obj->valuestring,
                strlen(type_obj->valuestring));
        parent_object.parameters[i].writable = writable;
      }

      // Add parent object to the global list
      parent_object_node_t *node =
          (parent_object_node_t *)malloc(sizeof(parent_object_node_t));
      if (node == NULL) {
        CcspTraceError(("Memory allocation failed for parent object node\n"));
        free(parent_object.parameters);
        return;
      }
      node->parent = parent_object;
      node->next = parent_objects;
      parent_objects = node;

    } else if (cJSON_IsObject(child)) {
      char new_path[MAX_PARAM_NAME_LEN];
      snprintf(new_path, sizeof(new_path), "%s.%s", path, child->string);
      process_object_recursive(child, new_path, get_handler, set_handler);
    }
    child = child->next;
  }
}

int ccsp_dml_init(const char *component_name, const char *json_file_path,
                  rbusGetHandler_t get_handler, rbusSetHandler_t set_handler) {
  int rc = 0;

  // TODO: Switch-case on different DML types? In this case, we'd just have
  // RBUS, but it might be good to implement agnostically? That might be
  // overkill.
  CcspTraceInfo(("Initializing RBUS component: %s\n", component_name));

  /* Open RBUS first - needed before CosaSecurityInitialize */
  rc = rbus_open(&g_rbus_handle, component_name);
  if (rc != RBUS_ERROR_SUCCESS) {
    CcspTraceError(("rbus_open failed: %d\n", rc));
    return -1;
  }

  FILE *file = fopen(json_file_path, "r");
  if (!file) {
    int saved_errno = errno;
    CcspTraceError(("Cannot open JSON config file: path=%s err=%s\n",
                    json_file_path, strerror(saved_errno)));
    return -1;
  }

  fseek(file, 0, SEEK_END);
  long file_size = ftell(file);
  if (file_size < 0) {
    CcspTraceError(("Failed to get file size for: %s\n", json_file_path));
    fclose(file);
    return -1;
  }
  fseek(file, 0, SEEK_SET);

  char *json_buffer = (char *)malloc(file_size + 1);
  if (!json_buffer) {
    CcspTraceError(("Memory allocation failed for JSON buffer\n"));
    fclose(file);
    return -1;
  }

  size_t bytes_read = fread(json_buffer, 1, file_size, file);
  fclose(file);

  if ((long)bytes_read != file_size) {
    CcspTraceError(("Failed to read complete JSON file: %s\n", json_file_path));
    free(json_buffer);
    return -1;
  }

  json_buffer[file_size] = '\0';

  cJSON *root = cJSON_Parse(json_buffer);
  if (!root) {
    const char *error_ptr = cJSON_GetErrorPtr();
    fprintf(stderr, "Error: JSON parse failed: %s\n",
            error_ptr ? error_ptr : "unknown");
    free(json_buffer);
    return -1;
  }

  CcspTraceInfo(("Successfully parsed JSON config file: %s\n", json_file_path));

  // Prepare linked list for parent objects
  parent_objects = malloc(sizeof(parent_object_node_t));
  if (parent_objects == NULL) {
    CcspTraceError(("Memory allocation failed for parent objects list\n"));
    cJSON_Delete(root);
    free(json_buffer);
    return -1;
  }

  /* Get Device.DeviceInfo section */
  cJSON *device = cJSON_GetObjectItem(root, "Device");
  if (device) {
    cJSON *device_info = cJSON_GetObjectItem(device, "DeviceInfo");
    if (device_info) {
      process_object_recursive(device_info, "Device.DeviceInfo", get_handler,
                               set_handler);
    } else {
      process_object_recursive(device, "Device", get_handler, set_handler);
    }
  }

  cJSON_Delete(root);
  free(json_buffer);

  CcspTraceInfo(("CCSP component JSON config processing complete\n"));
  return 0;
}

/**
 * Terminate RBUS - called from cleanup
 */
void ccsp_dml_shutdown(void) {
  if (g_rbus_handle) {
    CcspTraceInfo(("Terminating RBUS\n"));

    /* Close RBUS */
    rbus_close(g_rbus_handle);
    g_rbus_handle = NULL;

    /* Free metadata and registered elements */
    // advsec_free_param_metadata();
    // advsec_free_registered_elements();
  }
}

parent_object_t *get_parent_object(const char *name) {
  parent_object_node_t *current = parent_objects;
  while (current) {
    CcspTraceDebug(("Checking parent object: %s against %s\n",
                    current->parent.name, name));
    if (strcmp(current->parent.name, name) == 0) {
      return &current->parent;
    }
    current = current->next;
  }

  return NULL;
}
