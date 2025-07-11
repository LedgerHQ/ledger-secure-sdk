#ifndef ST33K1M5
#define ST33K1M5
#endif  // ST33K1M5

#ifndef TARGET_ID
#define TARGET_ID 0x33400004
#endif  // TARGET_ID

#ifndef TARGET_APEX_P
#define TARGET_APEX_P
#endif  // TARGET_APEX_P

// Apex+ and Apex- share this TARGET_APEX flag
// This definition must remain after `TARGET_APEX_P` definition
#ifndef TARGET_APEX
#define TARGET_APEX
#endif  // TARGET_APEX
