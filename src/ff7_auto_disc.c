/* Original NTSC-U DSCHANGE.X, identical on all three owned discs.
 * See docs/AUTO_DISC.md for the disassembly evidence and live validation. */
#include "mod_plugins.h"

enum {
    FF7_REQUESTED_DISC = 0x8009D588u,
    FF7_CD_STATE = 0x80071A60u,
    FF7_CD_STOPPED = 7u,
};

static int ff7_disc_request(int *ready) {
    *ready = 0;
    if (!psx_mod_game_started()) return 0;
    /* Overlay identity matters: FIELD, BATTLE and DSCHANGE reuse this slot.
     * Check the prologue, lid-wait loop and requested-disc comparison. */
    if (psx_mod_read_word(0x800A0000u) != 0x27BDFFA8u ||
        psx_mod_read_word(0x800A0008u) != 0x0080F021u ||
        psx_mod_read_word(0x800A08A4u) != 0x0C00F840u ||
        psx_mod_read_word(0x800A08B4u) != 0x30420010u ||
        psx_mod_read_word(0x800A0C0Cu) != 0x145EFE1Cu) return 0;
    const uint32_t requested = psx_mod_read_byte(FF7_REQUESTED_DISC);
    if (requested < 1u || requested > 3u) return 0;
    /* DSCHANGE loads both images and completes its fade before 0x80034104
     * stops the CD and publishes state 7. Only then does its lid-wait loop
     * begin. Keep the request alive through validation for retry suppression. */
    *ready = psx_mod_read_word(FF7_CD_STATE) == FF7_CD_STOPPED;
    return (int)requested;
}

static void ff7_auto_disc_activate(void) {
    psx_mod_set_disc_request_provider(ff7_disc_request);
}
PSX_MOD_CONSTRUCTOR(ff7_register_auto_disc) {
    (void)psx_mod_register_activation_plugin("ff7.auto-disc", ff7_auto_disc_activate);
}
