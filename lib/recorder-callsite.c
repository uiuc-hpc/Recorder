/* Call site tracking: each interceptor records its caller's return address,
 * interned into a per-rank (module, offset) table.
 *
 * Two things that fail silently if changed:
 *   - offset is against link_map->l_addr (the slide), not Dl_info.dli_fbase.
 *     Non-PIE main programs have dli_fbase 0x400000 but slide 0.
 *   - we store ip-1; a return address points past the call.
 *
 * The MPI wrappers put the prologue in imp_*() behind a forwarder, so there
 * __builtin_return_address(0) lands inside librecorder. Only in that case do
 * we unwind out. */
#define _GNU_SOURCE
#define UNW_LOCAL_ONLY
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>
#include <limits.h>
#include <unistd.h>
#include <dlfcn.h>
#include <link.h>
#include <libunwind.h>
#include <pthread.h>
#include "recorder.h"

#define CALLSITE_UNKNOWN 0xFFFFFFFFu

typedef struct CallSiteModule_t {
    char*    path;              /* key */
    uint32_t id;
    size_t   path_len;          /* including the NUL */
    UT_hash_handle hh;
} CallSiteModule;

typedef struct CallSite_t {
    const void* ip;             /* key: already ip-1 */
    uint32_t    id;
    uint32_t    module_id;
    uint64_t    offset;
    UT_hash_handle hh;
} CallSite;

/* dladdr1/realpath can reach intercepted functions. */
static __thread int in_intern = 0;

static pthread_mutex_t  cs_mutex = PTHREAD_MUTEX_INITIALIZER;
static bool             cs_enabled      = false;
static CallSite*        cs_table        = NULL;
static CallSiteModule*  cs_modules      = NULL;
static uint32_t         cs_count        = 0;
static uint32_t         cs_module_count = 0;
static size_t           cs_strbytes     = 0;

/* Our own address span, to spot returns into our forwarders. */
static uintptr_t self_start = 0, self_end = 0;

struct self_probe { uintptr_t addr, start, end; };

static int self_phdr_cb(struct dl_phdr_info* info, size_t size, void* data)
{
    (void)size;
    struct self_probe* sp = (struct self_probe*) data;
    uintptr_t lo = UINTPTR_MAX, hi = 0;
    int hit = 0;
    for (int i = 0; i < info->dlpi_phnum; i++) {
        const ElfW(Phdr)* p = &info->dlpi_phdr[i];
        if (p->p_type != PT_LOAD) continue;
        uintptr_t s = info->dlpi_addr + p->p_vaddr;
        uintptr_t e = s + p->p_memsz;
        if (s < lo) lo = s;
        if (e > hi) hi = e;
        if (sp->addr >= s && sp->addr < e) hit = 1;
    }
    if (hit) { sp->start = lo; sp->end = hi; return 1; }
    return 0;
}

void callsite_set_enabled(bool enabled)
{
    cs_enabled = enabled;
    if (!enabled || self_end) return;
    struct self_probe sp = { (uintptr_t)(void*)callsite_set_enabled, 0, 0 };
    if (dl_iterate_phdr(self_phdr_cb, &sp)) {
        self_start = sp.start;
        self_end   = sp.end;
    }
}

static inline int is_self(const void* p)
{
    uintptr_t a = (uintptr_t) p;
    return self_end && a >= self_start && a < self_end;
}

/* First frame outside librecorder. */
static const void* unwind_out_of_self(void)
{
    unw_context_t ctx;
    unw_cursor_t  cur;
    if (unw_getcontext(&ctx) != 0) return NULL;
    if (unw_init_local(&cur, &ctx) != 0) return NULL;

    for (int n = 0; n < 64 && unw_step(&cur) > 0; n++) {
        unw_word_t ip = 0;
        unw_get_reg(&cur, UNW_REG_IP, &ip);
        if (!ip) break;
        const void* p = (const void*)((uintptr_t)ip - 1);
        if (!is_self(p)) return p;
    }
    return NULL;
}

static uint32_t module_intern(const char* path)
{
    CallSiteModule* m = NULL;
    HASH_FIND_STR(cs_modules, path, m);
    if (m) return m->id;

    size_t len = strlen(path) + 1;
    m = (CallSiteModule*) recorder_malloc(sizeof(CallSiteModule));
    m->path     = (char*) recorder_malloc(len);
    memcpy(m->path, path, len);
    m->path_len = len;
    m->id       = cs_module_count++;
    cs_strbytes += len;
    HASH_ADD_KEYPTR(hh, cs_modules, m->path, len - 1, m);
    return m->id;
}

uint32_t callsite_intern(void* return_address)
{
    if (!cs_enabled || !return_address) return CALLSITE_UNKNOWN;
    if (in_intern) return CALLSITE_UNKNOWN;

    /* return address points past the call */
    const void* ip = (const void*)((const char*)return_address - 1);

    /* Behind a forwarder the address is fixed, so resolve before lookup. */
    if (is_self(ip)) {
        in_intern = 1;
        ip = unwind_out_of_self();
        in_intern = 0;
        if (!ip) return CALLSITE_UNKNOWN;
    }

    CallSite* cs = NULL;
    pthread_mutex_lock(&cs_mutex);
    HASH_FIND(hh, cs_table, &ip, sizeof(const void*), cs);
    if (cs) { uint32_t id = cs->id; pthread_mutex_unlock(&cs_mutex); return id; }
    pthread_mutex_unlock(&cs_mutex);

    in_intern = 1;
    Dl_info info;
    struct link_map* lm = NULL;
    int ok = dladdr1((void*)ip, &info, (void**)&lm, RTLD_DL_LINKMAP);
    char resolved[PATH_MAX];
    const char* path = NULL;
    if (ok && info.dli_fname && lm) {
        path = realpath(info.dli_fname, resolved) ? resolved : info.dli_fname;
    }
    in_intern = 0;
    if (!path) return CALLSITE_UNKNOWN;

    pthread_mutex_lock(&cs_mutex);
    /* another thread may have interned it while we resolved */
    HASH_FIND(hh, cs_table, &ip, sizeof(const void*), cs);
    if (cs) { uint32_t id = cs->id; pthread_mutex_unlock(&cs_mutex); return id; }

    cs = (CallSite*) recorder_malloc(sizeof(CallSite));
    cs->ip        = ip;
    cs->module_id = module_intern(path);
    /* slide, not dli_fbase */
    cs->offset    = (uint64_t)((const char*)ip - (const char*)lm->l_addr);
    cs->id        = cs_count++;
    HASH_ADD(hh, cs_table, ip, sizeof(const void*), cs);
    uint32_t id = cs->id;
    pthread_mutex_unlock(&cs_mutex);
    return id;
}

/* Ids are per-rank, so each rank needs its own table. */
void callsite_save_local(int rank, const char* traces_dir)
{
    if (!cs_enabled) return;

    char path[1280];
    snprintf(path, sizeof(path), "%s/%d.callsites", traces_dir, rank);
    FILE* f = GOTCHA_REAL_CALL(fopen)(path, "wb");
    if (!f) return;

    CallSiteSectionHeader hdr;
    hdr.version      = 1;
    hdr.num_modules  = cs_module_count;
    hdr.num_sites    = cs_count;
    hdr.strtab_bytes = (uint32_t) cs_strbytes;
    GOTCHA_REAL_CALL(fwrite)(&hdr, sizeof(hdr), 1, f);

    /* ordered by id */
    char**  paths = (char**)  calloc(cs_module_count, sizeof(char*));
    size_t* lens  = (size_t*) calloc(cs_module_count, sizeof(size_t));
    CallSiteModule *m, *mtmp;
    HASH_ITER(hh, cs_modules, m, mtmp) { paths[m->id] = m->path; lens[m->id] = m->path_len; }
    for (uint32_t i = 0; i < cs_module_count; i++)
        if (paths[i]) GOTCHA_REAL_CALL(fwrite)(paths[i], lens[i], 1, f);
    free(paths); free(lens);

    /* ordered by id */
    if (cs_count) {
        CallSiteEntry* tab = (CallSiteEntry*) calloc(cs_count, sizeof(CallSiteEntry));
        CallSite *cs, *cstmp;
        HASH_ITER(hh, cs_table, cs, cstmp) {
            tab[cs->id].module_id = cs->module_id;
            tab[cs->id].offset    = cs->offset;
        }
        GOTCHA_REAL_CALL(fwrite)(tab, sizeof(CallSiteEntry), cs_count, f);
        free(tab);
    }
    GOTCHA_REAL_CALL(fclose)(f);
}

void callsite_cleanup()
{
    CallSite *cs, *cstmp;
    HASH_ITER(hh, cs_table, cs, cstmp) {
        HASH_DEL(cs_table, cs);
        recorder_free(cs, sizeof(CallSite));
    }
    cs_table = NULL;

    CallSiteModule *m, *mtmp;
    HASH_ITER(hh, cs_modules, m, mtmp) {
        HASH_DEL(cs_modules, m);
        recorder_free(m->path, m->path_len);
        recorder_free(m, sizeof(CallSiteModule));
    }
    cs_modules = NULL;
    cs_count = 0; cs_module_count = 0; cs_strbytes = 0;
}
