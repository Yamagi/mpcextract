Monkeystone Games MPC file extractor
====================================
This small utility extracts the contents of a Monkeystone MPC file into
the current directory. MPC files are a proprietary archive file format,
found in several games made by Monkeystone. This tool was tested against
'Hyperspace Deilvery Boy' only, but I'm confident that it'll work with
other games, too.

MPC file format
---------------
The MPC file format is yet another variant of the popular PAK files,
found in several older games like Quake I and II. Every MPC file starts
 with a header:
 
    0 to 3:  char[4]   Signature, the little endian string 'MPUC'.
    4 to 7:  uint32_t  Directory offset from the end of the header.
    8 to 11: uint32_t  Unknown fild, maybe some kinf of party bit.

The directory can be located anywhere in the file, but it's a good idea
to place it at the end. So you'll need only to rewrite the directory and
not the whole MPC file if you're adding data to it. The directory starts
with a field containing the number of files in the directory. Remember
to add 12 bytes for the file header:

    start_of_file + 12 + directory_offset: uint32_t  number of files

The directory itself is just a bunch of entries, starting right after
the 'number of files' field at:

    start_of_file + 12 + directory_offset + 4:  First directory entry

Each entry consists of 5 fields with a total of 80 bytes:

    0 to 63:  char[64]  Filename.
    64 to 67: uint32_t  Fileposition from the start of the MPC file.
    68 to 71: uint32_t  Filelength in byte.
    72 to 75: uint32_t  Unknown, same as byte 68 to 71.
    76 to 79: uint32_t  Unknown, maybe some kind of parity bit.

Reading a MPC file is easy:
* Read the header and check it's signature.
* Locate the directory and read the number of files in it.
* Walk through the directory, read each entry.
* Use the entries data to extract the file.
