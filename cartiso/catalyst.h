#ifdef __cplusplus
#include <stdint.h> /* C++ doesn't know the uint64_t type */

extern "C" {
#endif
void* catalystInitialize(const char* scriptName);
void catalystFinalize(void* cp);
void catalystOutput(void* cp, int tstep, int ni, int nj, int nk, int is, int ie, int js, int je,
                    int ks, int ke, float deltax, float deltay, float deltaz, float *data, const char* dataName,
                    float* xdata, const char* xdataName, uint64_t ntris, float *points, float *norms,
                    float *trixdata, const char *trixdataName);
#ifdef __cplusplus
}
#endif
