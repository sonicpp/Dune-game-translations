Cryo Dune translation
=====================
This file will help you to translate Cryo's game Dune into your language.

Extraction
----------
All Dune text files are compressed with HSQ algorithm. To extract these file
you have to use external HSQ util, see
[Dune Revival project](https://sourceforge.net/p/dunerevival/)
or
[FED2k discussion](https://forum.dune2k.com/topic/17217-rewriting-cryos-dune-1-it-seems-possible/?page=1).

After extraction you can also unpack these files to better editable format
by using *utils/tu* util - this util will put each phrase on single line.

Warning: if you do not use *utils/tu*, then all phrases have to be exact size
as the original file - it is because of offsets in the file.

Translation
-----------
Now, when you have files prepared, you can translate them. Choose, which files
you will be editing (see files/TEXT.md for files description):

English:
- PHRASE11.HSQ
- PHRASE12.HSQ
- COMMAND1.HSQ

France:
- PHRASE21.HSQ
- PHRASE22.HSQ
- COMMAND2.HSQ

German:
- PHRASE31.HSQ
- PHRASE32.HSQ
- COMMAND3.HSQ

Warning: beware of special bytes - these bytes has to be preserved. See
files/TEXT.md for details.

Warning: Special national characters are not supported right now, use the same
characters as editing file.

Packing
-------
When your translation is done, just pack them back into game format by
*utils/tu* (if you unpacked them before) and compress them by HSQ algorithm
(by some external util).

