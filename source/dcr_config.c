/* dcr_config.c -- Teeny Titans' settings: config.ini's
 * options, on the runtime's INI engine (runtime/source/rt_cfg.c).
 *
 * The DcrConfig fields with no option here stay at "off" for the shared code
 * that reads them. MIT.
 */
#include <stdio.h>
#include <sys/stat.h>
#include <switch.h>

#include "dcr_config.h"
#include "dcr_path.h"
#include "rt_cfg.h"
#include "rt_window.h"
#include "util.h"

static DcrConfig g_cfg = {
    .res_w = 720, .res_h = 1280, .boost = 1, .mix_48k = 1,
};

const DcrConfig *dcr_config(void) { return &g_cfg; }

static const CfgOpt k_opts[] = {
    CFG_ROW_RESOLUTION("auto",
                       "Rendering resolution: auto (1080 if docked when the game starts, 720 in\n"
                       "# handheld), 1080 or 720. The Switch scales the picture to the screen either way."),
    {"display", "tate_rotation", "right",
     "The game is portrait: hold the console upright. right: its right side up.\n"
     "# left: its left side up.",
     CFG_CHOICE, "right,left", &g_cfg.tate_left},
    CFG_ROW_BOOST("CPU at 1785 MHz while the game starts (until its title, through the\n"
                  "# logos) and inside loading frames (those over 50 ms), normal otherwise.",
                  &g_cfg.boost),
    {"audio", "mix_at_48khz", "true",
     "Mix the game's audio at 48 kHz, as phones do. Off: 24 kHz, the Android\n"
     "# fallback's rate.",
     CFG_BOOL, NULL, &g_cfg.mix_48k},
    CFG_ROW_GL_SELFTEST(&g_cfg.gl_selftest),
    {"debug", "profile_long_frames", "false",
     "Write where the time goes in frames over 100 ms (loading) to debug.log.",
     CFG_BOOL, NULL, &g_cfg.profile},
    CFG_ROW_BOOT_LOG("Show the start-up log on screen at every launch. Off: the screen stays\n"
                     "# dark until the game draws, and the log appears only while something is\n"
                     "# being set up (first launch, a new game.apk or NRO).",
                     &g_cfg.boot_log),
    {"debug", "log_sounds", "false",
     "Write every sound the game plays to debug.log (for a bug report about audio).",
     CFG_BOOL, NULL, &g_cfg.log_sounds},
};

static const CfgMigrate k_migrate[] = {
    {"display", "resolution", "auto", "auto", 1},
};

static void apply(void) {
  const RtConfig *rt = rt_config();
  /* a portrait window; the picture is turned on the way out (dcr_boot.c) */
  g_cfg.res_w = rt->res_h;
  g_cfg.res_h = rt->res_w;
  dcr_window_set_size(g_cfg.res_w, g_cfg.res_h);
  int docked = appletGetOperationMode() == AppletOperationMode_Console;
  debugPrintf("[config] %dx%d (%s, %s), CPU boost %s, 48 kHz %s, profiler %s, sound log %s\n", g_cfg.res_w,
              g_cfg.res_h, rt_config_get("display", "resolution"), docked ? "docked" : "handheld",
              g_cfg.boost ? "on" : "off", g_cfg.mix_48k ? "on" : "off", g_cfg.profile ? "on" : "off",
              g_cfg.log_sounds ? "on" : "off");
}

static const CfgTable k_table = {
    .opts = k_opts,
    .nopts = CFG_COUNT(k_opts),
    .migrate = k_migrate,
    .nmigrate = CFG_COUNT(k_migrate),
    .version = 1,
    .apply = apply,
};

void dcr_config_load(void) { rt_config_load(&k_table); }

/* First start: nothing to carry over. */
void port_config_new_file(void) {}
