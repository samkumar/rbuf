#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "rbuf.h"

#define BUFLEN 50
#define NRBUFENT RBUF_NUMENTRIES(BUFLEN)

uint8_t largebuf[1000];

#define LONGSTR "As more and more physical information becomes available, a critical problem is enabling the simple and efficient exchange of this data. We present our design for a simple RESTful web service called the Simple Measuring and Actuation Profile (sMAP) which allows instruments and other producers of physical information to directly publish their data. In our design study, we consider what information should be represented, and how it fits into the RESTful paradigm. To evaluate sMAP, we implement a large number of data sources using this profile, and consider how easy it is to use to build new applications. We also design and evaluate a set of adaptations made at each layer of the protocol stack which allow sMAP to run on constrained devices."

int main(int argc, char** argv) {
    uint8_t* buffer = malloc(BUFLEN);
    uint8_t* onechar = (uint8_t*) "a";
    uint8_t* twochars = (uint8_t*) "bc";
    uint8_t* threechars = (uint8_t*) "def";
    uint8_t* fourchars = (uint8_t*) "ghij";
    uint8_t* fivechars = (uint8_t*) "klmno";
    uint8_t* sixchars = (uint8_t*) "pqrstu";
    uint8_t* sevenchars = (uint8_t*) "vwxyzAB";
    uint8_t* eightchars = (uint8_t*) "CDEFGHIJ";
    uint8_t* ninechars = (uint8_t*) "KLMNOPQRS";
    uint8_t* tenchars = (uint8_t*) "TUVWXYZ123";
    uint8_t* fifteenchars = (uint8_t*) "4567890~`:;<>,?";
    uint8_t* longstring = (uint8_t*) LONGSTR;
    uint8_t buf[201];
    int poppedbuffers;
    int traversedentry;
    uint32_t len;
    int rv;
    
    rbuf_init(buffer, BUFLEN);

    printf("Initially\n");
    rbuf_print(buffer, NRBUFENT);
    
    rv = rbuf_append(buffer, NRBUFENT, onechar, 1);
    printf("rv = %d\n", rv);

    printf("After writing one character\n");
    rbuf_print(buffer, NRBUFENT);
    
    rv = rbuf_append(buffer, NRBUFENT, threechars, 3);
    printf("rv = %d\n", rv);

    printf("After writing three characters\n");
    rbuf_print(buffer, NRBUFENT);

    rv = rbuf_append(buffer, NRBUFENT, longstring, strlen(LONGSTR));
    printf("rv = %d\n", rv);

    printf("After writing a long string\n");
    rbuf_print(buffer, NRBUFENT);
    
    rv = rbuf_append(buffer, NRBUFENT, tenchars, 10);
    printf("rv = %d\n", rv);

    printf("After writing ten characters\n");
    rbuf_print(buffer, NRBUFENT);

    printf("Trying to add two characters\n");
    rv = rbuf_append(buffer, NRBUFENT, twochars, 2);
    printf("rv = %d\n", rv);

    printf("After trying\n");
    rbuf_print(buffer, NRBUFENT);

    printf("Reading the first 200 characters\n");
    len = rbuf_read(buffer, NRBUFENT, buf, 200, 0, 0, NULL, NULL);
    buf[len] = '\0';
    printf("%s\n", (char*) buf);

    printf("Rereading with an offset of 2\n");
    len =rbuf_read(buffer, NRBUFENT, buf, 200, 2, 0, NULL, NULL);
    buf[len] = '\0';
    printf("%s\n", (char*) buf);

    printf("Rereading with an offset of 555\n");
    len = rbuf_read(buffer, NRBUFENT, buf, 200, 555, 0, NULL, NULL);
    buf[len] = '\0';
    printf("%s\n", (char*) buf);

    printf("Rereading with an offset of 600\n");
    memset(buf, 0xFE, 200);
    len = rbuf_read(buffer, NRBUFENT, buf, 200, 600, 0, NULL, NULL);
    buf[len] = '\0';
    printf("%s\n", (char*) buf);

    printf("Reading and removing the first 3 characters\n");
    poppedbuffers = 0;
    traversedentry = 0;
    len = rbuf_read(buffer, NRBUFENT, buf, 3, 0, 1, &poppedbuffers,
                    &traversedentry);
    buf[len] = '\0';
    printf("%d %d %s\n", poppedbuffers, traversedentry, (char*) buf);

    printf("Reading the first 200 characters\n");
    len = rbuf_read(buffer, NRBUFENT, buf, 200, 0, 0, NULL, NULL);
    buf[len] = '\0';
    printf("%s\n", (char*) buf);

    printf("Rereading with an offset of 2\n");
    len =rbuf_read(buffer, NRBUFENT, buf, 200, 2, 0, NULL, NULL);
    buf[len] = '\0';
    printf("%s\n", (char*) buf);

    printf("Reading and removing the first 3 characters\n");
    poppedbuffers = 0;
    traversedentry = 0;
    len = rbuf_read(buffer, NRBUFENT, buf, 3, 0, 1, &poppedbuffers,
                    &traversedentry);
    buf[len] = '\0';
    printf("%d %d %s\n", poppedbuffers, traversedentry, (char*) buf);

    printf("Reading the first 200 characters\n");
    len = rbuf_read(buffer, NRBUFENT, buf, 200, 0, 0, NULL, NULL);
    buf[len] = '\0';
    printf("%s\n", (char*) buf);

    printf("Rereading with an offset of 550\n");
    len = rbuf_read(buffer, NRBUFENT, buf, 200, 550, 0, NULL, NULL);
    buf[len] = '\0';
    printf("%s\n", (char*) buf);

    printf("Now, this is what it looks like\n");
    rbuf_print(buffer, NRBUFENT);

    printf("There are %d bytes in the buffer\n",
           rbuf_used_space(buffer, NRBUFENT));

    printf("Appending a fifteen character buffer\n");
    rv = rbuf_append(buffer, NRBUFENT, fifteenchars, 15);
    printf("rv = %d\n", rv);

    printf("After writing fifteen characters\n");
    rbuf_print(buffer, NRBUFENT);

    printf("Reading 15 characters at offset 754\n");
    len = rbuf_read(buffer, NRBUFENT, buf, 15, 754, 0, NULL, NULL);
    buf[len] = '\0';
    printf("%s\n", (char*) buf);

    printf("Removing 753 characters\n");
    poppedbuffers = 0;
    traversedentry = 0;
    len = rbuf_read(buffer, NRBUFENT, largebuf, 753, 0, 1, &poppedbuffers,
                    &traversedentry);
    largebuf[len] = '\0';
    printf("%d %d %s\n", poppedbuffers, traversedentry, (char*) largebuf);

    printf("After removing 753 characters\n");
    rbuf_print(buffer, NRBUFENT);

    printf("Appending a seven character buffer\n");
    rv = rbuf_append(buffer, NRBUFENT, sevenchars, 7);
    printf("rv = %d\n", rv);

    printf("After appending seven characters\n");
    rbuf_print(buffer, NRBUFENT);

    printf("Appending a fifteen character buffer\n");
    rv = rbuf_append(buffer, NRBUFENT, fifteenchars, 15);
    printf("rv = %d\n", rv);

    printf("Removing one character\n");
    poppedbuffers = 0;
    traversedentry = 0;
    len = rbuf_read(buffer, NRBUFENT, buf, 1, 0, 1, &poppedbuffers,
                    &traversedentry);
    buf[len] = '\0';
    printf("%d %d %s\n", poppedbuffers, traversedentry, (char*) buf);

    printf("After removal of the character\n");
    rbuf_print(buffer, NRBUFENT);

    printf("Appending a fifteen character buffer\n");
    rv = rbuf_append(buffer, NRBUFENT, fifteenchars, 15);
    printf("rv = %d\n", rv);

    printf("After appending fifteen characters\n");
    rbuf_print(buffer, NRBUFENT);
}
