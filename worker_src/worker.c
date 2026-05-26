#define PBL_WORKER
#include "../src/morpheuz.h" // Includiamo morpheuz.h per le costanti STEP_GOAL e NUM_DAILY_STEP_HISTORY_DAYS

/*
 * Semplice rilevamento passi per il worker (Aplite)
 * Questa funzione è una versione semplificata di quella presente in morpheuz.c
 */
static void worker_accel_data_handler(AccelData *data, uint32_t num_samples) {
  // Leggi lo stato precedente del rilevamento passi e il conteggio attuale
  bool step_high = persist_read_bool(PERSIST_STEP_HIGH_KEY);
  uint32_t current_steps = persist_read_int(PERSIST_STEPS_KEY);

  for (uint32_t i = 0; i < num_samples; i++) {
    // Calcoliamo la magnitudo vettoriale approssimativa
    int32_t magnitude = (data[i].x * data[i].x) + (data[i].y * data[i].y) + (data[i].z * data[i].z);
    if (!step_high && magnitude > 1500000) { // Superamento soglia (~1.2G)
      current_steps++;
      step_high = true;
    } else if (step_high && magnitude < 1000000) { // Ritorno a riposo (~1.0G)
      step_high = false;
    }
  }

  // Scrivi i passi aggiornati e lo stato del rilevamento nella memoria persistente
  persist_write_int(PERSIST_STEPS_KEY, current_steps);
  persist_write_bool(PERSIST_STEP_HIGH_KEY, step_high);

  // Controlla se l'obiettivo passi è stato raggiunto (il worker aggiorna solo il flag)
  if (current_steps >= STEP_GOAL) {
    if (!persist_exists(PERSIST_GOAL_MET_KEY) || !persist_read_bool(PERSIST_GOAL_MET_KEY)) {
      persist_write_bool(PERSIST_GOAL_MET_KEY, true); // Marca l'obiettivo come raggiunto per oggi
      // Il worker non può mostrare notifiche direttamente. L'app principale le gestirà.
    }
  }
}

static void prv_init() {
  // Inizializza il worker
  // Per Aplite, ci abboniamo direttamente ai dati dell'accelerometro nel worker
  accel_data_service_subscribe(25, worker_accel_data_handler);
  accel_service_set_sampling_rate(ACCEL_SAMPLING_10HZ);
  int i;

  // Inizializza le chiavi persistenti se non esistono (inclusi i nuovi)
  if (!persist_exists(PERSIST_STEPS_KEY)) {
    persist_write_int(PERSIST_STEPS_KEY, 0);
  }
  if (!persist_exists(PERSIST_STEP_HIGH_KEY)) {
    persist_write_bool(PERSIST_STEP_HIGH_KEY, false);
  }
  if (!persist_exists(PERSIST_GOAL_MET_KEY)) {
    persist_write_bool(PERSIST_GOAL_MET_KEY, false);
  }
  // Inizializza le chiavi per la cronologia passi
  if (!persist_exists(PERSIST_DAILY_STEPS_BASE_KEY)) {
      persist_write_int(PERSIST_DAILY_STEPS_BASE_KEY, 0);
  }
  for (i = 0; i < NUM_DAILY_STEP_HISTORY_DAYS; i++) {
      if (!persist_exists(PERSIST_DAILY_STEPS_0 + i)) {
          persist_write_int(PERSIST_DAILY_STEPS_0 + i, 0);
      }
  }
}

static void prv_deinit() {
  // De-inizializza i servizi alla chiusura del worker
  accel_data_service_unsubscribe();
}

int main(void) {
  prv_init();
  worker_event_loop();
  prv_deinit();
  return 0;
}