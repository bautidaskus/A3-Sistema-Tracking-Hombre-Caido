#include "firmware_node/src/services/fall_detector.h"
#include "config/fall_params.h"

#include <stdlib.h>

typedef enum {
  FALL_STATE_WAIT_PEAK = 0,
  FALL_STATE_TRACK_IDLE
} fall_state_t;

typedef struct {
  fall_cfg_t cfg;
  fall_state_t state;
  int16_t peak_centi_g;
  uint32_t peak_epoch_ms;
  uint32_t idle_acc_ms;
  uint32_t elapsed_ms;
  uint32_t sample_period_ms;
  uint32_t sample_period_rem;
  uint32_t sample_period_rem_acc;
  bool initialized;
} fall_detector_ctx_t;

static fall_detector_ctx_t s_ctx;

static uint32_t compute_sample_period_ms(uint8_t fs_hz, uint32_t* rem_out) {
  if (fs_hz == 0) {
    if (rem_out) *rem_out = 0;
    return 10U;
  }
  const uint32_t numerator = 1000U;
  const uint32_t base = numerator / fs_hz;
  const uint32_t rem = numerator % fs_hz;
  if (rem_out) *rem_out = rem;
  return base > 0 ? base : 1U;
}

static void ctx_reset_state(void) {
  s_ctx.state = FALL_STATE_WAIT_PEAK;
  s_ctx.peak_centi_g = 0;
  s_ctx.peak_epoch_ms = 0;
  s_ctx.idle_acc_ms = 0;
}

void fall_detector_init(const fall_cfg_t* cfg) {
  if (cfg) {
    s_ctx.cfg = *cfg;
  } else {
    s_ctx.cfg.Athr_centi_g = FALL_A_THR_CENTI_G;
    s_ctx.cfg.Ithr_centi_g = FALL_I_THR_CENTI_G;
    s_ctx.cfg.Tidle_ms     = FALL_IDLE_MS;
    s_ctx.cfg.fs_hz        = FALL_FS_HZ;
  }

  s_ctx.elapsed_ms = 0;
  s_ctx.sample_period_ms = compute_sample_period_ms(s_ctx.cfg.fs_hz, &s_ctx.sample_period_rem);
  s_ctx.sample_period_rem_acc = 0U;
  s_ctx.initialized = true;
  ctx_reset_state();
}

static int16_t vector_peak_centi_g(const accel_raw_t* s) {
  int16_t ax = (int16_t)abs(s->ax);
  int16_t ay = (int16_t)abs(s->ay);
  int16_t az = (int16_t)abs(s->az);
  int16_t peak = ax;
  if (ay > peak) peak = ay;
  if (az > peak) peak = az;
  return peak;
}

static uint32_t advance_time_ms(void) {
  s_ctx.elapsed_ms += s_ctx.sample_period_ms;
  if (s_ctx.sample_period_rem != 0U && s_ctx.cfg.fs_hz != 0U) {
    s_ctx.sample_period_rem_acc += s_ctx.sample_period_rem;
    if (s_ctx.sample_period_rem_acc >= s_ctx.cfg.fs_hz) {
      s_ctx.elapsed_ms += 1U;
      s_ctx.sample_period_rem_acc -= s_ctx.cfg.fs_hz;
    }
  }
  return s_ctx.elapsed_ms;
}

bool fall_detector_feed(const accel_raw_t* s, fall_event_t* out_event) {
  if (!s || !out_event) return false;
  if (!s_ctx.initialized) fall_detector_init(NULL);

  const uint32_t sample_epoch_ms = advance_time_ms();
  const int16_t sample_peak = vector_peak_centi_g(s);

  switch (s_ctx.state) {
    case FALL_STATE_WAIT_PEAK:
      if (sample_peak >= s_ctx.cfg.Athr_centi_g) {
        s_ctx.state = FALL_STATE_TRACK_IDLE;
        s_ctx.peak_centi_g = sample_peak;
        s_ctx.peak_epoch_ms = sample_epoch_ms;
        s_ctx.idle_acc_ms = 0;
      }
      break;

    case FALL_STATE_TRACK_IDLE:
      if (sample_peak > s_ctx.peak_centi_g) {
        s_ctx.peak_centi_g = sample_peak;
        s_ctx.peak_epoch_ms = sample_epoch_ms;
      }

      if (sample_peak <= s_ctx.cfg.Ithr_centi_g) {
        uint32_t next_idle = s_ctx.idle_acc_ms + s_ctx.sample_period_ms;
        if (next_idle > 0xFFFFu) next_idle = 0xFFFFu;
        s_ctx.idle_acc_ms = next_idle;
      } else {
        s_ctx.idle_acc_ms = 0;
      }

      if (s_ctx.idle_acc_ms >= s_ctx.cfg.Tidle_ms) {
        out_event->epoch_ms = s_ctx.peak_epoch_ms;
        out_event->ax_peak_centi_g = s_ctx.peak_centi_g;
        out_event->idle_ms = (uint16_t)(s_ctx.idle_acc_ms > 0xFFFFu ? 0xFFFFu : s_ctx.idle_acc_ms);
        ctx_reset_state();
        return true;
      }

      if (sample_peak < s_ctx.cfg.Athr_centi_g && s_ctx.idle_acc_ms == 0) {
        const uint32_t window_ms = s_ctx.cfg.Tidle_ms + 100U;
        if ((sample_epoch_ms - s_ctx.peak_epoch_ms) > window_ms) {
          ctx_reset_state();
        }
      }
      break;
  }

  return false;
}
