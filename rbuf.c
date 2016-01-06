/* REFERENCE BUFFER */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "rbuf.h"

/* Unlike cbufs, there isn't any ambiguity whether the buffer is full or empty.
   Even if r_index == w_index, we can check if the entry at r_index has any data
   in it.
   
   INVARIANT: The entry at r_index always contains data, unless the buffer is
   completely empty.

   INVARIANT: The entry at w_index always has free space, unless the buffer is
   completely full.

   INVARIANT: If there is a free entry in the buffer, then either the entry at
   w_index is completely empty, or the entry after that is completely empty.
*/

int rbuf_init(uint8_t* buf, size_t length) {
    if (length < sizeof(struct refbuf_header)) {
        return -1;
    }
    memset(buf, 0x00, length);
    return 0;
}

/* Returns whether or not ENTRY contains directly buffered data, or a pointer to   data. */
int _is_direct_buffer(union refbuf_entry* entry) {
    return (entry->datent.length & DATA_ID_MASK) != 0;
}

uint32_t _rbufent_used_space(union refbuf_entry* entry) {
    if (_is_direct_buffer(entry)) {
        return (uint32_t) (entry->datent.length & DATA_LEN_MASK);
    } else {
        int32_t rawlen = entry->ptrent.buflen;
        return (uint32_t) rawlen;
    }
}

int _rbuf_isfull(struct refbuf_header* rhdr, union refbuf_entry* entries) {
    uint32_t used_space = _rbufent_used_space(&entries[rhdr->r_index]);
    return (rhdr->r_index == rhdr->w_index) && used_space == rhdr->readpos
        && used_space > 0;
}

int _rbuf_isempty(struct refbuf_header* rhdr, union refbuf_entry* entries) {
    return (rhdr->r_index == rhdr->w_index) &&
        _rbufent_used_space(&entries[rhdr->r_index]) == 0;
}

/* Appends BUF to RBUF. RBUF_NUMENTRIES is the number of entries in RBUF, and
   can be found by applying the RBUF_NUMENTRIES macro to the length of the
   buffer used to initialize RBUF. */
int rbuf_append(uint8_t* rbuf, uint16_t rbuf_numentries,
                uint8_t* buf, int32_t buflen) {
    struct refbuf_header* rhdr = (struct refbuf_header*) rbuf;
    union refbuf_entry* entries = (union refbuf_entry*) (rhdr + 1);
    union refbuf_entry* current;
    uint16_t nextindex; // Index of the next entry after rhdr->w_index
    int hasnextent; // TRUE if there's a next entry to move to, else FALSE
    uint32_t usedcurr; // Bytes of used space in the current entry
    uint32_t maxdirect; // Max. number of bytes it's better to buffer directly
    if (_rbuf_isfull(rhdr, entries)) {
        goto outofspace;
    }
    nextindex = (rhdr->w_index + 1) % rbuf_numentries;
    hasnextent = (nextindex != rhdr->r_index);
    current = &entries[rhdr->w_index];
    usedcurr = _rbufent_used_space(current);
    if (usedcurr == 0) { // Store the data in the current entry
        if (buflen > DATA_ENTRY_CAP) {
            goto storeptr;
        } else {
            goto bufferinemptycurrent;
        }
    } else { // There's directly buffered data in this entry.
        maxdirect = DATA_ENTRY_CAP - usedcurr;
        if (hasnextent) {
            maxdirect += DATA_ENTRY_CAP;
        }
        if (buflen > maxdirect) {
            if (hasnextent) {
                current = &entries[nextindex];
                nextindex = (nextindex + 1) % rbuf_numentries;
                goto storeptr;
            } else {
                goto outofspace;
            }
        } else {
            /* This is the only nontrivial case. We need to first buffer into
               the current entry, and then continue into the next. */
            uint32_t freecurr = DATA_ENTRY_CAP - usedcurr;
            uint32_t tobuffer = freecurr < buflen ? freecurr : buflen;
            memcpy(&current->datent.data[usedcurr], buf, tobuffer);
            current->datent.length = DATA_ID_MASK | (usedcurr + tobuffer);
            if (buflen <= freecurr) {
                goto returnbuffered;
            }
            current = &entries[nextindex];
            rhdr->w_index = nextindex;
            buf += tobuffer;
            buflen -= tobuffer;
            goto bufferinemptycurrent;
        }
    }

    /* This label stores a pointer to buf into the current entry. */
    storeptr:
    current->ptrent.bufptr = buf;
    current->ptrent.buflen = buflen;
    rhdr->w_index = nextindex;
    return 1;

    /* This label copies the contents of buf into the current entry, assuming
       that the current entry is empty and has enough space. */
    bufferinemptycurrent:
    memcpy(&current->datent.data, buf, buflen);
    current->datent.length = DATA_ID_MASK | (uint8_t) buflen;
    if (buflen == DATA_ENTRY_CAP) {
        rhdr->w_index = nextindex;
    }
    returnbuffered:
    return 2;
    
    outofspace:
    return 0;
}

uint32_t rbuf_read(uint8_t* rbuf, uint16_t rbuf_numentries, uint8_t* data,
                   uint32_t numbytes, uint32_t offset, int pop, int* nfreed,
                   int* traversedentry) {
    struct refbuf_header* rhdr = (struct refbuf_header*) rbuf;
    union refbuf_entry* entries = (union refbuf_entry*) (rhdr + 1);
    uint32_t bytesleft = numbytes;
    uint32_t offsetleft = offset;
    uint16_t currindex;
    uint16_t stopindex;
    union refbuf_entry* current;
    uint8_t* currdata;
    uint32_t currdatalen;
    uint32_t readpos;
    if (_rbuf_isempty(rhdr, entries)) {
        return 0;
    }
    currindex = rhdr->r_index;
    readpos = rhdr->readpos;
    stopindex = rhdr->r_index == rhdr->w_index ? rhdr->r_index :
        (rhdr->w_index + 1) % rbuf_numentries;
    do {
        current = &entries[currindex];
        if (_is_direct_buffer(current)) {
            currdata = current->datent.data;
        } else {
            currdata = current->ptrent.bufptr;
        }
        currdatalen = _rbufent_used_space(current);
        currdata += readpos;
        currdatalen -= readpos;
        if (currdatalen < offsetleft) {
            /* Skip this whole entry. */
            offsetleft -= currdatalen;
            currdatalen = 0;
        } else {
            /* Skip the first offsetleft bytes of this entry. */
            currdata += offsetleft;
            currdatalen -= offsetleft;
            offsetleft = 0;
        }
        if (currdatalen > bytesleft) {
            memcpy(data, currdata, bytesleft);
            readpos += bytesleft;
            bytesleft = 0;
            break;
        } else {
            memcpy(data, currdata, currdatalen);
            data += currdatalen;
            bytesleft -= currdatalen;
            if (pop) {
                if (nfreed && !_is_direct_buffer(current)) {
                    *nfreed += 1;
                }
                if (traversedentry) {
                    *traversedentry = 1;
                }
                /* Zero entries as we pop them. */
                memset(current, 0x00, sizeof(union refbuf_entry));
            }
            readpos = 0;
        }
        currindex = (currindex + 1) % rbuf_numentries;
    } while (currindex != stopindex);

    if (pop) {
        rhdr->r_index = currindex;
        rhdr->readpos = readpos;
    }

    return numbytes - bytesleft;
}

uint32_t rbuf_used_space(uint8_t* rbuf, uint16_t rbuf_numentries) {
    struct refbuf_header* rhdr = (struct refbuf_header*) rbuf;
    union refbuf_entry* entries = (union refbuf_entry*) (rhdr + 1);
    uint16_t i = rhdr->r_index;
    uint32_t total = 0;
    do {
        total += _rbufent_used_space(&entries[i]);
        if (i == rhdr->w_index) {
            break;
        }
        i = (i + 1) % rbuf_numentries;
    } while (i != rhdr->r_index);
    total -= rhdr->readpos;
    return total;
}

int rbuf_has_free_entry(uint8_t* rbuf, uint16_t rbuf_numentries) {
    struct refbuf_header* rhdr = (struct refbuf_header*) rbuf;
    union refbuf_entry* entries = (union refbuf_entry*) (rhdr + 1);
    uint16_t after_w_index = (rhdr->w_index + 1) % rbuf_numentries;
    return _rbufent_used_space(&entries[rhdr->w_index]) == 0
        || _rbufent_used_space(&entries[after_w_index]) == 0;
}

void rbuf_print(uint8_t* rbuf, uint16_t rbuf_numentries) {
    struct refbuf_header* rhdr = (struct refbuf_header*) rbuf;
    union refbuf_entry* entries = (union refbuf_entry*) (rhdr + 1);
    uint8_t* byteptr;
    uint16_t i, j;
    printf("Header: %s = %d, %s = %d, %s = %d\n", "readpos", rhdr->readpos,
           "r_index", rhdr->r_index, "w_index", rhdr->w_index);

    for (i = 0; i < rbuf_numentries; i++) {
        byteptr = (uint8_t*) &entries[i];
        for (j = 0; j < sizeof(union refbuf_entry); j++) {
            printf("%02x ", *byteptr);
            byteptr++;
        }
        printf("   ");
    }
    printf("\n");
}
