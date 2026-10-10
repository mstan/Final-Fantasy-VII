#ifndef FF7_PARTY_GAME_H
#define FF7_PARTY_GAME_H

#include "ff7_party.h"

enum { FF7_PARTY_GUEST_FIRST = 11, FF7_PARTY_GUEST_EMPTY = 255,
       FF7_PARTY_MOTION_UNCHANGED = 0, FF7_PARTY_MOTION_SEPHIROTH = 1 };

/* Immutable asset/rule descriptors, registered before activation. Canonical
 * identities are persistent; guest proxies are session-local storage handles.
 * presentation/compatibility/growth select a stock rule set, never a record.
 * Distinct added members may share a rule set while owning different assets,
 * stats, equipment, materia and progress. Future asset/animation/limit adapters
 * extend this descriptor rather than borrowing a native character's storage.
 * Descriptors and strings must have static lifetime. */
typedef struct FF7PartyProfile {
    uint16_t identity;
    uint8_t presentation;
    uint8_t kernel_template_row;
    uint8_t weapon_compatibility;
    uint8_t armor_compatibility;
    uint8_t accessory_compatibility;
    uint8_t growth_identity;
    const char* battle_model;
    const char* portrait;
    uint8_t motion_policy; /* Asset-format adapter; independent of identity. */
    const char* field_source; /* Optional original DAT/BSX stem, e.g. FIELD/NIVL. */
    uint8_t field_model_index; /* Asset index only; never a script entity ID. */
    uint8_t limit_policy; /* Independent move/progression policy; zero unchanged. */
    uint8_t initial_level_offset; /* Fresh record: growth_identity's level + offset;
                                   * zero keeps the kernel template. Saved growth stays owned. */
} FF7PartyProfile;

int ff7_party_game_register_profile(const FF7PartyProfile* profile);
const FF7PartyProfile* ff7_party_game_profile(uint8_t guest);
int ff7_party_game_ready(void);
unsigned ff7_party_game_extra_count(void);
uint8_t ff7_party_game_extra_guest(unsigned index);
uint16_t ff7_party_game_identity(uint8_t guest);
uint8_t ff7_party_game_guest_id(uint16_t identity);
uint32_t ff7_party_game_record(uint8_t guest);
uint8_t ff7_party_game_presentation(uint8_t guest);
int ff7_party_game_is_extra(uint8_t guest);
int ff7_party_game_select(unsigned slot, uint16_t identity);
/* Read all three bytes after a native PHS swap; do not serialize its two
 * intermediate stores as independent party selections. */
void ff7_party_game_observe_active(void);

/* Emulation-thread save lifecycle API. Guest enhancement RAM is the authority;
 * ordinary guest writes to extra records are included in snapshots/rewind.
 * Load validates everything before publishing records or live party bytes.
 * fallback contains only native guest IDs/255 for a standard-save projection.
 * Unknown profile IDs are refused, retaining the previous state. */
FF7PartyResult ff7_party_game_snapshot(FF7PartyState* out);
FF7PartyResult ff7_party_game_load(const FF7PartyState* state,
                                 const uint8_t fallback[3]);
int ff7_party_game_fallback(uint8_t out[3]);
/* Fresh templates for a new game or an ordinary native save. This resets
 * added progress explicitly rather than retaining a previous session's data. */
int ff7_party_game_initialize(const uint8_t fallback[3]);

/* Shared menu/battle instruction adapters. Alias scratch contains the proxy,
 * not a forged native index; final 132-byte stride hooks replace its offset.
 * Native alias entries and records remain unchanged. */
uint32_t ff7_party_game_alias_word(uint8_t guest);
uint32_t ff7_party_game_record_offset(uint8_t guest);

#endif
