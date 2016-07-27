This document describes files compressed by HSQ algorithm.

File format
===========
HSQ compression is based on LZ77 compression algorithm. It consist of two
parts: header and compressed data itself.

Header
------
Header is made of first 6 bytes of the file.

```
   0  1  2  3  4  5
 +--+--+--+--+--+--+
 |D0|D1|D2|C0|C1|CH|
 +--+--+--+--+--+--+
```

- bytes D0, D1 and D2 in little endian order forms size of decompressed file
- bytes C0 and C1 forms size of compressed file (size of HSQ file)
- CH is header checksum. The lowest byte of sum of the header must be 0xAB:
 (D0 + D1 + D2 + C0 + C1 + CH) & 0xFF == 0xAB

