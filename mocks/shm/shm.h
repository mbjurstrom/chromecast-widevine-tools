
#ifdef __cplusplus
extern "C" {
#endif

#ifdef DLLEXPORTS
#define EXPORT __declspec (dllexport)
#else
#define EXPORT
#endif

/*
*/
EXPORT
int MV_SHM_Init();


#ifdef __cplusplus
}
#endif
