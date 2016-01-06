#ifndef RBUF_H_
#define RBUF_H_

#include <stdint.h>

typedef int32_t nxle_int32_t;

/* REFERENCE BUFFER
   The reference buffer maintains references to data, either as pointers to
   buffers, or by copying the data and storing it locally. */

struct refbuf_header {
    uint32_t readpos; // position of the next byte to read within the entry
    uint16_t r_index; // index of the entry containing the next byte to be read
    uint16_t w_index; // index of the next entry with a free spot
} __attribute__((packed));

struct pointer_entry {
    uint8_t* bufptr;
    nxle_int32_t buflen; // MUST be little-endian
} __attribute__((packed));

#define DATA_ENTRY_CAP (sizeof(struct pointer_entry) - 1)
#define DATA_LEN_MASK ((uint8_t) 0x7F)
#define DATA_ID_MASK ((uint8_t) 0x80)

struct data_entry {
    uint8_t data[DATA_ENTRY_CAP];
    uint8_t length; // Overlaps with most significant byte of "buflen"
} __attribute__((packed));
// No harm in packing this, since it only contains uint8_ts.

/* Use external types to guarantee endianness. */
union refbuf_entry {
    struct pointer_entry ptrent;
    struct data_entry datent;
} __attribute__((packed));

#define RBUF_NUMENTRIES(buflen) ((buflen - sizeof(struct refbuf_header)) / \
                                 sizeof(union refbuf_entry))

/* Initializes a Reference Buffer. */
int rbuf_init(uint8_t* buf, size_t length);

/* Appends the contents of a buffer to the Reference Buffer. Returns 0 if the
   Reference Buffer is full and could not accept the data. Returns 1 if the
   Reference Buffer contains a reference to the data. Returns 2 if the Reference
   Buffer directly buffers the data.
   RBUF_NUMENTRIES is the number of entries in the buffer. It can be found
   via a call to the RBUF_NUMENTRIES macro, which, given the length of the
   buffer passed in to rbuf_init(), computes how many entries are in the
   Reference Buffer. */
int rbuf_append(uint8_t* rbuf, uint16_t rbuf_numentries,
                uint8_t* buf, int32_t buflen);

/* Reads NUMBYTES bytes of data into DATA from the Reference Buffer, starting
   at an offset of OFFSET bytes. If POP is set to a true value, then this also
   removes the first OFFSET + NUMBYTES bytes of data from the buffer. If
   NFREED is not NULL, it is incremented once for each referenced (pointed-to)
   buffer that is no longer needed by the Reference Buffer and can be reclaimed
   by the application. If at least one entry of the Reference Buffer is
   completely traversed during the read (either in the offset or in the copied
   bytes) and TRAVERSEDENTRY is not NULL, then *TRAVERSEDENTRY is set to 1.
   In particular, if POP is set to a true value and *TRAVERSEDENTRY is set to 1,
   then the next call to RBUF_APPEND is guaranteed to succeed.
   Returns the number of bytes read, which is smaller than NUMBYTES if there
   are fewer than OFFSET + NUMBYTES bytes in the Reference Buffer.
   RBUF_NUMENTRIES is the number of entries in the buffer. It can be found
   via a call to the RBUF_NUMENTRIES macro, which, given the length of the
   buffer passed in to rbuf_init(), computes how many entries are in the
   Reference Buffer. */
uint32_t rbuf_read(uint8_t* rbuf, uint16_t rbuf_numentries, uint8_t* data,
                   uint32_t numbytes, uint32_t offset, int pop, int* nfreed,
                   int* traversedentry);

/* Computes the total amount of data stored in the Reference Buffer. */
uint32_t rbuf_used_space(uint8_t* rbuf, uint16_t rbuf_numentries);

/* Returns a true value if a call to rbuf_append is guaranteed to succeed
   (return a 1 or a 2). Returns a false value if it is not. */
int rbuf_has_free_entry(uint8_t* rbuf, uint16_t rbuf_numentries);

/* Provides a human-readable representation of a Reference Buffer. */
void rbuf_print(uint8_t* rbuf, uint16_t rbuf_numentries);

#endif
