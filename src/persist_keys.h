#ifndef PERSIST_KEYS_H_
#define PERSIST_KEYS_H_

#define PERSIST_STEPS_KEY 12125 // Nuova chiave per i passi nel persist storage
#define PERSIST_STEP_HIGH_KEY 12126 // Chiave per lo stato step_high nel persist storage
#define PERSIST_GOAL_MET_KEY 12127 // Chiave per indicare se l'obiettivo passi è stato raggiunto oggi

// Chiavi per la cronologia dei passi giornalieri (ultimi 7 giorni)
#define PERSIST_DAILY_STEPS_BASE_KEY 12128 // time_t del giorno a cui si riferisce PERSIST_DAILY_STEPS_0
#define PERSIST_DAILY_STEPS_0 12129 // Passi del giorno più recente (ieri)
#define PERSIST_DAILY_STEPS_1 12130
#define PERSIST_DAILY_STEPS_2 12131
#define PERSIST_DAILY_STEPS_3 12132
#define PERSIST_DAILY_STEPS_4 12133
#define PERSIST_DAILY_STEPS_5 12134
#define PERSIST_DAILY_STEPS_6 12135 // Passi del giorno più vecchio (7 giorni fa)

// Chiavi per la cronologia del sonno (ultimi 7 giorni)
#define PERSIST_DAILY_LIGHT_0 12136
#define PERSIST_DAILY_LIGHT_1 12137
#define PERSIST_DAILY_LIGHT_2 12138
#define PERSIST_DAILY_LIGHT_3 12139
#define PERSIST_DAILY_LIGHT_4 12140
#define PERSIST_DAILY_LIGHT_5 12141
#define PERSIST_DAILY_LIGHT_6 12142
#define PERSIST_DAILY_DEEP_0 12143
#define PERSIST_DAILY_DEEP_1 12144
#define PERSIST_DAILY_DEEP_2 12145
#define PERSIST_DAILY_DEEP_3 12146
#define PERSIST_DAILY_DEEP_4 12147
#define PERSIST_DAILY_DEEP_5 12148
#define PERSIST_DAILY_DEEP_6 12149

#define PERSIST_LAST_MDAY 12150 // Ultimo giorno (tm_mday) in cui i passi sono stati visualizzati/gestiti

#endif /* PERSIST_KEYS_H_ */