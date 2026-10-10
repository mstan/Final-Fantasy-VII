#include "ff7_party_game.h"
#include "ff7_party_limit.h"
#include "ff7_party_menu.h"
#include "ff7_party_battle.h"
#include "ff7_party_game_save.h"
#include "ff7_party_field.h"
#include "ff7_party_field_loader.h"
#include "mod_plugins.h"
#include <stdlib.h>
#include <string.h>
#include <zlib.h>

/* All mutable ownership/identity/record state lives in Expansion 1, which the
 * framework captures together with RAM, devices and card extensions. Host
 * globals below hold only immutable profiles and activation-fixed pointers. */
enum {
    GAME_MAGIC = 0x47374646, GAME_VERSION = 2,
    H_MAGIC = 0, H_VERSION = 4, H_READY = 8, H_COUNT = 12,
    H_FALLBACK = 16, H_ACTIVE = 20, H_IDS = 32, H_PRESENTATIONS = 64,
    H_FLAGS = 80, H_ALIASES = 96, H_RECORDS = 256,
    GAME_BYTES = H_RECORDS + FF7_PARTY_EXTRA_CAPACITY * FF7_PARTY_RECORD_SIZE
};
static const uint32_t NATIVE_RECORDS = 0x8009c738u, ACTIVE = 0x8009cbdcu;
static const FF7PartyProfile s_sephiroth = {
    FF7_PARTY_SEPHIROTH, 10, 7, 10, 0, 0, 0,
    "ENEMY6/SEFIROS.LZS", "MENU/PCEFI.TIM", FF7_PARTY_MOTION_SEPHIROTH,
    "FIELD/NIVL", 3, FF7_LIMIT_SEPHIROTH, 3
};
static const FF7PartyProfile* s_profiles[FF7_PARTY_EXTRA_CAPACITY];
static unsigned s_profile_count;
static uint32_t s_game;
static int s_activated;

static const FF7PartyProfile* find_profile(uint16_t id) {
    for (unsigned i = 0; i < s_profile_count; ++i)
        if (s_profiles[i]->identity == id) return s_profiles[i];
    return NULL;
}

int ff7_party_game_register_profile(const FF7PartyProfile* p) {
    if (s_activated || !p || p->identity < FF7_PARTY_EXTRA_FIRST ||
        p->identity == FF7_PARTY_EMPTY || p->presentation > 10 ||
        p->kernel_template_row >= 9 || p->weapon_compatibility > 10 ||
        p->armor_compatibility > 10 || p->accessory_compatibility > 10 ||
        p->growth_identity >= 9 || !p->battle_model || !*p->battle_model ||
        !p->portrait || !*p->portrait || find_profile(p->identity) ||
        s_profile_count == FF7_PARTY_EXTRA_CAPACITY) return 0;
    s_profiles[s_profile_count++] = p;
    return 1;
}

int ff7_party_game_ready(void) {
    return s_game && psx_mod_read_word(s_game + H_MAGIC) == GAME_MAGIC &&
        psx_mod_read_word(s_game + H_VERSION) == GAME_VERSION &&
        psx_mod_read_word(s_game + H_READY) == 1 &&
        psx_mod_read_word(s_game + H_COUNT) <= FF7_PARTY_EXTRA_CAPACITY;
}

unsigned ff7_party_game_extra_count(void) {
    return ff7_party_game_ready() ? psx_mod_read_word(s_game + H_COUNT) : 0;
}

uint8_t ff7_party_game_extra_guest(unsigned i) {
    return i < ff7_party_game_extra_count() ? (uint8_t)(FF7_PARTY_GUEST_FIRST+i) : 255;
}

int ff7_party_game_is_extra(uint8_t guest) {
    return guest >= FF7_PARTY_GUEST_FIRST &&
        (unsigned)(guest-FF7_PARTY_GUEST_FIRST) < ff7_party_game_extra_count();
}

uint16_t ff7_party_game_identity(uint8_t guest) {
    if (guest <= 8) return guest;
    if (guest == 9) return 6; /* Original Young Cloud / Cait Sith alias. */
    if (guest == 10) return 7; /* Original story Sephiroth / Vincent alias. */
    if (!ff7_party_game_is_extra(guest)) return FF7_PARTY_EMPTY;
    return psx_mod_read_half(s_game + H_IDS + 2u*(guest-FF7_PARTY_GUEST_FIRST));
}

uint8_t ff7_party_game_guest_id(uint16_t identity) {
    if (identity < FF7_PARTY_NATIVE_COUNT) return (uint8_t)identity;
    for (unsigned i = 0; i < ff7_party_game_extra_count(); ++i)
        if (psx_mod_read_half(s_game+H_IDS+2*i) == identity)
            return (uint8_t)(FF7_PARTY_GUEST_FIRST+i);
    return FF7_PARTY_GUEST_EMPTY;
}

const FF7PartyProfile* ff7_party_game_profile(uint8_t guest) {
    return ff7_party_game_is_extra(guest) ? find_profile(ff7_party_game_identity(guest)) : NULL;
}

uint8_t ff7_party_game_presentation(uint8_t guest) {
    const FF7PartyProfile* p = ff7_party_game_profile(guest);
    return p ? p->presentation : guest;
}

uint32_t ff7_party_game_record(uint8_t guest) {
    if (guest <= 10)
        return NATIVE_RECORDS + (uint32_t)ff7_party_game_identity(guest)*FF7_PARTY_RECORD_SIZE;
    return ff7_party_game_is_extra(guest) ? s_game + H_RECORDS +
        (uint32_t)(guest-FF7_PARTY_GUEST_FIRST)*FF7_PARTY_RECORD_SIZE : 0;
}

uint32_t ff7_party_game_record_offset(uint8_t guest) {
    uint32_t record = ff7_party_game_record(guest);
    return record ? record - NATIVE_RECORDS : 0;
}

uint32_t ff7_party_game_alias_word(uint8_t guest) {
    return ff7_party_game_is_extra(guest) ? s_game+H_ALIASES+
        4u*(guest-FF7_PARTY_GUEST_FIRST) : 0;
}

static int valid_fallback(const uint8_t* party) {
    if (!party) return 0;
    for (unsigned i = 0; i < 3; ++i) {
        if (party[i] == 255) continue;
        if (party[i] > 10) return 0;
        for (unsigned j = 0; j < i; ++j)
            if (party[j]!=255 && ff7_party_game_identity(party[i])==ff7_party_game_identity(party[j]))return 0;
    }
    return 1;
}

int ff7_party_game_fallback(uint8_t out[3]) {
    uint8_t next[3];
    if (!out || !ff7_party_game_ready()) return 0;
    for (unsigned i = 0; i < 3; ++i) next[i] = psx_mod_read_byte(s_game+H_FALLBACK+i);
    if (!valid_fallback(next)) return 0;
    memcpy(out, next, 3);
    return 1;
}

FF7PartyResult ff7_party_game_snapshot(FF7PartyState* out) {
    static const uint8_t digest[32] = {0};
    FF7PartyState next;
    size_t needed;
    if (!out || !ff7_party_game_ready()) return FF7_PARTY_INVALID;
    ff7_party_init(&next);
    next.count = (uint16_t)ff7_party_game_extra_count();
    for (unsigned i = 0; i < next.count; ++i) {
        FF7PartyMember* member = &next.extra[i];
        member->id = psx_mod_read_half(s_game+H_IDS+2*i);
        member->presentation_id = psx_mod_read_byte(s_game+H_PRESENTATIONS+i);
        member->flags = psx_mod_read_byte(s_game+H_FLAGS+i);
        const FF7PartyProfile* p = find_profile(member->id);
        if (!p || p->presentation != member->presentation_id) return FF7_PARTY_INVALID;
        uint32_t record = s_game+H_RECORDS+i*FF7_PARTY_RECORD_SIZE;
        for (unsigned j = 0; j < FF7_PARTY_RECORD_SIZE; ++j)
            member->record[j] = psx_mod_read_byte(record+j);
        if (member->record[0] != FF7_PARTY_GUEST_FIRST+i) return FF7_PARTY_INVALID;
    }
    for (unsigned i = 0; i < 3; ++i) {
        uint8_t guest = psx_mod_read_byte(ACTIVE+i);
        next.active[i] = ff7_party_game_identity(guest);
        if (guest != 255 && next.active[i] == FF7_PARTY_EMPTY) return FF7_PARTY_INVALID;
        guest=psx_mod_read_byte(0x8009d391u+i);
        next.stored[i]=ff7_party_game_identity(guest);
        if(guest!=255 && next.stored[i]==FF7_PARTY_EMPTY)return FF7_PARTY_INVALID;
        next.stored_hint[i]=(guest==9||guest==10)?guest:255;
    }
    next.stored_known=1;
    if (ff7_party_encode(&next,digest,NULL,0,&needed) != FF7_PARTY_BUFFER_SMALL)
        return FF7_PARTY_INVALID;
    *out = next;
    return FF7_PARTY_OK;
}

FF7PartyResult ff7_party_game_load(const FF7PartyState* state, const uint8_t fallback[3]) {
    static const uint8_t digest[32] = {0};
    size_t needed;
    uint8_t active[3],stored[3];
    if (!s_game || !valid_fallback(fallback) ||
        ff7_party_encode(state,digest,NULL,0,&needed) != FF7_PARTY_BUFFER_SMALL)
        return FF7_PARTY_INVALID;
    for (unsigned i = 0; i < state->count; ++i) {
        const FF7PartyProfile* p = find_profile(state->extra[i].id);
        if (!p || p->presentation != state->extra[i].presentation_id) return FF7_PARTY_INVALID;
    }
    for (unsigned i = 0; i < 3; ++i) {
        uint16_t id = state->active[i];
        active[i] = id < 9 ? (uint8_t)id : 255;
        if (id == FF7_PARTY_EMPTY || id < 9) continue;
        for (unsigned j = 0; j < state->count; ++j)
            if (state->extra[j].id == id) active[i] = (uint8_t)(FF7_PARTY_GUEST_FIRST+j);
        if (active[i] == 255) return FF7_PARTY_INVALID;
    }
    for(unsigned i=0;i<3;++i){
        if(state->active[i]<9 && (fallback[i]==9||fallback[i]==10) &&
           ff7_party_game_identity(fallback[i])==state->active[i])active[i]=fallback[i];
        stored[i]=psx_mod_read_byte(0x8009d391u+i);
        if(!state->stored_known)continue;
        uint16_t id=state->stored[i];stored[i]=id<9?(uint8_t)id:255;
        if(id>=FF7_PARTY_EXTRA_FIRST && id!=FF7_PARTY_EMPTY)
            for(unsigned j=0;j<state->count;++j)
                if(state->extra[j].id==id)stored[i]=(uint8_t)(FF7_PARTY_GUEST_FIRST+j);
        if(id!=FF7_PARTY_EMPTY && stored[i]==255)return FF7_PARTY_INVALID;
        if(state->stored_hint[i]!=255)stored[i]=state->stored_hint[i];
    }
    /* No guest execution/snapshot can interleave this emulation-thread commit. */
    psx_mod_write_word(s_game+H_READY,0);
    for (unsigned i = H_FALLBACK; i < GAME_BYTES; ++i) psx_mod_write_byte(s_game+i,0);
    for (unsigned i = 0; i < 3; ++i) {
        psx_mod_write_byte(s_game+H_FALLBACK+i,fallback[i]);
        psx_mod_write_half(s_game+H_ACTIVE+2*i,state->active[i]);
    }
    for (unsigned i = 0; i < state->count; ++i) {
        const FF7PartyMember* m = &state->extra[i];
        uint32_t record = s_game+H_RECORDS+i*FF7_PARTY_RECORD_SIZE;
        psx_mod_write_half(s_game+H_IDS+2*i,m->id);
        psx_mod_write_byte(s_game+H_PRESENTATIONS+i,m->presentation_id);
        psx_mod_write_byte(s_game+H_FLAGS+i,m->flags);
        psx_mod_write_word(s_game+H_ALIASES+4*i,FF7_PARTY_GUEST_FIRST+i);
        for (unsigned j = 0; j < FF7_PARTY_RECORD_SIZE; ++j)
            psx_mod_write_byte(record+j,j ? m->record[j] : (uint8_t)(FF7_PARTY_GUEST_FIRST+i));
        /* Pre-limit extended saves lack bit0; diagnostic prototype saves may
         * hold techniques or a level this policy does not define. */
        const FF7PartyProfile* profile = find_profile(m->id);
        uint8_t limits[FF7_PARTY_RECORD_SIZE];
        memcpy(limits,m->record,sizeof limits);
        if (ff7_party_limit_canonical(profile->limit_policy,limits)) {
            psx_mod_write_byte(record+14,limits[14]);
            psx_mod_write_byte(record+15,limits[15]);
            psx_mod_write_half(record+0x22,(uint16_t)(limits[0x22]|limits[0x23]<<8));
            psx_mod_counter_add("ff7.party_game.limit_canonical",1);
        }
    }
    psx_mod_write_word(s_game+H_MAGIC,GAME_MAGIC);
    psx_mod_write_word(s_game+H_VERSION,GAME_VERSION);
    psx_mod_write_word(s_game+H_COUNT,state->count);
    psx_mod_write_word(s_game+H_READY,1);
    for (unsigned i = 0; i < 3; ++i){
        psx_mod_write_byte(ACTIVE+i,active[i]);
        if(state->stored_known)psx_mod_write_byte(0x8009d391u+i,stored[i]);
    }
    psx_mod_counter_add("ff7.party_game.load",1);
    return FF7_PARTY_OK;
}

void ff7_party_game_observe_active(void) {
    FF7PartyState state;
    uint8_t native[3];
    if (ff7_party_game_snapshot(&state) != FF7_PARTY_OK) {
        psx_mod_counter_add("ff7.party_game.active_reject",1);
        return;
    }
    for (unsigned i = 0; i < 3; ++i) {
        psx_mod_write_half(s_game+H_ACTIVE+2*i,state.active[i]);
        native[i] = psx_mod_read_byte(ACTIVE+i);
    }
    if (valid_fallback(native))
        for (unsigned i = 0; i < 3; ++i) psx_mod_write_byte(s_game+H_FALLBACK+i,native[i]);
}

int ff7_party_game_select(unsigned slot, uint16_t id) {
    FF7PartyState next;
    if (slot >= 3 || ff7_party_game_snapshot(&next) != FF7_PARTY_OK) return 0;
    uint16_t selected[3];
    memcpy(selected,next.active,sizeof(selected));
    selected[slot] = id;
    if (ff7_party_select(&next,selected) != FF7_PARTY_OK) return 0;
    psx_mod_write_byte(ACTIVE+slot,ff7_party_game_guest_id(id));
    ff7_party_game_observe_active();
    return 1;
}

static int kernel_templates(uint8_t** result, uint8_t** growth_result,
                            uint32_t* growth_size) {
    uint32_t size = 0, position = 0;
    uint8_t* raw = NULL;
    uint8_t* decoded = NULL;
    uint8_t* growth = NULL;
    int ok = 0;
    if (!psx_mod_read_disc_file("INIT/KERNEL.BIN",NULL,0,&size) || size < 24 || size > 1024*1024)
        return 0;
    raw = (uint8_t*)malloc(size);
    if (!raw || !psx_mod_read_disc_file("INIT/KERNEL.BIN",raw,size,&size)) goto done;
    for (unsigned i = 0; i <= 3; ++i) {
        if (position > size || size-position < 6) goto done;
        uint32_t compressed = raw[position] | (uint32_t)raw[position+1]<<8;
        uint32_t bytes = raw[position+2] | (uint32_t)raw[position+3]<<8;
        position += 6;
        if (compressed > size-position) goto done;
        if (i == 2 || i == 3) {
            z_stream stream;
            if (bytes < (i == 2 ? 0x61cu : 9*FF7_PARTY_RECORD_SIZE)) goto done;
            decoded = (uint8_t*)malloc(bytes);
            if (!decoded) goto done;
            memset(&stream,0,sizeof(stream));
            stream.next_in = raw+position; stream.avail_in = compressed;
            stream.next_out = decoded; stream.avail_out = bytes;
            if (inflateInit2(&stream,31) != Z_OK) goto done;
            int status = inflate(&stream,Z_FINISH);
            int valid = status == Z_STREAM_END && stream.total_out == bytes &&
                        stream.total_in == compressed;
            inflateEnd(&stream);
            if (!valid) goto done;
            if (i == 2) {
                growth = decoded; *growth_size = bytes; decoded = NULL;
            } else {
                static const uint8_t ids[9] = {0,1,2,3,4,5,9,10,8};
                for (unsigned row = 0; row < 9; ++row)
                    if (decoded[row*FF7_PARTY_RECORD_SIZE] != ids[row]) { ok=0; goto done; }
                *result = decoded; *growth_result = growth;
                decoded = NULL; growth = NULL;
                ok = 1;
            }
        }
        position += compressed;
    }
done:
    free(decoded); free(growth); free(raw); return ok;
}

static unsigned record_half(const uint8_t* r, unsigned offset) {
    return r[offset] | (unsigned)r[offset+1]<<8;
}
static void record_half_set(uint8_t* r, unsigned offset, unsigned value) {
    r[offset]=(uint8_t)value; r[offset+1]=(uint8_t)(value>>8);
}
static void record_word_set(uint8_t* r, unsigned offset, uint32_t value) {
    record_half_set(r,offset,value); record_half_set(r,offset+2,value>>16);
}
static unsigned growth_group(unsigned level) {
    return !level ? 0 : level < 61 ? (level-1)/10 : (level-61)/20+6;
}
static unsigned growth_index(int difference) {
    /* Middle of retail's 1..8 roll, without consuming the guest RNG. */
    int index=difference+4;
    return index < 0 ? 0 : index > 11 ? 11 : (unsigned)index;
}
static int growth_baseline(const uint8_t* pair, unsigned stat, unsigned level) {
    int base=(int8_t)pair[1];
    if (stat == 6) return base*40+(int)(pair[0]*level);
    if (stat == 7) return base*2+(int)(pair[0]*level/10);
    return base+(int)(pair[0]*level/100);
}
static uint32_t growth_exp(const uint8_t* growth, unsigned curve, unsigned level) {
    /* Retail 801b1320 selects one coefficient for the requested level, then
     * sums floor(coefficient*i*i/10), i=1..level. */
    unsigned coefficient=growth[0x21c+16*curve+2*growth_group(level)];
    uint32_t total=0;
    for (unsigned i=1; i<=level; ++i) total+=coefficient*i*i/10;
    return total;
}
static int seed_relative_level(uint8_t* r, const FF7PartyProfile* p,
                               const uint8_t* growth, uint32_t bytes) {
    if (!p->initial_level_offset) return 1;
    uint32_t source=NATIVE_RECORDS+p->growth_identity*FF7_PARTY_RECORD_SIZE;
    unsigned level=psx_mod_read_byte(source+1);
    unsigned hp=psx_mod_read_half(source+0x2e), mp=psx_mod_read_half(source+0x32);
    const uint8_t* row=growth+11*p->growth_identity;
    if (!level || level > 99 || !hp || !mp) return 0;
    for (unsigned s=0; s<9; ++s)
        if (0x21cu+16u*row[s]+16u > bytes) return 0;
    unsigned target=level+p->initial_level_offset;
    if (target > 99) target=99;
    for (unsigned s=0; s<6; ++s) r[2+s]=psx_mod_read_byte(source+2+s);
    memset(r+8,0,6); /* Fresh member does not inherit equipment stat bonuses. */
    for (; level<target; ++level) {
        unsigned group=growth_group(level);
        for (unsigned s=0; s<8; ++s) {
            const uint8_t* pair=growth+0x21c+16*row[s]+2*group;
            int next=growth_baseline(pair,s,level+1);
            if (s<6) {
                unsigned value=r[2+s]+growth[0x1f8+growth_index(next-r[2+s])];
                r[2+s]=(uint8_t)(value > 100 ? 100 : value);
            } else {
                unsigned* value=s==6 ? &hp : &mp;
                int before=growth_baseline(pair,s,level);
                unsigned index=growth_index(100*before/(int)*value-100);
                int updated=(int)*value+(next-before)*growth[0x204+12*(s-6)+index]/100;
                unsigned cap=s==6 ? 9999 : 999;
                *value=updated<1 ? 1 : (unsigned)updated>cap ? cap : (unsigned)updated;
            }
        }
    }
    r[1]=(uint8_t)target; r[0x21]=target==99 ? 61 : 0;
    record_half_set(r,0x2c,hp); record_half_set(r,0x2e,hp); record_half_set(r,0x38,hp);
    record_half_set(r,0x30,mp); record_half_set(r,0x32,mp); record_half_set(r,0x3a,mp);
    uint32_t exp=growth_exp(growth,row[8],target-1);
    record_word_set(r,0x3c,exp);
    record_word_set(r,0x80,target==99 ? 0 : growth_exp(growth,row[8],target)-exp);
    return 1;
}

/* Kernel templates for scene-only rows (flashback Sephiroth) hold 0xFFFF
 * max HP/MP placeholders until the native recalculation runs on party entry.
 * A fresh added member can sit in the PHS reserve first, where the stock
 * screens draw those maxima (garbage readout and an overlong gauge). Settle
 * placeholders to the base stats; party entry recalculates as usual. */
static void settle_template(uint8_t* r) {
    for (unsigned o = 0x2c; o <= 0x30; o += 4) {
        unsigned cur = r[o] | r[o+1]<<8, base = r[o+2] | r[o+3]<<8;
        unsigned max_at = o == 0x2c ? 0x38 : 0x3a, max = r[max_at] | r[max_at+1]<<8;
        if (max != 0xffffu) continue;
        max = base;
        r[max_at] = (uint8_t)max; r[max_at+1] = (uint8_t)(max>>8);
        if (cur > max) { r[o] = (uint8_t)max; r[o+1] = (uint8_t)(max>>8); }
    }
}

int ff7_party_game_initialize(const uint8_t fallback[3]) {
    if(!s_game || !valid_fallback(fallback))return 0;
    uint8_t* templates = NULL;
    uint8_t* growth = NULL;
    uint32_t growth_size = 0;
    if (!kernel_templates(&templates,&growth,&growth_size)) {
        psx_mod_counter_add("ff7.party_game.template_reject",1); return 0;
    }
    FF7PartyState state;
    ff7_party_init(&state);
    for (unsigned i = 0; i < 3; ++i) state.active[i] = ff7_party_game_identity(fallback[i]);
    for (unsigned i = 0; i < s_profile_count; ++i) {
        const FF7PartyProfile* p = s_profiles[i];
        uint8_t record[FF7_PARTY_RECORD_SIZE];
        memcpy(record,templates+p->kernel_template_row*FF7_PARTY_RECORD_SIZE,sizeof record);
        if (!seed_relative_level(record,p,growth,growth_size)) {
            free(templates); free(growth);
            psx_mod_counter_add("ff7.party_game.template_reject",1); return 0;
        }
        settle_template(record);
        ff7_party_limit_initial(p->limit_policy,record);
        if (ff7_party_add(&state,p->identity,p->presentation,
                record) != FF7_PARTY_OK) {
            free(templates); free(growth); return 0;
        }
    }
    free(templates); free(growth);
    if (ff7_party_game_load(&state,fallback) != FF7_PARTY_OK)return 0;
    psx_mod_counter_add("ff7.party_game.initialize",1);return 1;
}

static void party_game_vblank(void) {
    if (!s_game || !psx_mod_game_started()) return;
    if (ff7_party_game_ready()) {
        uint8_t native[3];
        for (unsigned i = 0; i < 3; ++i) native[i] = psx_mod_read_byte(ACTIVE+i);
        /* New-game and story scripts replace the vanilla party independently
         * of PHS. Keep the last wholly native fallback current, without copying
         * character records or exposing an intermediate two-store PHS swap. */
        if (valid_fallback(native))
            for (unsigned i = 0; i < 3; ++i)
                psx_mod_write_byte(s_game+H_FALLBACK+i,native[i]);
        return;
    }
    if (psx_mod_read_word(s_game+H_MAGIC) != GAME_MAGIC ||
        psx_mod_read_word(s_game+H_VERSION) != GAME_VERSION ||
        psx_mod_read_word(s_game+H_READY) != 0) return;
    /* Wait for retail new-game/load initialization. No field/party is selected
     * or unlocked in the vanilla roster. PHS itself retains story lock checks. */
    uint8_t fallback[3];
    for (unsigned i = 0; i < 3; ++i) fallback[i] = psx_mod_read_byte(ACTIVE+i);
    uint8_t level = psx_mod_read_byte(NATIVE_RECORDS+1);
    if (!valid_fallback(fallback) || psx_mod_read_byte(NATIVE_RECORDS) != 0 ||
        !level || level > 99 || !psx_mod_read_half(NATIVE_RECORDS+0x2e)) return;
    (void)ff7_party_game_initialize(fallback);
}

static void party_game_activate(void) {
    s_activated = 1;
    s_game = psx_mod_alloc_guest_memory(GAME_BYTES,4);
    if (!s_game || !ff7_party_menu_init() || !ff7_party_battle_init() ||
        !ff7_party_game_save_init() || !ff7_party_field_init() ||
        !ff7_party_field_loader_init() || !ff7_party_limit_init()) {
        s_game = 0;
        psx_mod_counter_add("ff7.party_game.alloc_reject",1);
        return;
    }
    psx_mod_write_word(s_game+H_MAGIC,GAME_MAGIC);
    psx_mod_write_word(s_game+H_VERSION,GAME_VERSION);
    psx_mod_write_word(s_game+H_READY,0);
    psx_mod_counter_add(s_game ? "ff7.party_game.activate" : "ff7.party_game.alloc_reject",1);
}

static void party_game_restore(void) {
    /* Guest bytes and ownership rewind as one unit; never republish host-era
     * records here. A corrupt header remains unavailable to every adapter. */
    ff7_party_menu_restore();
    ff7_party_battle_restore();
    ff7_party_limit_restore();
    ff7_party_game_save_restore();
    ff7_party_field_restore();
    ff7_party_field_loader_restore();
    psx_mod_counter_add(ff7_party_game_ready() ? "ff7.party_game.restore" :
                        "ff7.party_game.restore_empty",1);
}

PSX_MOD_CONSTRUCTOR(register_ff7_party_game) {
    (void)ff7_party_game_register_profile(&s_sephiroth);
    (void)psx_mod_register_activation_plugin("ff7.party-gameplay",party_game_activate);
    (void)psx_mod_register_vblank_plugin("ff7.party-gameplay",party_game_vblank);
    (void)psx_mod_register_savestate_plugin("ff7.party-gameplay",party_game_restore);
}
