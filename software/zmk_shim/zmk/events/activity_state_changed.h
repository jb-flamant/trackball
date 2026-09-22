/*
 * Shim ZMK pour application Zephyr pure.
 *
 * Fournit les types de l'evenement « changement d'etat d'activite » attendus
 * par le driver PMW3610. L'accesseur renvoie NULL : combine aux macros no-op de
 * event_manager.h, le callback correspondant n'est jamais invoque.
 */
#ifndef ZMK_SHIM_ACTIVITY_STATE_CHANGED_H
#define ZMK_SHIM_ACTIVITY_STATE_CHANGED_H

#include <zmk/event_manager.h>

enum zmk_activity_state {
	ZMK_ACTIVITY_ACTIVE,
	ZMK_ACTIVITY_IDLE,
	ZMK_ACTIVITY_SLEEP,
};

struct zmk_activity_state_changed {
	enum zmk_activity_state state;
};

static inline struct zmk_activity_state_changed *
as_zmk_activity_state_changed(const zmk_event_t *eh)
{
	(void)eh;
	return (struct zmk_activity_state_changed *)0;
}

#endif /* ZMK_SHIM_ACTIVITY_STATE_CHANGED_H */
