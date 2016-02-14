This document describes following files (extractated):
- PHRASE11.HSQ
- PHRASE12.HSQ
- PHRASE21.HSQ
- PHRASE22.HSQ
- PHRASE31.HSQ
- PHRASE32.HSQ
- COMMAND1.HSQ
- COMMAND2.HSQ
- COMMAND3.HSQ

Every single file is compressed by HSQ algorithm, which is not described in
this document. For HSQ compression see
[Dune Revival project](https://sourceforge.net/p/dunerevival/)
or
[FED2k discussion](https://forum.dune2k.com/topic/17217-rewriting-cryos-dune-1-it-seems-possible/?page=1).

File format
===========
Extracted HSQ text files are formed by two parts.

First part consists of 16-bit little endian offsets (related to beginning of
the file). These offsets define swhere is start of each phrase located. Second
part consists the phrases itself. Example: for 10th phrase read 10th 16-bit
word from file - content of this word says where 10th phrases begins. The end
of phrase is marked by 0xFF byte (see below).

Special bytes:
- 0xFF - end of phrase

National specific
=================

English
-------
Affected files:
- PHRASE11.HSQ
- PHRASE12.HSQ
- COMMAND1.HSQ

Special bytes:

France
------
Affected files:
- PHRASE21.HSQ
- PHRASE22.HSQ
- COMMAND2.HSQ

Special bytes:

German
------
Affected files:
- PHRASE31.HSQ
- PHRASE32.HSQ
- COMMAND3.HSQ

Special bytes:

File types
==========
Phrases are divided into two files - maybe because of file size limit.

Phrase 1
--------
Affected files:
- PHRASE11.HSQ
- PHRASE21.HSQ
- PHRASE31.HSQ

Important phrases:

Phrase 2
--------
Affected files:
- PHRASE12.HSQ
- PHRASE22.HSQ
- PHRASE32.HSQ

Important phrases:

Command
-------
Affected files:
- COMMAND1.HSQ
- COMMAND2.HSQ
- COMMAND3.HSQ

Important phrases:
