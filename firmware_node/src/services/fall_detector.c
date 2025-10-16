#include "firmware_node/src/services/fall_detector.h"
#include "config/fall_params.h"

static fall_cfg_t s_cfg;

void fall_detector_init(const fall_cfg_t* cfg) {
  if (cfg) {
    s_cfg = *cfg;
  } else {
    s_cfg.Athr_centi_g = FALL_A_THR_CENTI_G;
    s_cfg.Ithr_centi_g = FALL_I_THR_CENTI_G;
    s_cfg.Tidle_ms     = FALL_IDLE_MS;
    s_cfg.fs_hz        = FALL_FS_HZ;
  }
}

bool fall_detector_feed(const accel_raw_t* s, fall_event_t* out_event) {
  (void)s;
  (void)out_event;
  // MVP: stub. Implementar lógica: detectar pico > Athr y luego inactividad
  // durante Tidle_ms (usando contador de muestras basado en fs_hz).
  // Al confirmar, llenar out_event y retornar true.
  return false;
}

