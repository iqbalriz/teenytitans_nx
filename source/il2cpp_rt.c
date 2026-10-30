/* il2cpp_rt.c -- what IL2CPP's garbage collector needs from the OS, and does
 * not get from Horizon.
 *
 * STOP-THE-WORLD. libil2cpp's Boehm collector stops every other thread with
 * tgkill(pid, tid, SIG_SUSPEND) and restarts them with SIG_THR_RESTART; each
 * thread's signal handler records where its stack is, acknowledges on
 * GC_suspend_ack_sem and sleeps in sigsuspend until the world restarts. Horizon
 * has no signals, so the handler's work is done FOR the thread, from the
 * collector, as the handler in this libil2cpp does it (libil2cpp+0x1049d94):
 *
 *   suspend:  find the thread's GC_Thread in GC_threads[id & 0xff]; if its
 *             last_stop_count is GC_stop_count, stop (a retry for a thread
 *             already stopped). Otherwise pause the thread
 *             (svcSetThreadActivity), read its registers, push r0-r12 and lr
 *             below its sp so the conservative scan sees them, publish that as
 *             stack_ptr, post the semaphore, set last_stop_count.
 *   restart:  resume the thread. This collector waits for no acknowledgement.
 *
 * Nothing in this path may log or allocate: another thread may be paused while
 * holding the log's or the allocator's lock. Counters are reported later, from
 * the main loop.
 *
 * The addresses are for Teeny Titans 1.2.7's libil2cpp (Unity 2017.4.36f1). The
 * handler's first words are checked before any of them is used. MIT.
 */
#include <stdio.h>
#include <string.h>
#include <switch.h>

#include "bionic_pthread.h"
#include "dcr_patches.h"
#include "imports.h"
#include "so_util.h"
#include "util.h"

extern so_module il2cpp_mod;

#define RVA_SUSPEND_HANDLER 0x1049d94u
static const uint32_t k_guard_handler[4] = {0xe92d48f0u, 0xe28db010u, 0xe24dd008u, 0xe1a04000u};
#define RVA_STOP_COUNT  0x13ff510u
#define RVA_SIG_SUSPEND 0x13e1470u
#define RVA_SIG_RESTART 0x13e1474u
#define RVA_GC_THREADS  0x13ff520u
#define RVA_ACK_SEM     0x1405958u

/* GC_Thread */
typedef struct GcThread {
  struct GcThread *next;
  uintptr_t id; /* pthread_t */
  int tid;
  volatile uint32_t last_stop_count;
  void *volatile stack_ptr;
} GcThread;

typedef int (*fn_sem_post)(void *sem);

static fn_sem_post g_sem_post;
static volatile uint32_t *g_stop_count;
static volatile int *g_sig_suspend, *g_sig_restart;
static GcThread **g_threads;
static void *g_ack_sem;
static int g_gc_ready;

static volatile uint32_t g_n_suspend, g_n_restart, g_n_retry, g_n_ctx_fail, g_n_unknown;
uint32_t dcr_gc_suspends(void) { return g_n_suspend; }

/* svcGetThreadContext3 with the kernel's own 0x320-byte ThreadContext. For an
 * AArch32 thread the kernel fills r[0..14] (sp = r[13], lr = r[14]). */
typedef struct {
  uint64_t r[29];
  uint64_t fp, lr, sp, pc;
  uint32_t psr, _pad;
  uint8_t v[32][16];
  uint32_t fpcr, fpsr;
  uint64_t tpidr;
} KThreadContext;
_Static_assert(sizeof(KThreadContext) == 0x320, "kernel ThreadContext is 0x320 bytes");

static Result get_thread_context(KThreadContext *ctx, Handle h) {
  register uint32_t r0 __asm__("r0") = (uint32_t)(uintptr_t)ctx;
  register uint32_t r1 __asm__("r1") = h;
  /* The 32-bit SVC ABI returns with r1-r3 zeroed: they must be clobbers. */
  __asm__ volatile("svc 0x33" : "+r"(r0), "+r"(r1) : : "r2", "r3", "r12", "lr", "memory");
  return r0;
}

int dcr_il2cpp_install(void) {
  const uintptr_t b = (uintptr_t)il2cpp_mod.load_virtbase;
  if (dcr_guard_ok(&il2cpp_mod, RVA_SUSPEND_HANDLER, k_guard_handler, "GC suspend handler")) {
    g_stop_count = (volatile uint32_t *)(b + RVA_STOP_COUNT);
    g_sig_suspend = (volatile int *)(b + RVA_SIG_SUSPEND);
    g_sig_restart = (volatile int *)(b + RVA_SIG_RESTART);
    g_threads = (GcThread **)(b + RVA_GC_THREADS);
    g_ack_sem = (void *)(b + RVA_ACK_SEM);
    g_sem_post = (fn_sem_post)dcr_import_lookup("sem_post");
    g_gc_ready = g_sem_post != NULL;
  }
  debugPrintf("[il2cpp] GC bridge %s\n", g_gc_ready ? "armed" : "NOT ARMED -- the first collection will hang");
  return g_gc_ready;
}

static GcThread *gc_lookup(const BThread *t) {
  GcThread *me = g_threads[(uintptr_t)t & 0xff];
  while (me && me->id != (uintptr_t)t)
    me = me->next;
  return me;
}

static int in_stack(const BThread *t, uintptr_t p) {
  uintptr_t lo = (uintptr_t)t->stack_base;
  return lo && p >= lo && p <= lo + t->stack_size;
}

static void gc_suspend(BThread *t) {
  GcThread *me = gc_lookup(t);
  if (!me) {
    g_n_unknown++;
    return; /* not a GC thread: the collector does not wait for it */
  }
  const uint32_t stop = *g_stop_count;
  if (me->last_stop_count == stop) {
    g_n_retry++;
    return;
  }
  KThreadContext ctx;
  uintptr_t sp = 0;
  b_pause_lock();
  svcSetThreadActivity(t->handle, ThreadActivity_Paused);
  t->gc_paused = 1;
  if (R_SUCCEEDED(get_thread_context(&ctx, t->handle)))
    sp = (uintptr_t)ctx.r[13];
  b_pause_unlock();
  if (!in_stack(t, sp)) {
    /* Unknown sp: scan the whole stack. Conservative, never unsafe. */
    g_n_ctx_fail++;
    me->stack_ptr = t->stack_base;
  } else {
    uint32_t *save = (uint32_t *)((sp - 16 * 4) & ~7u);
    for (int i = 0; i < 13; i++)
      save[i] = (uint32_t)ctx.r[i];
    save[13] = (uint32_t)ctx.r[14];
    me->stack_ptr = save;
  }
  me->last_stop_count = stop;
  __atomic_thread_fence(__ATOMIC_SEQ_CST);
  g_n_suspend++;
  g_sem_post(g_ack_sem);
}

static void gc_restart(BThread *t) {
  b_pause_lock();
  if (t->gc_paused) {
    t->gc_paused = 0;
    __atomic_thread_fence(__ATOMIC_SEQ_CST);
    svcSetThreadActivity(t->handle, ThreadActivity_Runnable);
  }
  b_pause_unlock();
  g_n_restart++;
}

/* Called by bionic_signal.c for every tkill/pthread_kill. */
int port_gc_signal(BThread *t, int sig) {
  if (!g_gc_ready || !t)
    return 0;
  if (sig == *g_sig_suspend) {
    gc_suspend(t);
    return 1;
  }
  if (sig == *g_sig_restart) {
    gc_restart(t);
    return 1;
  }
  return 0;
}

int b_tkill(int tid, int sig);
int b_tgkill(int pid, int tid, int sig) {
  (void)pid;
  return b_tkill(tid, sig);
}

void dcr_il2cpp_report(void) {
  if (!g_n_suspend && !g_n_unknown)
    return;
  debugPrintf("[il2cpp] GC bridge: %lu suspends, %lu restarts, %lu retries, %lu full-stack scans, %lu non-GC threads\n",
              (unsigned long)g_n_suspend, (unsigned long)g_n_restart, (unsigned long)g_n_retry,
              (unsigned long)g_n_ctx_fail, (unsigned long)g_n_unknown);
}

/* ---------------------------------------------------------- VideoPlayer
 * The intro (InitController.PlayIntroVideo) is a UnityEngine.Video.VideoPlayer
 * on a URL. Its Android backend needs MediaExtractor and MediaCodec, which are
 * not here: the game would wait on a player that never starts. So the two
 * managed entry points the game uses are replaced: set_url notes the file,
 * Play hands it to su_video.c, played like a full-screen movie by the frame
 * loop, and dcr_il2cpp_video_done() then raises loopPointReached, which is
 * what the game waits for. The addresses are as for the GC, above. */
#define RVA_VP_SET_URL 0xfe3650u
#define RVA_VP_PLAY    0xfe3748u
#define RVA_VP_LOOP_POINT 0xfe42bcu /* InvokeLoopPointReachedCallback_Internal */
static const uint32_t k_guard_set_url[4] = {0xe92d4c70u, 0xe28db010u, 0xe1a05000u, 0xe59f0058u};
static const uint32_t k_guard_play[4] = {0xe1a01000u, 0xeaffffffu, 0xe92d4830u, 0xe28db008u};

typedef struct {
  void *klass, *monitor;
  int32_t length;
  uint16_t chars[];
} Il2CppStr;

void su_video_request(const char *file, int bg, int mode, int url, int64_t offset, int64_t length);

static char g_vp_url[256];
static void *g_vp;

static void vp_set_url(void *self, const Il2CppStr *url, void *method) {
  (void)self, (void)method;
  size_t n = 0;
  for (int i = 0; url && i < url->length && n + 1 < sizeof g_vp_url; i++)
    g_vp_url[n++] = (char)url->chars[i];
  g_vp_url[n] = 0;
  debugPrintf("[video] VideoPlayer url %s\n", g_vp_url);
}

static void vp_play(void *self, void *method) {
  (void)method;
  const char *base = strrchr(g_vp_url, '/');
  if (!base)
    return;
  char path[300];
  snprintf(path, sizeof path, "Videos%s", base);
  g_vp = self;
  su_video_request(path, 0, 2 /* skippable */, 0, 0, 0);
}

void dcr_il2cpp_video_done(void) {
  if (!g_vp)
    return;
  void *vp = g_vp;
  g_vp = NULL;
  void (*invoke)(void *, void *, void *) =
      (void (*)(void *, void *, void *))((uintptr_t)il2cpp_mod.load_virtbase + RVA_VP_LOOP_POINT);
  invoke(NULL, vp, NULL);
}

void dcr_il2cpp_video_install(void) {
  const int a = dcr_hook(&il2cpp_mod, RVA_VP_SET_URL, k_guard_set_url, (void *)vp_set_url, "VideoPlayer.set_url");
  const int b = dcr_hook(&il2cpp_mod, RVA_VP_PLAY, k_guard_play, (void *)vp_play, "VideoPlayer.Play");
  debugPrintf("[il2cpp] VideoPlayer %s\n", a && b ? "goes to the movie player" : "NOT replaced");
}

/* dcr_prof.c's names for managed frames came from Mono's tables, and its
 * stop-the-world total from the Mono bridge. */
int dcr_mono_method_name(uint32_t addr, char *buf, size_t cap) {
  (void)addr, (void)buf, (void)cap;
  return 0;
}

uint64_t dcr_gc_stopped_ns(void) { return 0; }
