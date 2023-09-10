/* lide210.c
 *
 * This simple demo shows polling of Canon's Lide210 scanner buttons
 *
 * The protocol is very simple:
 * The scanner implements a seperate input endpoint (something the host receives).
 * From its usb descriptor it is obvious interrupt EP3 - one byte in length.
 * The buttons are simply mapped to individual bits in this byte.
 * A pressed button corresponds to its bit set. If multiple buttons are pressed during
 * one report interval (8ms)  - multiple bits are set.
 * Until the button is released, it is only reported once...
 *
 * by S. Bärwolf, Rudolstadt 2023
 *
 * please compile with: gcc -o /tmp/lide210 lide210.c -lusb -DMYDEBUG
 */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <stdbool.h>
#include <stdint.h>
#include <stdatomic.h>
#include <inttypes.h>
#include <errno.h>


//ID 04a9:190a Canon, Inc. CanoScan LiDE 210
#define USBDEV_VENDOR_CANON     0x04a9  /* Canon Inc. */
#define USBDEV_PRODUCT_LIDE210  0x190a  /* CanoScan LiDE 210 */

#define LIDE210_POLLINTERVAL_MS 100

#define LIDE210_BUTTON_PDF      (UINT8_C(0x10)) /* most left button */
#define LIDE210_BUTTON_PDFNEXT  (UINT8_C(0x01))
#define LIDE210_BUTTON_AUTOSCAN (UINT8_C(0x02)) /* middle button */
#define LIDE210_BUTTON_COPY     (UINT8_C(0x04))
#define LIDE210_BUTTON_EMAIL    (UINT8_C(0x08)) /* most right button */

#include <usb.h>


#define assigned(x) (x!=NULL)
#define ISPRESSED(x,y) ((((uint8_t)x)&((uint8_t)y))!=0)

#ifdef MYDEBUG
#	define fdebugf(file, args...) fprintf(file, ##args)
#else
#	define fdebugf(file, args...)
#endif


static atomic_bool global_doexit;
void exithandler(int sig, siginfo_t *info, void *ucontext) {
  atomic_store(&global_doexit, true);
}
void install_handlers(void) {
    struct sigaction sig;

    atomic_store(&global_doexit, false);
    memset(&sig, 0, sizeof(sig));
    sig.sa_sigaction=exithandler;
    sig.sa_flags=0;
    sigemptyset(&sig.sa_mask);
    sigaddset(&sig.sa_mask, SA_ONSTACK);
    sigaddset(&sig.sa_mask, SA_SIGINFO);

    if (sigaction(SIGINT, &sig, NULL) == -1)
        atomic_store(&global_doexit, true);

    if (sigaction(SIGTERM, &sig, NULL) == -1)
        atomic_store(&global_doexit, true);
}

int main(int argc, char **argv) {
    struct usb_bus      *bus;
    struct usb_device   *dev;

    usb_init();
    install_handlers();

    usb_find_busses();
    usb_find_devices();
    for (bus=usb_get_busses(); bus; bus=bus->next) {
        for (dev=bus->devices; dev; dev=dev->next) {
            //for now just use the first scanner found
            if ((dev->descriptor.idVendor == USBDEV_VENDOR_CANON) && (dev->descriptor.idProduct == USBDEV_PRODUCT_LIDE210)) {
                usb_dev_handle *handle = NULL;
                handle = usb_open(dev);
                if (assigned(handle)) {
                    fdebugf(stderr,"using scanner on bus=%"PRIu32", dev=%"PRIu8"  ...\n", bus->location, dev->devnum);
                    while (true) {
                        int i;
                        uint8_t data;
                        bool __doexit = atomic_load(&global_doexit);
                        if (__doexit) break;

                        i=usb_interrupt_read(handle , USB_ENDPOINT_IN | 3, &data, sizeof(data), (LIDE210_POLLINTERVAL_MS));
                        if (i < 0) {
                            if (i != -ETIMEDOUT) {
                                fprintf(stderr,"error %i received - %s", i, usb_strerror());
                                break;
                            }
                        }
                        if (i == 1) {
                            fdebugf(stderr,"\ndata=0x%02x ( ", data);
                            if (ISPRESSED(data, LIDE210_BUTTON_PDF))      fdebugf(stderr,"PDF ");
                            if (ISPRESSED(data, LIDE210_BUTTON_PDFNEXT))  fdebugf(stderr,"Next ");
                            if (ISPRESSED(data, LIDE210_BUTTON_AUTOSCAN)) fdebugf(stderr,"Autoscan ");
                            if (ISPRESSED(data, LIDE210_BUTTON_COPY))     fdebugf(stderr,"Copy ");
                            if (ISPRESSED(data, LIDE210_BUTTON_EMAIL))    fdebugf(stderr,"EMail ");
                            fdebugf(stderr,")\n");
                        } else {
                            fdebugf(stderr,".");
                            fflush(stderr);
                        }
                    }
                    fdebugf(stderr,"\nreleasing scanner on bus=%"PRIu32", dev=%"PRIu8"  ...bye...\n", bus->location, dev->devnum);
                    usb_close(handle);
                }

                break; // do not process further lide210 scanners if there is another on on the usb
            }
        }
    }

}
