/*
 * Shim ZMK pour application Zephyr pure.
 *
 * Reproduit le strict minimum du gestionnaire d'evenements ZMK utilise par le
 * driver PMW3610 : le type d'evenement et les macros d'enregistrement
 * d'ecouteur/abonnement, ici reduites a des no-op. L'ecouteur d'activite du
 * driver ne sera donc jamais appele (pas d'endormissement automatique piloté
 * par ZMK), ce qui est sans effet sur la lecture du capteur.
 */
#ifndef ZMK_SHIM_EVENT_MANAGER_H
#define ZMK_SHIM_EVENT_MANAGER_H

typedef void zmk_event_t;

/* En ZMK, ces macros enregistrent un ecouteur dans le gestionnaire
 * d'evenements. Sans ZMK, on les neutralise. */
#define ZMK_LISTENER(mod, cb)
#define ZMK_SUBSCRIPTION(mod, ev)

#endif /* ZMK_SHIM_EVENT_MANAGER_H */
