/*
 * Contexte USB device (nouveau stack Zephyr, device_next).
 */
#ifndef TRACKBALL_USB_H
#define TRACKBALL_USB_H

#include <zephyr/usb/usbd.h>

/*
 * Configure et initialise le peripherique USB (descripteurs, configuration
 * pleine vitesse, enregistrement des classes disponibles : HID + CDC-ACM).
 * Renvoie le contexte pret a etre active par usbd_enable(), ou NULL en cas
 * d'erreur.
 */
struct usbd_context *app_usbd_init(void);

#endif /* TRACKBALL_USB_H */
