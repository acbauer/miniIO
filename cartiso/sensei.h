#ifdef __cplusplus
#include <stdint.h> /* C++ doesn't know the uint64_t type */

extern "C" {
#endif
void* senseiInitialize(const char* xmlFileName);
void senseiFinalize(void* cp);
void senseiOutput(void* cp, int tstep,
                  int ni, int nj, int nk, int is, int ie, int js, int je,
                  int ks, int ke, float deltax, float deltay, float deltaz,
                  float *isodata, const char* isodataname, float* perlindata,
                  const char* perlindataname);
#ifdef __cplusplus
}
#endif
