#include "ccsp_dml_lib.h"

// Global RBUS handle
static rbusHandle_t g_rbus_handle = NULL;

int ccsp_dml_init(const char *component_name, const char *json_file_path) {
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

  return 0;
}
