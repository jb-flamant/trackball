/*
 * Shim ZMK pour application Zephyr pure.
 *
 * Le driver PMW3610 (module hors-arbre badjeff/zmk-pmw3610-driver) inclut
 * <zmk/keymap.h> mais n'en utilise aucun symbole. En dehors de ZMK, cet
 * en-tete n'existe pas : on fournit un stub vide.
 */
#ifndef ZMK_SHIM_KEYMAP_H
#define ZMK_SHIM_KEYMAP_H

#endif /* ZMK_SHIM_KEYMAP_H */
