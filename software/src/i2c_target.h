/*
 * Sortie I2C target : expose le pointage du trackball a un controleur hote.
 */
#ifndef TRACKBALL_I2C_TARGET_H
#define TRACKBALL_I2C_TARGET_H

#include <stdint.h>

/*
 * Enregistre le trackball comme peripherique I2C target (non bloquant).
 * Renvoie 0 si l'enregistrement a reussi, un code d'erreur sinon (dans ce cas
 * la sortie I2C est simplement absente, le reste du firmware continue).
 */
int app_i2c_target_init(void);

/*
 * Accumule le mouvement et met a jour l'etat des boutons, pour lecture par
 * l'hote I2C. Appelee depuis le callback input (thread input).
 *   dx, dy  : deltas bruts du capteur (indépendants du mode molette USB)
 *   buttons : bit0 gauche, bit1 droit, bit2 milieu
 */
void app_i2c_target_feed(int dx, int dy, uint8_t buttons);

#endif /* TRACKBALL_I2C_TARGET_H */
