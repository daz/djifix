/**********
This program is free software: you can redistribute it and/or modify
it under the terms of the GNU General Public License as published by
the Free Software Foundation, either version 3 of the License, or
(at your option) any later version.

This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with this program.  If not, see <http://www.gnu.org/licenses/>.
**********/
/*
    A C program to repair corrupted video files that can sometimes be produced by
    DJI quadcopters.
    Version 2023-05-05

    Copyright (c) 2014-2023 Live Networks, Inc.  All rights reserved.

    Version history:
    - 2014-09-01: Initial version
    - 2014-09-21: When repairing 'type 2' files, we prompt the user to specify the video format
            that was used (because the SPS NAL unit - that we prepend to the repaired file -
	    differs for each video format).
    - 2014-10-04: We now automatically generate the name of the repaired file from the name of
            the original file (therefore, the user no longer specifies this on the command line).
	    Also, we can now handle certain (rare) files in which the
	    'ftyp','moov','mdat'(containing 'ftyp') sequence occurs more than once at the start.
    - 2014-10-11: When performing a 'type 2' repair, we now better handle the case where we see
            a 4-byte 'NAL size' of 0.  This anomalous situation can happen when a large chunk of
	    zero bytes appears in the file.  We now try to recover from this by scanning forward
	    until we see (what we hope to be) a 'NAL size' of 2, which would indicate
	    the resumption of sane data.
    - 2015-01-08: Handle anomalous 0xFFFFFFFF words that can appear at the start (or interior)
            of corrupted files.
    - 2015-01-24: We can now repair 'type 2' files that were recorded in 1080p/60 format.
            (The DJI Phantom 2 Vision+ doesn't yet record in this format, but some other cameras
	     can, and they can produce files that are corrupted in a similar way.)
	    We now also try to recover from encountering bad (far too large) NAL sizes when
	    repairing 'type 2' files, and unexpected garbage appearing at the beginning of files.
    - 2015-03-30: We now support two ('4k') video formats used by the Inspire 1:
            2160p/30 and 2160p/24, and updated the H.264 SPS data to work for 1080p/60 files
	    from the Inspire 1.  We also support a wider range of damaged files.
    - 2015-05-09: We now support an additional video format - 1080p/24 - used by the Inspire 1.
    - 2015-06-16: We now support an additional video format - 1080p/50 - used by the Inspire 1.
    - 2015-09-25: We now support two more video formats: 2160p/25 and 720p/25
    - 2015-11-03: We now support an additional video format - 1520p/30
    - 2015-11-27: Corrected(?) the SPS NAL unit prepended to each frame for the 2160p/25 format.

    - 2016-04-19: We now support an additional video format - 1520p/25
    - 2016-08-05: Made the checking for 0x00000002 a bit more robust: We now also check that the following bytes are sane
    - 2016-08-15: We now support an additional video format - 720p/48
    - 2016-12-08: We now support an additional video format - 1520p/24
    - 2017-02-10: We now support new-style movie files (e.g., produced by the Phantom 4 Pro)
    - 2017-02-11: We now support an additional video format - 2160(x4096)p/30 (for new-style)
    - 2017-03-13: We now use the term "4K" to refer only to 2160x4096 video.
                  We now support an additional video format - 1530p/24
    - 2017-03-20: We now support an additional video format - 1530p/30
    - 2017-04-02: We now allow 'mdat' atoms to have a (broken) atom size that's <8, because
      when we're checking for 'mdat' atoms, we don't use the atom size anyway.
    - 2017-04-08: We now support an additional video format - H.265 1080p/120
    - 2017-04-17: We now support an additional video format - H.265 1080p/25
    - 2017-04-24: We now support an additional video format - H.264 480p, from the XL FLIR camera
    - 2017-05-15: We now support an additional video format - H.264 2160(x3840)p/48
    - 2017-05-18: We now support an additional video format - H.264 2160(x4096)p/60
    - 2017-05-21: Improved the skipping over anomalous non-video data (e.g., in an Osmo file)
    - 2017-05-29: We now support an additional video format - H.264 2160(x3840)p/50
    - 2017-06-06: We now do a better job of searching for video data within a file. In particular,
                  we now detect H.264 SPS NAL units in addition to 2-byte NAL units.
    - 2017-06-14: We now support an additional video format - H.264 2160(x3840)p24 (type 2)
    - 2017-06-20: For clarity, added an output message when we start to repair 'type 4' files.
    - 2017-06-27: We now support an additional video format - H.264 1080p/120 (type 3)
    - 2017-06-28: Made the check for embedded H.264 SPS NAL units a bit more strict
    - 2017-06-29: Fixed a bug in 'type 4' file repair.
    - 2017-07-05: Loosened the check for embedded H.264 SPS NAL units to allow for Spark videos
                  (that have a shorter SPS length).
    - 2017-07-19: We now support 4 new video formats (used in the Mavic Pro):
                  1530p/25, 1080p/48, 720p/50, 720p/24
    - 2017-08-01: We now support an additional video format - H.264 1530p/30 (type 3)
    - 2017-08-25: We now support two new formats - 2160(x4096)p24 and 1080p24 (type 3)
    - 2017-08-29: The previous version's new format should have been 2160(x4096)p48, not p24
    - 2017-09-29: We now support an additional video format - H.264 2160(x3840)p60 (type 3)
    - 2017-10-10: We now support an additional video format - H.264 1530p/30 (type 3)
    - 2017-10-22: We now support an additional video format - H.264 1080p/25 (type 3)
    - 2017-11-04: We now support an additional video format - H.265 2160(x3840)p25 (type 3)
    - 2018-01-24: We now support an additional video format - H.264 2160(x4096)p25 (type 2)
    - 2018-01-27: Zenmuse apparently uses a different SPS/PPS for its (type 2) 1080p30 videos than
                  the Phantom did, so we added a new format for this.
    - 2018-02-21: We now support an additional video format - H.264 1080p/30 (type 3)
    - 2018-03-31: We can now repair more kinds of 'type 4' file (apparently from the Mavic Air)
    - 2018-04-22: We now support an additional video format - H.264 1520p/60 (type 2)
    - 2018-05-03: We now support an additional video format - H.264 2160(x4096)p25 (type 3)
    - 2018-07-11: We now support an additional video format - H.264 1080p60 (type 3)
    - 2018-08-01: If the size of the initial 'ftyp' atom is bad, just ignore it, rather than failing
    - 2018-08-17: We now support an additional video format - H.264 2160(x3840)p25 (type 3)
    - 2018-09-03: We can now repair at least some 'type 4' H.265 (HEVC) video files
    - 2019-01-14: We now support an additional video format - H.264 1530p/48 (type 3)
    - 2019-02-25: We now support an additional video format - H.264 2160(x4096)p50 (type 3)
    - 2019-06-09: The Osmo+ apparently uses a different SPS/PPS for its (type 3) 720p60 videos than
                  the Phantom did, so we added a new format for this.
    - 2019-08-20: We now support an additional video format - H.264 2160(x3840)p24 (type 3)
    - 2019-11-07: We now support an additional video format - H.264 720p (type 3)
    - 2019-11-19: The Mavic Mini changed the type 3 video format somewhat by adding a metadata track.
                  We now skip over blocks from this track (though print out the first block).
		  The H.264 1530p30 SPS and PPS values have been updated to conform to those now
		  used by the Mavic Mini.
    - 2019-12-28: Added support for the new SPS and PPS values used for 1080p30 by the Mavic Mini.
    - 2019-12-29: We now support an additional video format - H.264 1530p/25 (type 3)(Mavic Mini)
                  We also allow for variable-sized 'metadata' blocks in Mavic Mini videos.
    - 2020-01-07: We now support an additional video format - H.265 1080p60 (type 3)
    - 2020-01-08: Added support for the new SPS and PPS values used for 1080p60 by the Mavic Mini.
    - 2020-02-13: We now support an additional video format - H.264 1080p/50 (type 3)(Mavic Mini)
    - 2020-03-23: We now support an additional video format - H.264 1080p/25 (type 3)(Mavic Mini)
    - 2020-03-23a: We now support an additional video format - H.265 2160(x3840)p30 (type 3)
    - 2020-03-25: We now detect (and print the first occurrence of) an additional type of
                  metadata block.
    - 2020-04-14: The SPS for H.264 1530p/30 (type 3)(Mavic Mini) appears to have changed slightly.
    - 2020-05-05: We now support two additional video formats - H.264 1530p/24 (type 3)(Mavic Mini),
                  and H.264 1080p/24  (type 3)(Mavic Mini)
    - 2020-07-12: We now support an additional video format - H.265 1530p50 (type 3)
    - 2021-01-01: We now support an additional video format - H.265 2160(x3840)p30 (type 3)(DJI Mini 2)
    - 2021-01-24: We now support an additional video format - H.264 1520p60 (type 3)(DJI Mini 2)
    - 2021-06-21: Improved the checking for video in 'type 4' repairs, to (at least partially)
                  repair some Mavic FPV videos.
    - 2021-07-02: We now support videos from "DJI Mini 2" drones.  These are similar to
                  'type 3' videos, but lack a JPEG prefix.  We designate them as 'type 5'
		  videos, and allow the user to select only known "DJI Mini 2" formats.
                  We can now handle extended atom sizes for 'mdat' atoms (because we don't
		  use the size of 'mdat' atoms)
    - 2021-07-04: We support two more video formats - 3840x2160p24 and 1920x1080p48 -
                  for both 'type 5' (as in the previous version) and 'type 3' (i.e., with
		  JPEG previews).
		  To keep the number of (type 3) choices under control, we remove support
		  for H.265 120fps and H.264 120 fps.  If, however, someone needs these,
		  then let us know and we'll re-add them.
    - 2021-11-08: We now support an additional video format - H.264 720p24 (type 5)
    - 2021-12-02: We now handle 'isom' in addition to 'ftyp' at/near the start of the file.
    - 2022-01-13: We now handle a more general metadata block format (seen in the Mavic Mini SE)
    - 2022-03-28: Made the parsing/skipping of metadata blocks more robust.  Sometimes, the
                  'tail end' of the metadata blocks is non-printable-ASCII, which means that it
		  really doesn't exist.
    - 2022-04-17: Replaced the SPS data for (type 3) H.264 1530p, 48 fps so that it conforms
                  to video produced by newer DJI drones.
    - 2022-07-04: Fixed a bug in the allocation of the name for the output filename.
    - 2022-08-16: Improved the checking for video in 'type 4' repairs
    - 2022-09-05: Added a new format for 'type 5': H.264 1080p30 (Mavic Air)
    - 2023-01-14: Added a new format for 'type 5': H.264 1080p25 (Mavic Air)
    - 2023-04-19: Updated the format for 1080p30 'type 2' for (presumably) more modern devices
    - 2023-05-05: We can now identify and skip over some more types of Osmo audio data.
                  Added a new format for 'type 5': H.264 2160(x3840)p25.
*/

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static void usage(char const* progName) {
  fprintf(stderr, "Usage: %s name-of-video-file-to-repair\n", progName);
}

static int checkFor0x00000002(unsigned first4Bytes, unsigned next4Bytes) {
  /* We check not just that "first4Bytes" is 0x00000002, but also that "next4Bytes" starts
     with two non-zero bytes, and then a zero byte: */
  return first4Bytes == 0x00000002
    && (next4Bytes&0xFF000000) != 0
    && (next4Bytes&0x00FF0000) != 0
    && (next4Bytes&0x0000FF00) == 0;
}

static int checkForVideo(unsigned first4Bytes, unsigned next4Bytes) {
  /* An expanded version of "checkFor0x00000002()", where we also check for what appears to
     be an initial SPS NAL unit (preceded by length) */
  if ((first4Bytes&0xFFFFFF00) != 0) return 0;

  return (first4Bytes == 0x00000002
	  && (next4Bytes&0xFF000000) != 0
	  && (next4Bytes&0x00FF0000) != 0
	  && (next4Bytes&0x0000FF00) == 0)
    || ((next4Bytes&0xFF000000) == 0x27000000
	&& first4Bytes > 25 && first4Bytes < 60)
    || ((next4Bytes&0xFF000000) == 0x40000000
	&& first4Bytes > 30 && first4Bytes < 60)
    || ((next4Bytes&0xFF000000) == 0x67000000
	&& first4Bytes > 10 && first4Bytes < 40);
}

static int checkForVideoType4(unsigned first4Bytes, unsigned next4Bytes) {
  /* A special version of "checkForVideo()" that works well for 'type 4' repairs: */
  unsigned char nextByte, nextNextByte;

  if (first4Bytes == 0 || first4Bytes > 0x008FFFFF) return 0; /* nalSize would be bad */

  nextByte = next4Bytes>>24;
  nextNextByte = next4Bytes>>16;
  switch (nextByte) {
    case 0x00: return nextNextByte == 0x01;
//    case 0x01: return (nextNextByte >= 0x1e && nextNextByte < 0x40) || nextNextByte >= 0xF0;
    case 0x01: return nextNextByte == 0xFD;
    case 0x02: return nextNextByte == 0x01;
    case 0x26: return nextNextByte == 0x01;
    case 0x28: return nextNextByte == 0x01;
    case 0x40: return nextNextByte == 0x01;
    case 0x41: return (nextNextByte >= 0xE0 && nextNextByte <= 0xFC);
    case 0x42: return nextNextByte == 0x01;
    case 0x44: return nextNextByte == 0x01;
//    case 0x61: return nextNextByte >= 0xE0;
//    case 0x65: return nextNextByte >= 0xB0;
    case 0x65: return nextNextByte == 0xB8;
//    case 0x67: return nextNextByte >= 0x60; // SPS needs to be correct, so ignore this
//    case 0x68: return nextNextByte >= 0xE0; // PPS needs to be correct, so ignore this
//    case 0x6D: return nextNextByte >= 0x60;
    default: return 0;
  }
}

#define fourcc_free (('f'<<24)|('r'<<16)|('e'<<8)|'e')
#define fourcc_ftyp (('f'<<24)|('t'<<16)|('y'<<8)|'p')
#define fourcc_isom (('i'<<24)|('s'<<16)|('o'<<8)|'m')
#define fourcc_mdat (('m'<<24)|('d'<<16)|('a'<<8)|'t')
#define fourcc_mijd (('m'<<24)|('i'<<16)|('j'<<8)|'d')
#define fourcc_moov (('m'<<24)|('o'<<16)|('o'<<8)|'v')
#define fourcc_wide (('w'<<24)|('i'<<16)|('d'<<8)|'e')

static int get1Byte(FILE* fid, unsigned char* result); /* forward */
static int get2Bytes(FILE* fid, unsigned* result); /* forward */
static int get4Bytes(FILE* fid, unsigned* result); /* forward */
static int checkAtom(FILE* fid, unsigned fourccToCheck, unsigned* numRemainingBytesToSkip); /* forward */
static void doRepairType1(FILE* inputFID, FILE* outputFID, unsigned ftypSize); /* forward */
static void doRepairType2(FILE* inputFID, FILE* outputFID, unsigned second4Bytes); /* forward */
static void doRepairType3(FILE* inputFID, FILE* outputFID); /* forward */
static void doRepairType4(FILE* inputFID, FILE* outputFID); /* forward */
static void doRepairType5(FILE* inputFID, FILE* outputFID); /* forward */
static void doRepairType3or5Common(FILE* inputFID, FILE* outputFID); /* forward */

static char const* versionStr = "2023-05-05";
static char const* repairedFilenameStr = "-repaired";
static char const* startingToRepair = "Repairing the file (please wait)...";
static char const* cantRepair = "  We cannot repair this file!";

#ifdef CODE_COUNT
unsigned codeCount[65536];
#endif
int main(int argc, char** argv) {
  char* inputFileName;
  char* outputFileName;
  FILE* inputFID;
  FILE* outputFID;
  unsigned numBytesToSkip, dummy;
  int repairType = 1; /* by default */
  unsigned repairType1FtypSize; /* used only for 'repair type 1' files */
  unsigned repairType2Second4Bytes; /* used only for 'repair type 2' files */

  do {
    fprintf(stderr, "%s, version %s; Copyright (c) 2014-2023 Live Networks, Inc. All rights reserved.\n", argv[0], versionStr);
    fprintf(stderr, "The latest version of this software is available at http://djifix.live555.com/\n\n");

    if (argc != 2) {
      usage(argv[0]);
      break;
    }
    inputFileName = argv[1];

    /* Open the input file: */
    inputFID  = fopen(inputFileName, "rb");
    if (inputFID == NULL) {
      perror("Failed to open file to repair");
      break;
    }

    /* Check the first 8 bytes of the file, to see whether the file starts with a 'ftyp' atom
       (repair type 1), or H.264/H.265 NAL units (repair type 2 or 3): */
    {
      unsigned first4Bytes, next4Bytes;
      int fileStartIsOK;
      int amAtStartOfFile = 1;

      if (!get4Bytes(inputFID, &first4Bytes) || !get4Bytes(inputFID, &next4Bytes)) {
	fprintf(stderr, "Unable to read the start of the file.%s\n", cantRepair);
	break;
      }

      fileStartIsOK = 1;
      while (1) {
	if (next4Bytes == fourcc_ftyp || next4Bytes == fourcc_isom) {
	  /* Repair type 1 */
	  if (first4Bytes < 8 || first4Bytes > 0x000000FF) {
	    fprintf(stderr, "Ignoring bad length 0x%08x for initial 'ftyp' or 'isom' atom\n", first4Bytes);
	  } else if (fseek(inputFID, first4Bytes-8, SEEK_CUR) != 0) {
	    fprintf(stderr, "Bad length for initial 'ftyp' or 'isom' atom.%s\n", cantRepair);
	    fileStartIsOK = 0;
	  } else {
	    if (!amAtStartOfFile) fprintf(stderr, "Found 'ftyp' or 'isom' (at file position 0x%08lx)\n", ftell(inputFID) - 8); else fprintf(stderr, "Saw initial 'ftyp'or 'isom'.\n");
	  }
	} else if (checkFor0x00000002(first4Bytes, next4Bytes)) {
	  /* Assume repair type 2 */
	  if (!amAtStartOfFile) fprintf(stderr, "Found 0x00000002 (at file position 0x%08lx)\n", ftell(inputFID) - 8);
	  repairType = 2;
	  repairType2Second4Bytes = next4Bytes;
	} else if (first4Bytes == 0x00000000 || first4Bytes == 0xFFFFFFFF) {
	  /* Skip initial 0x00000000 or 0xFFFFFFFF data at the start of the file: */
	  if (amAtStartOfFile) {
	    fprintf(stderr, "Skipping initial junk 0x%08X bytes at the start of the file...\n", first4Bytes);
	    amAtStartOfFile = 0;
	  }
	  first4Bytes = next4Bytes;
	  if (!get4Bytes(inputFID, &next4Bytes)) {
	    fprintf(stderr, "File appears to contain nothing but zeros or 0xFF!%s\n", cantRepair);
	    fileStartIsOK = 0;
	  } else {
	    continue;
	  }
	} else {
	  /* There's garbage at the beginning of the file.  Skip until we find sane data: */
	  unsigned char c;

	  if (amAtStartOfFile) {
	    fprintf(stderr, "Didn't see an initial 'ftyp' or 'isom' atom, or 0x00000002.  Looking for data that we understand...\n");
	    amAtStartOfFile = 0;
	  }
	  if (!get1Byte(inputFID, &c)) {
	    /* We reached the end of the file, without seeing any data that we understand! */
	    fprintf(stderr, "...Unable to find sane initial data.%s\n", cantRepair);
	    fileStartIsOK = 0;
	  } else {
	    /* Shift "first4Bytes" and "next4Bytes" 1-byte to the left, and keep trying: */
	    first4Bytes = ((first4Bytes<<8)&0xFFFFFF00) | ((next4Bytes>>24)&0x000000FF);
	    next4Bytes = ((next4Bytes<<8)&0xFFFFFF00) | c;
	    continue;
	  }
	}

	/* If we get here, we've found the initial data that we're looking for, or end-of-file: */
	break;
      }
      if (!fileStartIsOK) break; /* An error occurred at/near the start of the file */
    }

    if (repairType == 1) {
      /* Check for a 'moov' atom next: */
      if (checkAtom(inputFID, fourcc_moov, &numBytesToSkip)) {
	fprintf(stderr, "Saw 'moov' (size %d == 0x%08x).\n", 8+numBytesToSkip, 8+numBytesToSkip);
	if (fseek(inputFID, numBytesToSkip, SEEK_CUR) != 0) {
	  fprintf(stderr, "Input file was truncated before end of 'moov'.%s\n", cantRepair);
	  break;
	}
      } else {
	fprintf(stderr, "Didn't see a 'moov' atom.\n");
	/* It's possible that this was a 'mdat' atom instead.  Check for that next: */
      }

      /* Check for a 'free' or a 'wide' atom that sometimes appears before 'mdat': */
      if (checkAtom(inputFID, fourcc_free, &numBytesToSkip)) {
	fprintf(stderr, "Saw 'free' (size %d == 0x%08x).\n", 8+numBytesToSkip, 8+numBytesToSkip);
	if (fseek(inputFID, numBytesToSkip, SEEK_CUR) != 0) {
	  fprintf(stderr, "Input file was truncated before end of 'free'.%s\n", cantRepair);
	  break;
	}
      } else if (checkAtom(inputFID, fourcc_wide, &numBytesToSkip)) {
	fprintf(stderr, "Saw 'wide'.\n");
	if (numBytesToSkip > 0) {
	  fprintf(stderr, "Warning: 'wide' atom size was %d (>8)\n", 8+numBytesToSkip);
	  if (fseek(inputFID, numBytesToSkip, SEEK_CUR) != 0) {
	    fprintf(stderr, "Input file was truncated before end of 'wide'.%s\n", cantRepair);
	    break;
	  }
	}
      }

      /* Check for a 'mdat' atom next: */
      if (checkAtom(inputFID, fourcc_mdat, &dummy)) {
	fprintf(stderr, "Saw 'mdat'.\n");
      
	/* Check whether the 'mdat' data begins with a 'ftyp' atom: */
	if (checkAtom(inputFID, fourcc_ftyp, &numBytesToSkip)) {
	  /* On rare occasions, this situation is repeated: The remainder of the file consists
	     of 'ftyp', 'moov', 'mdat' - with the 'mdat' data beginning with 'ftyp' again.
	     Check for this now:
	  */
	  long curPos;

	  while (1) {	
	    unsigned nbts_moov;

	    curPos = ftell(inputFID); /* remember where we are now */
	    if (fseek(inputFID, numBytesToSkip, SEEK_CUR) != 0) break;
	    if (!checkAtom(inputFID, fourcc_moov, &nbts_moov)) break;
	    if (fseek(inputFID, nbts_moov, SEEK_CUR) != 0) break;
	    if (!checkAtom(inputFID, fourcc_mdat, &dummy)) break; /* can 0x0000002 ever occur? */
	    if (!checkAtom(inputFID, fourcc_ftyp, &numBytesToSkip)) break;
	    fprintf(stderr, "(Saw nested 'ftyp' within 'mdat')\n");
	  }
	  fseek(inputFID, curPos, SEEK_SET); /* restore our old position */

	  repairType1FtypSize = numBytesToSkip+8;
	  fprintf(stderr, "Saw a 'ftyp' within the 'mdat' data.  We can repair this file.\n");
	} else {
	  unsigned next4Bytes;

	  fprintf(stderr, "Didn't see a 'ftyp' atom inside the 'mdat' data.\n");
	  /* It's possible that the 'mdat' data began with 0x00000002 (i.e., a 'type 2' repair) */
	  repairType = 2;
	  /* But first, check for the four bytes 'm','i','j','d'; or 0xFFD8FFE0 (JFIF header);
	     indicating a 'type 3' repair: */
	  if (get4Bytes(inputFID, &next4Bytes)) {
	    if (next4Bytes == fourcc_mijd) {
	      fprintf(stderr, "Saw 'mijd'.\n");
	      repairType = 3; /* New-style MP4 file containing a JPEG preview */
	    } else if (next4Bytes == 0xFFD8FFE0) {
	      fprintf(stderr, "Saw 'JFIF' header.\n");
	      repairType = 3; /* New-style MP4 file containing a JPEG preview */
	    } else {
	      fseek(inputFID, -4, SEEK_CUR);
	    }
	  }
	}
      } else {
	fprintf(stderr, "Didn't see a 'mdat' atom.\n");
	/* It's possible that the remaining bytes begin with 0x00000002 (i.e., a 'type 2' repair).*/
	/* Check for that next: */
	repairType = 2;
      }

      if (repairType == 2) {
	unsigned first4Bytes, next4Bytes;
	int sawVideo = 0;

	/* Check for known video occurring next: */
	fprintf(stderr, "Looking for video data...\n");
	if (get4Bytes(inputFID, &first4Bytes) && get4Bytes(inputFID, &next4Bytes)) {
	  while (1) {
	    if (checkForVideo(first4Bytes, next4Bytes)) {
	      sawVideo = 1;
	      if (first4Bytes == 0x00000002) {
		fprintf(stderr, "Found 0x00000002 (at file position 0x%08lx)\n", ftell(inputFID) - 8);
		repairType2Second4Bytes = next4Bytes;
	      } else {
		fprintf(stderr, "Found apparent H.264 or H.265 SPS (length %d, at file position 0x%08lx)\n", first4Bytes, ftell(inputFID) - 8);
		fseek(inputFID, -8, SEEK_CUR);
		repairType = 4; /* special case */
	      }
	      break;
	    } else if (first4Bytes < 0x01000000 && (next4Bytes&0xFFFF0000) == 0x65B80000) {
	      /* A special case: This looks like H.264 data for a DJI Mini 2 or Mavic Air ('type 5') video */
	      sawVideo = 1;
	      fprintf(stderr, "Found possible H.264 video data, at file position 0x%08lx\n",
		      ftell(inputFID) - 8);
	      fseek(inputFID, -8, SEEK_CUR);
	      repairType = 5;
	      break;
	    } else {
	      unsigned char c;

	      if (!get1Byte(inputFID, &c)) break;/*eof*/
	      first4Bytes = ((first4Bytes<<8)&0xFFFFFF00) | ((next4Bytes>>24)&0x000000FF);
	      next4Bytes = ((next4Bytes<<8)&0xFFFFFF00) | c;
	    }
	  }
	}

	if (!sawVideo) {
	  /* OK, now we have to give up: */
	  fprintf(stderr, "Didn't see any obvious video data.%s\n", cantRepair);
	  break;
	}
      } else if (repairType == 3) {
	unsigned char byte1 = 0;
	int sawEndOfJPEGs = 0;

	/* Skip over all JPEG previews (ending with 0xFFD9, and not then followed by 0xFFD8): */
	fprintf(stderr, "Skipping past JPEG previews...\n");
	while (1) {
	  unsigned char byte2;

	  if (!get1Byte(inputFID, &byte2)) break;/*eof*/
	  if (byte1 == 0xFF && byte2 == 0xD9) {
	    unsigned char byte3, byte4;
	    if (!get1Byte(inputFID, &byte3) || !get1Byte(inputFID, &byte4)) break;/*eof*/
	    if (!(byte3 == 0xFF && byte4 == 0xD8)) {
	      fseek(inputFID, -2, SEEK_CUR);
	      fprintf(stderr, "Found movie data (at file position 0x%08lx)\n", ftell(inputFID));
	      sawEndOfJPEGs = 1;
	      break;
	    } else {
	      byte1 = 0; /* for the next iteration */
	    }
	  } else {
	    byte1 = byte2; /* for the next iteration */
	  }
	}

	if (!sawEndOfJPEGs) {
	  /* OK, now we have to give up: */
	  fprintf(stderr, "Didn't see end of JPEG previews.%s\n", cantRepair);
	  break;
	}

	/* Sometimes, the movie data here begins with a 'mdat' atom header. Check for this now: */
	if (checkAtom(inputFID, fourcc_mdat, &dummy)) {
	  fprintf(stderr, "Saw 'mdat'.\n");
	}
      }
    }

    if (repairType > 1) {
      fprintf(stderr, "We can repair this file, but the result will be a '.h264' file (playable by the VLC or IINA media player), not a '.mp4' file.\n");
    }

    /* Now generate the output file name, and open the output file: */
    {
      unsigned suffixLen, outputFileNameSize;
      char* dotPtr = strrchr(inputFileName, '.');
      if (dotPtr == NULL) {
	dotPtr = &inputFileName[strlen(inputFileName)];
      }
      *dotPtr = '\0';
      
      suffixLen = repairType == 1 ? 3/*mp4*/ : 4/*h264*/;
      outputFileNameSize = (dotPtr - inputFileName) + strlen(repairedFilenameStr) + 1/*dot*/ + suffixLen + 1/*trailing '\0'*/;
      outputFileName = malloc(outputFileNameSize);
      sprintf(outputFileName, "%s%s.%s", inputFileName, repairedFilenameStr,
	      repairType == 1 ? "mp4" : "h264");

      outputFID = fopen(outputFileName, "wb");
      if (outputFID == NULL) {
	perror("Failed to open output file");
	free(outputFileName);
	break;
      }
    }

    /* Begin the repair: */
    if (repairType == 1) {
      doRepairType1(inputFID, outputFID, repairType1FtypSize);
    } else if (repairType == 2) {
      doRepairType2(inputFID, outputFID, repairType2Second4Bytes);
    } else if (repairType == 3) {
      doRepairType3(inputFID, outputFID);
    } else if (repairType == 4) {
      doRepairType4(inputFID, outputFID);
    } else if (repairType == 5) {
      doRepairType5(inputFID, outputFID);
    }

    fprintf(stderr, "...done\n");
    fclose(outputFID);
    fprintf(stderr, "\nRepaired file is \"%s\"\n", outputFileName);
    free(outputFileName);
#ifdef CODE_COUNT
    for (unsigned i = 0; i < 65536; ++i) if (codeCount[i] > 0) fprintf(stderr, "0x%04x: %d\n", i, codeCount[i]);
#endif

    if (repairType > 1) {
      fprintf(stderr, "This file can be played by the VLC media player (available at <http://www.videolan.org/vlc/>), or by the IINA media player (for MacOS; available at <https://lhc70000.github.io/iina/>).\n");
    }

    /* OK */
    return 0;
  } while (0);

  /* An error occurred: */
  return 1;
}

static int get1Byte(FILE* fid, unsigned char* result) {
  int fgetcResult;

  fgetcResult = fgetc(fid);
  if (feof(fid) || ferror(fid)) return 0;

  *result = (unsigned char)fgetcResult;
  return 1;
}

static int get2Bytes(FILE* fid, unsigned* result) {
  unsigned char c1, c2;

  if (!get1Byte(fid, &c1)) return 0;
  if (!get1Byte(fid, &c2)) return 0;

  *result = (c1<<8)|c2;
  return 1;
}

static int get4Bytes(FILE* fid, unsigned* result) {
  unsigned char c1, c2, c3, c4;

  if (!get1Byte(fid, &c1)) return 0;
  if (!get1Byte(fid, &c2)) return 0;
  if (!get1Byte(fid, &c3)) return 0;
  if (!get1Byte(fid, &c4)) return 0;

  *result = (c1<<24)|(c2<<16)|(c3<<8)|c4;
  return 1;
}

static int checkAtom(FILE* fid, unsigned fourccToCheck, unsigned* numRemainingBytesToSkip) {
  do {
    unsigned atomSize, fourcc;

    if (!get4Bytes(fid, &atomSize)) break;

    if (!get4Bytes(fid, &fourcc) || fourcc != fourccToCheck) break;
    
    /* For 'mdat' atoms, ignore the size, because we don't use it: */
    if (fourcc == fourcc_mdat) return 1;

    if (atomSize == 1) {
      fprintf(stderr, "Saw an extended (64-bit) atom size.  We currently don't handle this!\n");
      exit(1);
    }

    /* Check the atom size.  It should be >= 8. */
    if (atomSize < 8) break;
    *numRemainingBytesToSkip = atomSize - 8;

    return 1;
  } while (0);

  /* An error occurred. Rewind over the bytes that we read (assuming we read all 8): */
  if (fseek(fid, -8, SEEK_CUR) != 0) {
    fprintf(stderr, "Failed to rewind 8 bytes.%s\n", cantRepair);
  }
  return 0;
}

static void doRepairType1(FILE* inputFID, FILE* outputFID, unsigned ftypSize) {
  fprintf(stderr, "%s", startingToRepair);

  /* Begin the repair by writing the header for the initial 'ftype' atom: */
  fputc(ftypSize>>24, outputFID);
  fputc(ftypSize>>16, outputFID);
  fputc(ftypSize>>8, outputFID);
  fputc(ftypSize, outputFID);

  fputc('f', outputFID); fputc('t', outputFID); fputc('y', outputFID); fputc('p', outputFID);

  /* Then complete the repair by copying from the input file to the output file: */
  {
    unsigned char c;

    while (get1Byte(inputFID, &c)) fputc(c, outputFID);
  }
}

#define wr(c) fputc((c), outputFID)

static void putStartCode(FILE* outputFID) {
  wr(0x00); wr(0x00); wr(0x00); wr(0x01);
}

static unsigned char SPS_2160p30[] = { 0x27, 0x64, 0x00, 0x33, 0xac, 0x34, 0xc8, 0x03, 0xc0, 0x04, 0x3e, 0xc0, 0x5a, 0x80, 0x80, 0x80, 0xa0, 0x00, 0x00, 0x7d, 0x20, 0x00, 0x1d, 0x4c, 0x1d, 0x0c, 0x00, 0x07, 0x27, 0x08, 0x00, 0x01, 0xc9, 0xc3, 0x97, 0x79, 0x71, 0xa1, 0x80, 0x00, 0xe4, 0xe1, 0x00, 0x00, 0x39, 0x38, 0x72, 0xef, 0x2e, 0x1f, 0x08, 0x84, 0x53, 0x80, 0xfe };
/* The following was used in an earlier version of the software, but does not appear to be correct:
static unsigned char SPS_2160p25[] = { 0x27, 0x64, 0x00, 0x33, 0xac, 0x34, 0xc8, 0x01, 0x00, 0x01, 0x0f, 0xb0, 0x16, 0xa0, 0x20, 0x20, 0x28, 0x00, 0x00, 0x1f, 0x40, 0x00, 0x06, 0x1a, 0x87, 0x43, 0x00, 0x01, 0xc9, 0xc2, 0x00, 0x00, 0x72, 0x70, 0xe5, 0xde, 0x5c, 0x68, 0x60, 0x00, 0x39, 0x38, 0x40, 0x00, 0x0e, 0x4e, 0x1c, 0xbb, 0xcb, 0x87, 0xc2, 0x21, 0x14, 0xe0, 0xfe };
*/
static unsigned char SPS_2160x4096p25[] = { 0x27, 0x64, 0x00, 0x33, 0xac, 0x34, 0xc8, 0x01, 0x00, 0x01, 0x0f, 0xb0, 0x16, 0xa0, 0x20, 0x20, 0x28, 0x00, 0x00, 0x1f, 0x40, 0x00, 0x06, 0x1a, 0x87, 0x43, 0x00, 0x01, 0xc9, 0xc2, 0x00, 0x00, 0x72, 0x70, 0xe5, 0xde, 0x5c, 0x68, 0x60, 0x00, 0x39, 0x38, 0x40, 0x00, 0x0e, 0x4e, 0x1c, 0xbb, 0xcb, 0x87, 0xc2, 0x21, 0x14, 0xe0, 0xfe }; 
static unsigned char SPS_2160x3840p25[] = { 0x27, 0x64, 0x00, 0x33, 0xac, 0x34, 0xc8, 0x03, 0xc0, 0x04, 0x3e, 0xc0, 0x5a, 0x80, 0x80, 0x80, 0xa0, 0x00, 0x00, 0x7d, 0x00, 0x00, 0x18, 0x6a, 0x1d, 0x0c, 0x00, 0x07, 0x27, 0x08, 0x00, 0x01, 0xc9, 0xc3, 0x97, 0x79, 0x71, 0xa1, 0x80, 0x00, 0xe4, 0xe1, 0x00, 0x00, 0x39, 0x38, 0x72, 0xef, 0x2e, 0x1f, 0x08, 0x84, 0x53, 0x80, 0xfe };
static unsigned char SPS_2160x4096p24[] = { 0x27, 0x64, 0x00, 0x33, 0xac, 0x34, 0xc8, 0x01, 0x00, 0x01, 0x0f, 0xb0, 0x16, 0xa0, 0x20, 0x20, 0x28, 0x00, 0x00, 0x1f, 0x48, 0x00, 0x05, 0xdc, 0x07, 0x43, 0x00, 0x01, 0xc9, 0xc2, 0x00, 0x00, 0x72, 0x70, 0xe5, 0xde, 0x5c, 0x68, 0x60, 0x00, 0x39, 0x38, 0x40, 0x00, 0x0e, 0x4e, 0x1c, 0xbb, 0xfe };
static unsigned char SPS_2160x3840p24[] = { 0x27, 0x64, 0x00, 0x33, 0xac, 0x34, 0xc8, 0x03, 0xc0, 0x04, 0x3e, 0xc0, 0x5a, 0x80, 0x80, 0x80, 0xa0, 0x00, 0x00, 0x7d, 0x20, 0x00, 0x17, 0x70, 0x1d, 0x0c, 0x00, 0x07, 0x27, 0x08, 0x00, 0x01, 0xc9, 0xc3, 0x97, 0x79, 0x71, 0xa1, 0x80, 0x00, 0xe4, 0xe1, 0x00, 0x00, 0x39, 0x38, 0x72, 0xef, 0x2e, 0x1f, 0x08, 0x84, 0x53, 0x80, 0xfe };
static unsigned char SPS_1530p30[] = { 0x27, 0x64, 0x00, 0x29, 0xac, 0x34, 0xc8, 0x02, 0xa8, 0x0c, 0x1b, 0x01, 0x6a, 0x02, 0x02, 0x02, 0x80, 0x00, 0x01, 0xf4, 0x80, 0x00, 0x75, 0x30, 0x74, 0x30, 0x00, 0x15, 0x75, 0x20, 0x00, 0x05, 0x5d, 0x4a, 0x5d, 0xe5, 0xc6, 0x86, 0x00, 0x02, 0xae, 0xa4, 0x00, 0x00, 0xab, 0xa9, 0x4b, 0xbc, 0xb8, 0x7c, 0x22, 0x11, 0x4e, 0x00, 0x00, 0x00, 0xfe };
static unsigned char SPS_1530p25[] = { 0x27, 0x64, 0x00, 0x32, 0xac, 0x34, 0xc8, 0x02, 0xa8, 0x0c, 0x1b, 0x01, 0x6a, 0x02, 0x02, 0x02, 0x80, 0x00, 0x01, 0xf4, 0x00, 0x00, 0x61, 0xa8, 0x74, 0x30, 0x00, 0x15, 0x75, 0x20, 0x00, 0x05, 0x5d, 0x4a, 0x5d, 0xe5, 0xc6, 0x86, 0x00, 0x02, 0xae, 0xa4, 0x00, 0x00, 0xab, 0xa9, 0x4b, 0xbc, 0xb8, 0x7c, 0x22, 0x11, 0x4e, 0xfe };
static unsigned char SPS_1530p24[] = { 0x27, 0x64, 0x00, 0x32, 0xac, 0x34, 0xc8, 0x02, 0xa8, 0x0c, 0x1b, 0x01, 0x6a, 0x02, 0x02, 0x02, 0x80, 0x00, 0x01, 0xf4, 0x80, 0x00, 0x5d, 0xc0, 0x74, 0x30, 0x00, 0x15, 0x75, 0x20, 0x00, 0x05, 0x5d, 0x4a, 0x5d, 0xe5, 0xc6, 0x86, 0x00, 0x02, 0xae, 0xa4, 0x00, 0x00, 0xab, 0xa9, 0x4b, 0xbc, 0xb8, 0x7c, 0x22, 0x11, 0x4e, 0xfe };
static unsigned char SPS_1520p60[] = { 0x27, 0x64, 0x00, 0x2a, 0xac, 0x34, 0xc8, 0x02, 0xa4, 0x0b, 0xfb, 0x01, 0x6e, 0x02, 0x02, 0x02, 0x80, 0x00, 0x01, 0xf4, 0x80, 0x00, 0xea, 0x60, 0x74, 0x30, 0x00, 0x15, 0x75, 0x20, 0x00, 0x05, 0x5d, 0x4a, 0x5d, 0xe5, 0xc6, 0x86, 0x00, 0x02, 0xae, 0xa4, 0x00, 0x00, 0xab, 0xa9, 0x4b, 0xbc, 0xb8, 0x7c, 0x22, 0x11, 0x4e, 0x00, 0x00, 0x00, 0xfe };
static unsigned char SPS_1520p30[] = { 0x27, 0x64, 0x00, 0x29, 0xac, 0x34, 0xc8, 0x02, 0xa4, 0x0b, 0xfb, 0x01, 0x6a, 0x02, 0x02, 0x02, 0x80, 0x00, 0x01, 0xf4, 0x80, 0x00, 0x75, 0x30, 0x74, 0x30, 0x00, 0x13, 0x12, 0xc0, 0x00, 0x04, 0xc4, 0xb4, 0x5d, 0xe5, 0xc6, 0x86, 0x00, 0x02, 0x62, 0x58, 0x00, 0x00, 0x98, 0x96, 0x8b, 0xbc, 0xb8, 0x7c, 0x22, 0x11, 0x4e, 0x00, 0x00, 0x00, 0xfe };
static unsigned char SPS_1520p25[] = { 0x27, 0x64, 0x00, 0x29, 0xac, 0x34, 0xc8, 0x02, 0xa4, 0x0b, 0xfb, 0x01, 0x6a, 0x02, 0x02, 0x02, 0x80, 0x00, 0x00, 0x03, 0x00, 0x80, 0x00, 0x00, 0x19, 0x74, 0x30, 0x00, 0x13, 0x12, 0xc0, 0x00, 0x04, 0xc4, 0xb4, 0x5d, 0xe5, 0xc6, 0x86, 0x00, 0x02, 0x62, 0x58, 0x00, 0x00, 0x98, 0x96, 0x8b, 0xbc, 0xb8, 0x7c, 0x22, 0x11, 0x4e, 0xfe };
static unsigned char SPS_1520p24[] = { 0x27, 0x64, 0x00, 0x29, 0xac, 0x34, 0xc8, 0x02, 0xa4, 0x0b, 0xfb, 0x01, 0x6a, 0x02, 0x02, 0x02, 0x80, 0x00, 0x01, 0xf4, 0x80, 0x00, 0x5d, 0xc0, 0x74, 0x30, 0x00, 0x15, 0x75, 0x20, 0x00, 0x05, 0x5d, 0x4a, 0x5d, 0xe5, 0xc6, 0x86, 0x00, 0x02, 0xae, 0xa4, 0x00, 0x00, 0xab, 0xa9, 0x4b, 0xbc, 0xb8, 0x7c, 0x22, 0x11, 0x4e, 0x00, 0x00, 0x00, 0xfe };
static unsigned char SPS_1080p60[] = { 0x27, 0x64, 0x00, 0x2a, 0xac, 0x34, 0xc8, 0x07, 0x80, 0x22, 0x7e, 0x5c, 0x05, 0xa8, 0x08, 0x08, 0x0a, 0x00, 0x00, 0x07, 0xd2, 0x00, 0x03, 0xa9, 0x81, 0xd0, 0xc0, 0x00, 0x4c, 0x4b, 0x00, 0x00, 0x13, 0x12, 0xd1, 0x77, 0x97, 0x1a, 0x18, 0x00, 0x09, 0x89, 0x60, 0x00, 0x02, 0x62, 0x5a, 0x2e, 0xf2, 0xe1, 0xf0, 0x88, 0x45, 0x16, 0xfe };
static unsigned char SPS_1080i60[] = { 0x27, 0x4d, 0x00, 0x2a, 0x9a, 0x66, 0x03, 0xc0, 0x22, 0x3e, 0xf0, 0x16, 0xc8, 0x00, 0x00, 0x1f, 0x48, 0x00, 0x07, 0x53, 0x07, 0x43, 0x00, 0x02, 0x36, 0x78, 0x00, 0x02, 0x36, 0x78, 0x5d, 0xe5, 0xc6, 0x86, 0x00, 0x04, 0x6c, 0xf0, 0x00, 0x04, 0x6c, 0xf0, 0xbb, 0xcb, 0x87, 0xc2, 0x21, 0x14, 0x58, 0xfe };
static unsigned char SPS_1080p50[] = { 0x27, 0x64, 0x00, 0x29, 0xac, 0x34, 0xc8, 0x07, 0x80, 0x22, 0x7e, 0x5c, 0x05, 0xa8, 0x08, 0x08, 0x0a, 0x00, 0x00, 0x07, 0xd0, 0x00, 0x03, 0x0d, 0x41, 0xd0, 0xc0, 0x00, 0x4c, 0x4b, 0x00, 0x00, 0x13, 0x12, 0xd1, 0x77, 0x97, 0x1a, 0x18, 0x00, 0x09, 0x89, 0x60, 0x00, 0x02, 0x62, 0x5a, 0x2e, 0xf2, 0xe1, 0xf0, 0x88, 0x45, 0x16, 0xfe };
static unsigned char SPS_1080p48[] = { 0x27, 0x64, 0x00, 0x2a, 0xac, 0x34, 0xc8, 0x07, 0x80, 0x22, 0x7e, 0x5c, 0x05, 0xa8, 0x08, 0x08, 0x0a, 0x00, 0x00, 0x07, 0xd2, 0x00, 0x02, 0xee, 0x01, 0xd0, 0xc0, 0x00, 0x4c, 0x4b, 0x00, 0x00, 0x13, 0x12, 0xd1, 0x77, 0x97, 0x1a, 0x18, 0x00, 0x09, 0x89, 0x60, 0x00, 0x02, 0x62, 0x5a, 0x2e, 0xf2, 0xe1, 0xf0, 0x88, 0x45, 0x16, 0xfe };
/* Old format; assume no longer used:
static unsigned char SPS_1080p30_default[] = { 0x27, 0x4d, 0x00, 0x28, 0x9a, 0x66, 0x03, 0xc0, 0x11, 0x3f, 0x2e, 0x02, 0xd9, 0x00, 0x00, 0x03, 0x03, 0xe9, 0x00, 0x00, 0xea, 0x60, 0xe8, 0x60, 0x00, 0xe2, 0x98, 0x00, 0x03, 0x8a, 0x60, 0xbb, 0xcb, 0x8d, 0x0c, 0x00, 0x1c, 0x53, 0x00, 0x00, 0x71, 0x4c, 0x17, 0x79, 0x70, 0xf8, 0x44, 0x22, 0x8b, 0xfe };
*/
static unsigned char SPS_1080p30_default[] = { 0x67, 0x4d, 0x00, 0x1f, 0x93, 0x28, 0x08, 0x00, 0x93, 0x7f, 0xe0, 0x00, 0x20, 0x00, 0x28, 0x10, 0x00, 0x00, 0x03, 0x00, 0x10, 0x00, 0x00, 0x03, 0x03, 0xc8, 0xda, 0x08, 0x84, 0x65, 0x80, 0xfe };
static unsigned char SPS_1080p30_advanced[] = { 0x27, 0x64, 0x00, 0x29, 0xac, 0x34, 0xc8, 0x07, 0x80, 0x22, 0x7e, 0x5c, 0x05, 0xa8, 0x08, 0x08, 0x0a, 0x00, 0x00, 0x07, 0xd2, 0x00, 0x01, 0xd4, 0xc1, 0xd0, 0xc0, 0x00, 0x4c, 0x4b, 0x00, 0x00, 0x13, 0x12, 0xd1, 0x77, 0x97, 0x1a, 0x18, 0x00, 0x09, 0x89, 0x60, 0x00, 0x02, 0x62, 0x5a, 0x2e, 0xf2, 0xe1, 0xf0, 0x88, 0x45, 0x16, 0xfe };
static unsigned char SPS_1080p25[] = { 0x27, 0x4d, 0x00, 0x28, 0x9a, 0x66, 0x03, 0xc0, 0x11, 0x3f, 0x2e, 0x02, 0xd9, 0x00, 0x00, 0x03, 0x03, 0xe8, 0x00, 0x00, 0xc3, 0x50, 0xe8, 0x60, 0x00, 0xdc, 0xf0, 0x00, 0x03, 0x73, 0xb8, 0xbb, 0xcb, 0x8d, 0x0c, 0x00, 0x1b, 0x9e, 0x00, 0x00, 0x6e, 0x77, 0x17, 0x79, 0x70, 0xf8, 0x44, 0x22, 0x8b, 0xfe };
static unsigned char SPS_1080p24[] = { 0x27, 0x64, 0x00, 0x29, 0xac, 0x34, 0xc8, 0x07, 0x80, 0x22, 0x7e, 0x5c, 0x05, 0xa8, 0x08, 0x08, 0x0a, 0x00, 0x00, 0x07, 0xd2, 0x00, 0x01, 0x77, 0x01, 0xd0, 0xc0, 0x00, 0xbe, 0xbc, 0x00, 0x00, 0xbe, 0xbc, 0x17, 0x79, 0x71, 0xa1, 0x80, 0x01, 0x7d, 0x78, 0x00, 0x01, 0x7d, 0x78, 0x2e, 0xf2, 0xe1, 0xf0, 0x88, 0x45, 0x16, 0x00, 0x00, 0x00, 0xfe };
static unsigned char SPS_720p60_default[] = { 0x27, 0x4d, 0x00, 0x20, 0x9a, 0x66, 0x02, 0x80, 0x2d, 0xd8, 0x0b, 0x64, 0x00, 0x00, 0x0f, 0xa4, 0x00, 0x07, 0x53, 0x03, 0xa1, 0x80, 0x03, 0x8a, 0x60, 0x00, 0x0e, 0x29, 0x82, 0xef, 0x2e, 0x34, 0x30, 0x00, 0x71, 0x4c, 0x00, 0x01, 0xc5, 0x30, 0x5d, 0xe5, 0xc3, 0xe1, 0x10, 0x8a, 0x34, 0xfe };
static unsigned char SPS_720p60_advanced[] = { 0x27, 0x64, 0x00, 0x28, 0xac, 0x34, 0xc8, 0x05, 0x00, 0x5b, 0xb0, 0x16, 0xa0, 0x20, 0x20, 0x28, 0x00, 0x00, 0x1f, 0x48, 0x00, 0x0e, 0xa6, 0x07, 0x43, 0x00, 0x02, 0x62, 0x58, 0x00, 0x02, 0x62, 0x5a, 0x5d, 0xe5, 0xc6, 0x86, 0x00, 0x04, 0xc4, 0xb0, 0x00, 0x04, 0xc4, 0xb4, 0xbb, 0xcb, 0x87, 0xc2, 0x21, 0x14, 0x78, 0xfe };
static unsigned char SPS_720p50[] = { 0x27, 0x64, 0x00, 0x29, 0xac, 0x34, 0xc8, 0x05, 0x00, 0x5b, 0xb0, 0x16, 0xa0, 0x20, 0x20, 0x28, 0x00, 0x00, 0x1f, 0x40, 0x00, 0x0c, 0x35, 0x07, 0x43, 0x00, 0x07, 0xa1, 0x20, 0x00, 0x1e, 0x84, 0x85, 0xde, 0x5c, 0x68, 0x60, 0x00, 0xf4, 0x24, 0x00, 0x03, 0xd0, 0x90, 0xbb, 0xcb, 0x87, 0xc2, 0x21, 0x14, 0x78, 0xfe };
static unsigned char SPS_720p48[] = { 0x27, 0x64, 0x00, 0x28, 0xac, 0x34, 0xc8, 0x05, 0x00, 0x5b, 0xb0, 0x16, 0xa0, 0x20, 0x20, 0x28, 0x00, 0x00, 0x1f, 0x48, 0x00, 0x0b, 0xb8, 0x07, 0x43, 0x00, 0x07, 0xa1, 0x20, 0x00, 0x1e, 0x84, 0x85, 0xde, 0x5c, 0x68, 0x60, 0x00, 0xf4, 0x24, 0x00, 0x03, 0xd0, 0x90, 0xbb, 0xcb, 0x87, 0xc2, 0x21, 0x14, 0x78, 0x00, 0xfe };
static unsigned char SPS_720p30[] = { 0x27, 0x4d, 0x00, 0x1f, 0x9a, 0x66, 0x02, 0x80, 0x2d, 0xd8, 0x0b, 0x64, 0x00, 0x00, 0x0f, 0xa4, 0x00, 0x03, 0xa9, 0x83, 0xa1, 0x80, 0x02, 0x5c, 0x40, 0x00, 0x09, 0x71, 0x02, 0xef, 0x2e, 0x34, 0x30, 0x00, 0x4b, 0x88, 0x00, 0x01, 0x2e, 0x20, 0x5d, 0xe5, 0xc3, 0xe1, 0x10, 0x8a, 0x34, 0xfe };
static unsigned char SPS_720p25[] = { 0x27, 0x64, 0x00, 0x28, 0xac, 0x34, 0xc8, 0x05, 0x00, 0x5b, 0xb0, 0x16, 0xa0, 0x20, 0x20, 0x28, 0x00, 0x00, 0x1f, 0x40, 0x00, 0x06, 0x1a, 0x87, 0x43, 0x00, 0x0f, 0xd4, 0x80, 0x00, 0xfd, 0x4b, 0x5d, 0xe5, 0xc6, 0x86, 0x00, 0x1f, 0xa9, 0x00, 0x01, 0xfa, 0x96, 0xbb, 0xcb, 0x87, 0xc2, 0x21, 0x14, 0x78, 0xfe };
static unsigned char SPS_720p24[] = { 0x27, 0x64, 0x00, 0x29, 0xac, 0x34, 0xc8, 0x05, 0x00, 0x5b, 0xb0, 0x16, 0xa0, 0x20, 0x20, 0x28, 0x00, 0x00, 0x1f, 0x48, 0x00, 0x05, 0xdc, 0x07, 0x43, 0x00, 0x0f, 0xd4, 0x80, 0x00, 0xfd, 0x4b, 0x5d, 0xe5, 0xc6, 0x86, 0x00, 0x1f, 0xa9, 0x00, 0x01, 0xfa, 0x96, 0xbb, 0xcb, 0x87, 0xc2, 0x21, 0x14, 0x78, 0xfe };
static unsigned char SPS_480p30[] = { 0x27, 0x4d, 0x40, 0x1e, 0x9a, 0x66, 0x05, 0x01, 0xed, 0x80, 0xb6, 0x40, 0x00, 0x00, 0xfa, 0x40, 0x00, 0x3a, 0x98, 0x3a, 0x10, 0x00, 0x5e, 0x68, 0x00, 0x02, 0xf3, 0x40, 0xbb, 0xcb, 0x8d, 0x08, 0x00, 0x2f, 0x34, 0x00, 0x01, 0x79, 0xa0, 0x5d, 0xe5, 0xc3, 0xe1, 0x10, 0x8a, 0x3c, 0xfe };

static unsigned char PPS_P2VP[] =    { 0x28, 0xee, 0x3c, 0x80, 0xfe };
static unsigned char PPS_Inspire[] = { 0x28, 0xee, 0x38, 0x30, 0xfe };
static unsigned char PPS_For1080pNew[] = { 0x68, 0xee, 0x38, 0x80, 0xfe };

static void doRepairType2(FILE* inputFID, FILE* outputFID, unsigned second4Bytes) {
  /* Begin the repair by writing SPS and PPS NAL units (each preceded by a 'start code'): */
  {
    int formatCode;
    unsigned char* sps;
    unsigned char* pps;
    unsigned char c;

    /* The content of the SPS NAL unit depends upon which video format was used.
       Prompt the user for this now:
    */
    while (1) {
      fprintf(stderr, "First, however, we need to know which video format was used.  Enter this now.\n");
      fprintf(stderr, "\tIf the video format was 2160p, 30fps: Type 0, then the \"Return\" key.\n");
      fprintf(stderr, "\tIf the video format was 2160(x4096)p(4K), 25fps: Type 1, then the \"Return\" key.\n");
      fprintf(stderr, "\tIf the video format was 2160(x3840)p(UHD-1), 25fps: Type 2, then the \"Return\" key.\n");
      fprintf(stderr, "\tIf the video format was 2160(x4096)p(4K), 24fps: Type 3, then the \"Return\" key.\n");
      fprintf(stderr, "\tIf the video format was 2160(x3840)p(UHD-1), 24fps: Type 4, then the \"Return\" key.\n");
      fprintf(stderr, "\tIf the video format was 1530p, 30fps: Type 5, then the \"Return\" key.\n");
      fprintf(stderr, "\tIf the video format was 1530p, 25fps: Type 6, then the \"Return\" key.\n");
      fprintf(stderr, "\tIf the video format was 1530p, 24fps: Type 7, then the \"Return\" key.\n");
      fprintf(stderr, "\tIf the video format was 1520p, 60fps: Type 8, then the \"Return\" key.\n");
      fprintf(stderr, "\tIf the video format was 1520p, 30fps: Type 9, then the \"Return\" key.\n");
      fprintf(stderr, "\tIf the video format was 1520p, 25fps: Type A, then the \"Return\" key.\n");
      fprintf(stderr, "\tIf the video format was 1520p, 24fps: Type B, then the \"Return\" key.\n");
      fprintf(stderr, "\tIf the video format was 1080p, 60fps: Type C, then the \"Return\" key.\n");
      fprintf(stderr, "\tIf the video format was 1080i, 60fps: Type D, then the \"Return\" key.\n");
      fprintf(stderr, "\tIf the video format was 1080p, 50fps: Type E, then the \"Return\" key.\n");
      fprintf(stderr, "\tIf the video format was 1080p, 48fps: Type F, then the \"Return\" key.\n");
      fprintf(stderr, "\tIf the video format was 1080p, 30fps: Type G, then the \"Return\" key.\n");
      fprintf(stderr, "\tIf the video format was 1080p, 30fps (Zenmuse): Type H, then the \"Return\" key.\n");
      fprintf(stderr, "\tIf the video format was 1080p, 25fps: Type I, then the \"Return\" key.\n");
      fprintf(stderr, "\tIf the video format was 1080p, 24fps: Type J, then the \"Return\" key.\n");
      fprintf(stderr, "\tIf the video format was 720p, 60fps: Type K, then the \"Return\" key.\n");
      fprintf(stderr, "\tIf the video format was 720p, 60fps (Osmo+): Type L, then the \"Return\" key.\n");
      fprintf(stderr, "\tIf the video format was 720p, 50fps: Type M, then the \"Return\" key.\n");
      fprintf(stderr, "\tIf the video format was 720p, 48fps: Type N, then the \"Return\" key.\n");
      fprintf(stderr, "\tIf the video format was 720p, 30fps: Type O, then the \"Return\" key.\n");
      fprintf(stderr, "\tIf the video format was 720p, 25fps: Type P, then the \"Return\" key.\n");
      fprintf(stderr, "\tIf the video format was 720p, 24fps: Type Q, then the \"Return\" key.\n");
      fprintf(stderr, "\tIf the video format was 480p, 30fps: Type R, then the \"Return\" key.\n");
      fprintf(stderr, "(If you are unsure which video format was used, then guess as follows:\n");
      fprintf(stderr, "\tIf your file was from a Mavic Pro: Type 7, then the \"Return\" key.\n");
      fprintf(stderr, "\tIf your file was from a Phantom 2 Vision+: Type G, then the \"Return\" key.\n");
      fprintf(stderr, "\tIf your file was from an Inspire: Type 3, then the \"Return\" key.\n");
      fprintf(stderr, " If the resulting file is unplayable by VLC or IINA, then you may have guessed the wrong format;\n");
      fprintf(stderr, " try again with another format.)\n");
      fprintf(stderr, "If you know for sure that your video format was *not* one of the ones listed above, then please read FAQ number 4 at \"http://djifix.live555.com/#faq\", and we'll try to update the software to support your video format.\n");
      do {formatCode = getchar(); } while (formatCode == '\r' && formatCode == '\n');
      if ((formatCode >= '0' && formatCode <= '9') ||
	  (formatCode >= 'a' && formatCode <= 'r') ||
	  (formatCode >= 'A' && formatCode <= 'R')) {
	break;
      }
      fprintf(stderr, "Invalid entry!\n");
    }     

    fprintf(stderr, "%s", startingToRepair);
    switch (formatCode) {
      case '0': { sps = SPS_2160p30; pps = PPS_Inspire; break; }
      case '1': { sps = SPS_2160x4096p25; pps = PPS_Inspire; break; }
      case '2': { sps = SPS_2160x3840p25; pps = PPS_Inspire; break; }
      case '3': { sps = SPS_2160x4096p24; pps = PPS_Inspire; break; }
      case '4': { sps = SPS_2160x3840p24; pps = PPS_Inspire; break; }
      case '5': { sps = SPS_1530p30; pps = PPS_Inspire; break; }
      case '6': { sps = SPS_1530p25; pps = PPS_Inspire; break; }
      case '7': { sps = SPS_1530p24; pps = PPS_Inspire; break; }
      case '8': { sps = SPS_1520p60; pps = PPS_Inspire; break; }
      case '9': { sps = SPS_1520p30; pps = PPS_Inspire; break; }
      case 'a': case 'A': { sps = SPS_1520p25; pps = PPS_Inspire; break; }
      case 'b': case 'B': { sps = SPS_1520p24; pps = PPS_Inspire; break; }
      case 'c': case 'C': { sps = SPS_1080p60; pps = PPS_Inspire; break; }
      case 'd': case 'D': { sps = SPS_1080i60; pps = PPS_P2VP; break; }
      case 'e': case 'E': { sps = SPS_1080p50; pps = PPS_Inspire; break; }
      case 'f': case 'F': { sps = SPS_1080p48; pps = PPS_Inspire; break; }
      case 'g': case 'G': { sps = SPS_1080p30_default; pps = PPS_For1080pNew; break; }
      case 'h': case 'H': { sps = SPS_1080p30_advanced; pps = PPS_Inspire; break; }
      case 'i': case 'I': { sps = SPS_1080p25; pps = PPS_P2VP; break; }
      case 'j': case 'J': { sps = SPS_1080p24; pps = PPS_Inspire; break; }
      case 'k': case 'K': { sps = SPS_720p60_default; pps = PPS_P2VP; break; }
      case 'l': case 'L': { sps = SPS_720p60_advanced; pps = PPS_Inspire; break; }
      case 'm': case 'M': { sps = SPS_720p50; pps = PPS_Inspire; break; }
      case 'n': case 'N': { sps = SPS_720p48; pps = PPS_Inspire; break; }
      case 'o': case 'O': { sps = SPS_720p30; pps = PPS_P2VP; break; }
      case 'p': case 'P': { sps = SPS_720p25; pps = PPS_Inspire; break; }
      case 'q': case 'Q': { sps = SPS_720p24; pps = PPS_Inspire; break; }
      case 'r': case 'R': { sps = SPS_480p30; pps = PPS_P2VP; break; }
      default: { sps = SPS_1080p30_default; pps = PPS_P2VP; break; } /* shouldn't happen */
    };

    /*SPS*/
    putStartCode(outputFID);
    while ((c = *sps++) != 0xfe) wr(c);

    /*PPS*/
    putStartCode(outputFID);
    while ((c = *pps++) != 0xfe) wr(c);
  }

  /* Then write the first (2-byte) NAL unit, preceded by a 'start code': */
  putStartCode(outputFID);
  wr(second4Bytes>>24); wr(second4Bytes>>16);

  /* Then repeatedly:
     1/ Read a 4-byte NAL unit size.
     2/ Write a 'start code'.
     3/ Read 'NAL unit size' bytes, and write them to the output file.
  */
  {
    unsigned nalSize;
    unsigned char c1, c2;

    if (!get1Byte(inputFID, &c1)) return;
    if (!get1Byte(inputFID, &c2)) return;
    nalSize = ((second4Bytes&0xFFFF)<<16)|(c1<<8)|c2; /* for the first NAL unit */

    while (!feof(inputFID)) {
      putStartCode(outputFID);
      while (nalSize-- > 0) {
	wr(fgetc(inputFID));
      }

      if (!get4Bytes(inputFID, &nalSize)) return;
      if (nalSize == 0 || nalSize > 0x008FFFFF) {
	/* An anomalous situation (we got a NAL size that's 0, or much bigger than normal).
	   This suggests that the data here is not really video (or is corrupt in some other way).
	   Try to recover from this by repeatedly reading bytes until we get a 'nalSize'
	   of 0x00000002.  With luck, that will begin sane data once again.
	*/
	unsigned char c;
	unsigned long filePosition = ftell(inputFID)-4;

	fprintf(stderr, "\n(Skipping over anomalous bytes (nalSize 0x%08x), starting at file position 0x%08lx (%lu MBytes))...\n", nalSize, filePosition, filePosition/1000000);
	do {
	  if (!get1Byte(inputFID, &c)) return;
	  nalSize = (nalSize<<8)|c;
	} while (nalSize != 2);

	filePosition = ftell(inputFID)-4;
	fprintf(stderr, "...resuming at file position 0x%08lx (%lu MBytes)).  Continuing to repair the file (please wait)...", filePosition, filePosition/1000000);
      }
    }
  }
}


static unsigned char type3_H264_SPS_3000p30[] = { 0x27, 0x64, 0x00, 0x34, 0xad, 0x84, 0x61, 0x18, 0x46, 0x11, 0x84, 0x61, 0x18, 0x46, 0x11, 0x34, 0xc8, 0x03, 0xe8, 0x05, 0xe7, 0xe5, 0xc0, 0x5a, 0x80, 0x80, 0x80, 0xa0, 0x00, 0x00, 0x7d, 0x20, 0x00, 0x1d, 0x4c, 0x1d, 0x0c, 0x00, 0x02, 0xfa, 0xf0, 0x00, 0x00, 0x1c, 0x9c, 0x38, 0x5d, 0xe5, 0xc6, 0x86, 0x00, 0x01, 0x7d, 0x78, 0x00, 0x00, 0x0e, 0x4e, 0x1c, 0x2e, 0xf2, 0xe1, 0xf0, 0x88, 0x45, 0x12, 0xfe };
static unsigned char type3_H264_SPS_2160x4096p60[] = { 0x27, 0x64, 0x00, 0x34, 0xac, 0x34, 0xc8, 0x01, 0x00, 0x01, 0x0f, 0xb0, 0x16, 0xa0, 0x20, 0x20, 0x28, 0x00, 0x00, 0x1f, 0x48, 0x00, 0x0e, 0xa6, 0x07, 0x43, 0x00, 0x00, 0xbe, 0xbc, 0x00, 0x00, 0x0d, 0x69, 0x3a, 0x5d, 0xe5, 0xc6, 0x86, 0x00, 0x01, 0x7d, 0x78, 0x00, 0x00, 0x1a, 0xd2, 0x74, 0xbb, 0xcb, 0x87, 0xc2, 0x21, 0x14, 0x58, 0xfe };
static unsigned char type3_H264_SPS_2160x3840p60[] = { 0x27, 0x64, 0x00, 0x34, 0xac, 0x34, 0xc8, 0x03, 0xc0, 0x04, 0x3e, 0xc0, 0x5a, 0x80, 0x80, 0x80, 0xa0, 0x00, 0x00, 0x7d, 0x20, 0x00, 0x3a, 0x98, 0x1d, 0x0c, 0x00, 0x07, 0x27, 0x08, 0x00, 0x00, 0x80, 0xbe, 0xf5, 0xde, 0x5c, 0x68, 0x60, 0x00, 0x39, 0x38, 0x40, 0x00, 0x04, 0x05, 0xf7, 0xae, 0xf2, 0xe1, 0xf0, 0x88, 0x45, 0x16, 0xfe };
static unsigned char type3_H264_SPS_2160x4096p50[] = { 0x27, 0x64, 0x00, 0x34, 0xac, 0x34, 0xc8, 0x01, 0x00, 0x01, 0x0f, 0xb0, 0x16, 0xa0, 0x20, 0x20, 0x28, 0x00, 0x00, 0x1f, 0x40, 0x00, 0x0c, 0x35, 0x07, 0x43, 0x00, 0x00, 0xbe, 0xbc, 0x00, 0x00, 0x0d, 0x69, 0x3a, 0x5d, 0xe5, 0xc6, 0x86, 0x00, 0x01, 0x7d, 0x78, 0x00, 0x00, 0x1a, 0xd2, 0x74, 0xbb, 0xcb, 0x87, 0xc2, 0x21, 0x14, 0x58, 0xfe };
static unsigned char type3_H264_SPS_2160x3840p50[] = { 0x27, 0x64, 0x00, 0x34, 0xac, 0x34, 0xc8, 0x03, 0xc0, 0x04, 0x3e, 0xc0, 0x5a, 0x80, 0x80, 0x80, 0xa0, 0x00, 0x00, 0x7d, 0x00, 0x00, 0x30, 0xd4, 0x1d, 0x0c, 0x00, 0x02, 0xfa, 0xf0, 0x00, 0x00, 0x35, 0xa4, 0xe9, 0x77, 0x97, 0x1a, 0x18, 0x00, 0x05, 0xf5, 0xe0, 0x00, 0x00, 0x6b, 0x49, 0xd2, 0xef, 0x2e, 0x1f, 0x08, 0x84, 0x51, 0x60, 0xfe };
static unsigned char type3_H264_SPS_2160x4096p48[] = { 0x27, 0x64, 0x00, 0x34, 0xac, 0x34, 0xc8, 0x01, 0x00, 0x01, 0x0f, 0xb0, 0x16, 0xa0, 0x20, 0x20, 0x28, 0x00, 0x00, 0x1f, 0x48, 0x00, 0x0b, 0xb8, 0x07, 0x43, 0x00, 0x00, 0xbe, 0xbc, 0x00, 0x00, 0x0d, 0x69, 0x3a, 0x5d, 0xe5, 0xc6, 0x86, 0x00, 0x01, 0x7d, 0x78, 0x00, 0x00, 0x1a, 0xd2, 0x74, 0xbb, 0xcb, 0x87, 0xc2, 0x21, 0x14, 0x58, 0xfe };
static unsigned char type3_H264_SPS_2160x3840p48[] = { 0x27, 0x64, 0x00, 0x34, 0xac, 0x34, 0xc8, 0x03, 0xc0, 0x04, 0x3e, 0xc0, 0x5a, 0x80, 0x80, 0x80, 0xa0, 0x00, 0x00, 0x7d, 0x20, 0x00, 0x2e, 0xe0, 0x1d, 0x0c, 0x00, 0x02, 0xfa, 0xf0, 0x00, 0x00, 0x35, 0xa4, 0xe9, 0x77, 0x97, 0x1a, 0x18, 0x00, 0x05, 0xf5, 0xe0, 0x00, 0x00, 0x6b, 0x49, 0xd2, 0xef, 0x2e, 0x1f, 0x08, 0x84, 0x51, 0x60, 0xfe };
static unsigned char type3_H265_SPS_2160x4096p30[] = { 0x40, 0x01, 0x0c, 0x01, 0xff, 0xff, 0x21, 0x60, 0x00, 0x00, 0x03, 0x00, 0x00, 0x03, 0x00, 0x00, 0x03, 0x00, 0x00, 0x03, 0x00, 0x96, 0xac, 0x09, 0xfe };
static unsigned char type3_H264_SPS_2160x4096p30[] = { 0x27, 0x64, 0x00, 0x34, 0xac, 0x34, 0xc8, 0x01, 0x00, 0x01, 0x0f, 0xb0, 0x16, 0xa0, 0x20, 0x20, 0x28, 0x00, 0x00, 0x1f, 0x48, 0x00, 0x07, 0x53, 0x07, 0x43, 0x00, 0x00, 0xbe, 0xbc, 0x00, 0x00, 0x0d, 0x69, 0x3a, 0x5d, 0xe5, 0xc6, 0x86, 0x00, 0x01, 0x7d, 0x78, 0x00, 0x00, 0x1a, 0xd2, 0x74, 0xbb, 0xcb, 0x87, 0xc2, 0x21, 0x14, 0x58, 0xfe };
static unsigned char type3_H265_SPS_2160x3840p30[] = { 0x40, 0x01, 0x0c, 0x01, 0xff, 0xff, 0x21, 0x60, 0x00, 0x00, 0x03, 0x00, 0x00, 0x03, 0x00, 0x00, 0x03, 0x00, 0x00, 0x03, 0x00, 0x96, 0xac, 0x09, 0xfe };
static unsigned char type3_H264_SPS_2160x3840p30_DJIMini2[] = { 0x67, 0x64, 0x00, 0x33, 0xac, 0x34, 0xc8, 0x03, 0xc0, 0x04, 0x3e, 0xc0, 0x5a, 0x80, 0x80, 0x80, 0xa0, 0x00, 0x00, 0x7d, 0x20, 0x00, 0x1d, 0x4c, 0x1d, 0x0c, 0x00, 0x02, 0xfa, 0xf0, 0x00, 0x00, 0x2f, 0xaf, 0x09, 0x77, 0x97, 0x1a, 0x18, 0x00, 0x05, 0xf5, 0xe0, 0x00, 0x00, 0x5f, 0x5e, 0x12, 0xef, 0x2e, 0x1f, 0x08, 0x84, 0x51, 0x60, 0xfe };
static unsigned char type3_H264_SPS_2160x3840p30_other[] = { 0x27, 0x64, 0x00, 0x33, 0xac, 0x34, 0xc8, 0x03, 0xc0, 0x04, 0x3e, 0xc0, 0x5a, 0x80, 0x80, 0x80, 0xa0, 0x00, 0x00, 0x7d, 0x20, 0x00, 0x1d, 0x4c, 0x1d, 0x0c, 0x00, 0x02, 0xfa, 0xf0, 0x00, 0x00, 0x35, 0xa4, 0xe9, 0x77, 0x97, 0x1a, 0x18, 0x00, 0x05, 0xf5, 0xe0, 0x00, 0x00, 0x6b, 0x49, 0xd2, 0xef, 0x2e, 0x1f, 0x08, 0x84, 0x51, 0x60, 0xfe };
static unsigned char type3_H264_SPS_2160x4096p25[] = { 0x27, 0x64, 0x00, 0x33, 0xac, 0x34, 0xc8, 0x01, 0x00, 0x01, 0x0f, 0xb0, 0x16, 0xa0, 0x20, 0x20, 0x28, 0x00, 0x00, 0x1f, 0x40, 0x00, 0x06, 0x1a, 0x87, 0x43, 0x00, 0x00, 0xbe, 0xbc, 0x00, 0x00, 0x0d, 0x69, 0x3a, 0x5d, 0xe5, 0xc6, 0x86, 0x00, 0x01, 0x7d, 0x78, 0x00, 0x00, 0x1a, 0xd2, 0x74, 0xbb, 0xcb, 0x87, 0xc2, 0x21, 0x14, 0x58, 0xfe };
static unsigned char type3_H265_SPS_2160x3840p25[] = { 0x40, 0x01, 0x0c, 0x01, 0xff, 0xff, 0x21, 0x60, 0x00, 0x00, 0x03, 0x00, 0x00, 0x03, 0x00, 0x00, 0x03, 0x00, 0x00, 0x03, 0x00, 0x96, 0xac, 0x09, 0xfe };
static unsigned char type3_H264_SPS_2160x3840p25[] = { 0x27, 0x64, 0x00, 0x33, 0xac, 0x34, 0xc8, 0x03, 0xc0, 0x04, 0x3e, 0xc0, 0x5a, 0x80, 0x80, 0x80, 0xa0, 0x00, 0x00, 0x7d, 0x00, 0x00, 0x18, 0x6a, 0x1d, 0x0c, 0x00, 0x02, 0xfa, 0xf0, 0x00, 0x00, 0x35, 0xa4, 0xe9, 0x77, 0x97, 0x1a, 0x18, 0x00, 0x05, 0xf5, 0xe0, 0x00, 0x00, 0x6b, 0x49, 0xd2, 0xef, 0x2e, 0x1f, 0x08, 0x84, 0x51, 0x60, 0xfe };
static unsigned char type3_H264_SPS_2160x3840p24_DJIMini2[] = { 0x67, 0x64, 0x00, 0x33, 0xac, 0x34, 0xc8, 0x03, 0xc0, 0x04, 0x3e, 0xc0, 0x5a, 0x80, 0x80, 0x80, 0xa0, 0x00, 0x00, 0x7d, 0x20, 0x00, 0x17, 0x70, 0x1d, 0x0c, 0x00, 0x02, 0xfa, 0xf0, 0x00, 0x00, 0x2f, 0xaf, 0x09, 0x77, 0x97, 0x1a, 0x18, 0x00, 0x05, 0xf5, 0xe0, 0x00, 0x00, 0x5f, 0x5e, 0x12, 0xef, 0x2e, 0x1f, 0x08, 0x84, 0x51, 0x60, 0xfe };
static unsigned char type3_H264_SPS_2160x3840p24_other[] = { 0x27, 0x64, 0x00, 0x33, 0xac, 0x34, 0xc8, 0x03, 0xc0, 0x04, 0x3e, 0xc0, 0x5a, 0x80, 0x80, 0x80, 0xa0, 0x00, 0x00, 0x7d, 0x00, 0x00, 0x17, 0x70, 0x1d, 0x0c, 0x00, 0x02, 0xfa, 0xf0, 0x00, 0x00, 0x35, 0xa4, 0xe9, 0x77, 0x97, 0x1a, 0x18, 0x00, 0x05, 0xf5, 0xe0, 0x00, 0x00, 0x6b, 0x49, 0xd2, 0xef, 0x2e, 0x1f, 0x08, 0x84, 0x51, 0x60, 0xfe };
static unsigned char type3_H264_SPS_1530p60[] = { 0x67, 0x64, 0x00, 0x33, 0xac, 0x34, 0xc8, 0x02, 0xa8, 0x0c, 0x1f, 0x93, 0x01, 0x6a, 0x02, 0x02, 0x02, 0x80, 0x00, 0x01, 0xf4, 0x80, 0x00, 0xea, 0x60, 0x74, 0x30, 0x00, 0x09, 0x89, 0x68, 0x00, 0x00, 0x98, 0x96, 0x85, 0xde, 0x5c, 0x68, 0x60, 0x00, 0x13, 0x12, 0xd0, 0x00, 0x01, 0x31, 0x2d, 0x0b, 0xbc, 0xb8, 0x7c, 0x22, 0x11, 0x45, 0x80, 0xfe };
static unsigned char type3_H265_SPS_1530p50[] = { 0x40, 0x01, 0x0c, 0x01, 0xff, 0xff, 0x21, 0x60, 0x00, 0x00, 0x03, 0x00, 0x00, 0x03, 0x00, 0x00, 0x03, 0x00, 0x00, 0x03, 0x00, 0x96, 0xac, 0x09, 0xfe };
static unsigned char type3_H264_SPS_1530p48[] = { 0x67, 0x64, 0x00, 0x33, 0xac, 0x34, 0xc8, 0x02, 0xa8, 0x0c, 0x1f, 0x93, 0x01, 0x6a, 0x02, 0x02, 0x02, 0x80, 0x00, 0x01, 0xf4, 0x80, 0x00, 0xbb, 0x80, 0x74, 0x30, 0x00, 0x09, 0x89, 0x68, 0x00, 0x00, 0x98, 0x96, 0x85, 0xde, 0x5c, 0x68, 0x60, 0x00, 0x13, 0x12, 0xd0, 0x00, 0x01, 0x31, 0x2d, 0x0b, 0xbc, 0xb8, 0x7c, 0x22, 0x11, 0x45, 0x80, 0xfe };
// Old data, obsoleted by newer Mavic Mini format:
//static unsigned char type3_H264_SPS_1530p48[] = { 0x27, 0x64, 0x00, 0x33, 0xac, 0x34, 0xc8, 0x02, 0xa8, 0x0c, 0x1f, 0x93, 0x01, 0x6a, 0x02, 0x02, 0x02, 0x80, 0x00, 0x01, 0xf4, 0x80, 0x00, 0xbb, 0x80, 0x74, 0x30, 0x00, 0x0b, 0xeb, 0xc0, 0x00, 0x00, 0xd6, 0x93, 0xa5, 0xde, 0x5c, 0x68, 0x60, 0x00, 0x17, 0xd7, 0x80, 0x00, 0x01, 0xad, 0x27, 0x4b, 0xbc, 0xb8, 0x7c, 0x22, 0x11, 0x45, 0x80, 0xfe };
// Old data, obsoleted by newer Mavic Mini format:
//static unsigned char type3_H264_SPS_1530p30[] = { 0x27, 0x64, 0x00, 0x32, 0xac, 0x34, 0xc8, 0x02, 0xa8, 0x0c, 0x1b, 0x01, 0x6a, 0x02, 0x02, 0x02, 0x80, 0x00, 0x01, 0xf4, 0x80, 0x00, 0x75, 0x30, 0x74, 0x30, 0x00, 0x09, 0x89, 0x68, 0x00, 0x00, 0xab, 0xa9, 0x55, 0xde, 0x5c, 0x68, 0x60, 0x00, 0x13, 0x12, 0xd0, 0x00, 0x01, 0x57, 0x52, 0xab, 0xbc, 0xb8, 0x7c, 0x22, 0x11, 0x45, 0x80, 0xfe };
// Old version of the Mavic Mini format:
//static unsigned char type3_H264_SPS_1530p30[] = { 0x67, 0x64, 0x00, 0x32, 0xac, 0x34, 0xc8, 0x02, 0xa8, 0x0c, 0x1b, 0x01, 0x6a, 0x02, 0x02, 0x02, 0x80, 0x00, 0x01, 0xf4, 0x80, 0x00, 0x75, 0x30, 0x74, 0x30, 0x00, 0x13, 0x12, 0xc0, 0x00, 0x04, 0xc4, 0xb4, 0x5d, 0xe5, 0xc6, 0x86, 0x00, 0x02, 0x62, 0x58, 0x00, 0x00, 0x98, 0x96, 0x8b, 0xbc, 0xb8, 0x7c, 0x22, 0x11, 0x45, 0x80, 0xfe };
static unsigned char type3_H264_SPS_1530p30[] = { 0x67, 0x64, 0x00, 0x32, 0xac, 0x34, 0xc8, 0x02, 0xa8, 0x0c, 0x1f, 0x93, 0x01, 0x6a, 0x02, 0x02, 0x02, 0x80, 0x00, 0x01, 0xf4, 0x80, 0x00, 0x75, 0x30, 0x74, 0x30, 0x00, 0x13, 0x12, 0xc0, 0x00, 0x04, 0xc4, 0xb4, 0x5d, 0xe5, 0xc6, 0x86, 0x00, 0x02, 0x62, 0x58, 0x00, 0x00, 0x98, 0x96, 0x8b, 0xbc, 0xb8, 0x7c, 0x22, 0x11, 0x45, 0x80, 0xfe };
static unsigned char type3_H264_SPS_1530p25[] = { 0x67, 0x64, 0x00, 0x32, 0xac, 0x34, 0xc8, 0x02, 0xa8, 0x0c, 0x1b, 0x01, 0x6a, 0x02, 0x02, 0x02, 0x80, 0x00, 0x01, 0xf4, 0x00, 0x00, 0x61, 0xa8, 0x74, 0x30, 0x00, 0x13, 0x12, 0xc0, 0x00, 0x04, 0xc4, 0xb4, 0x5d, 0xe5, 0xc6, 0x86, 0x00, 0x02, 0x62, 0x58, 0x00, 0x00, 0x98, 0x96, 0x8b, 0xbc, 0xb8, 0x7c, 0x22, 0x11, 0x45, 0x80, 0xfe };
static unsigned char type3_H264_SPS_1530p24_MavicMini[] = { 0x67, 0x64, 0x00, 0x32, 0xac, 0x34, 0xc8, 0x02, 0xa8, 0x0c, 0x1f, 0x93, 0x01, 0x6a, 0x02, 0x02, 0x02, 0x80, 0x00, 0x01, 0xf4, 0x80, 0x00, 0x5d, 0xc0, 0x74, 0x30, 0x00, 0x13, 0x12, 0xc0, 0x00, 0x04, 0xc4, 0xb4, 0x5d, 0xe5, 0xc6, 0x86, 0x00, 0x02, 0x62, 0x58, 0x00, 0x00, 0x98, 0x96, 0x8b, 0xbc, 0xb8, 0x7c, 0x22, 0x11, 0x45, 0x80, 0xfe };
static unsigned char type3_H264_SPS_1530p24_other[] = { 0x27, 0x64, 0x00, 0x32, 0xac, 0x34, 0xc8, 0x02, 0xa8, 0x0c, 0x1b, 0x01, 0xaa, 0x02, 0x02, 0x02, 0xa0, 0x00, 0x01, 0xf4, 0xa0, 0x00, 0x5d, 0xc0, 0xa4, 0x30, 0x00, 0x09, 0xa9, 0x68, 0x00, 0x00, 0xab, 0xa9, 0x55, 0xde, 0xac, 0x68, 0x60, 0x00, 0xa3, 0x12, 0xd0, 0x00, 0xa1, 0x57, 0x52, 0xab, 0xac, 0xb8, 0x7c, 0x22, 0xa1, 0x45, 0x80, 0xfe };
static unsigned char type3_H265_SPS_1080p120[] = { 0x40, 0x01, 0x0c, 0x01, 0xff, 0xff, 0x21, 0x60, 0x00, 0x00, 0x03, 0x00, 0x00, 0x03, 0x00, 0x00, 0x03, 0x00, 0x00, 0x03, 0x00, 0x96, 0xac, 0x09, 0xfe };
static unsigned char type3_H264_SPS_1080p120[] = { 0x27, 0x64, 0x00, 0x33, 0xac, 0x34, 0xc8, 0x07, 0x80, 0x22, 0x7e, 0x5c, 0x05, 0xa8, 0x08, 0x08, 0x0a, 0x00, 0x00, 0x07, 0xd2, 0x00, 0x07, 0x53, 0x01, 0xd0, 0xc0, 0x00, 0x2f, 0xaf, 0x00, 0x00, 0x03, 0x03, 0x5a, 0x4e, 0x97, 0x79, 0x71, 0xa1, 0x80, 0x00, 0x5f, 0x5e, 0x00, 0x00, 0x06, 0xb4, 0x9d, 0x2e, 0xf2, 0xe1, 0xf0, 0x88, 0x45, 0x16, 0xfe };
static unsigned char type3_H265_SPS_1080p60[] = { 0x40, 0x01, 0x0c, 0x01, 0xff, 0xff, 0x21, 0x60, 0x00, 0x00, 0x03, 0x00, 0x00, 0x03, 0x00, 0x00, 0x03, 0x00, 0x00, 0x03, 0x00, 0x7b, 0xac, 0x09, 0xfe };
static unsigned char type3_H264_SPS_1080p60_MavicMini[] = { 0x67, 0x64, 0x00, 0x2a, 0xac, 0x34, 0xc8, 0x07, 0x80, 0x22, 0x7e, 0x5c, 0x05, 0xa8, 0x08, 0x08, 0x0a, 0x00, 0x00, 0x07, 0xd2, 0x00, 0x03, 0xa9, 0x81, 0xd0, 0xc0, 0x00, 0x4c, 0x4b, 0x00, 0x00, 0x13, 0x12, 0xd1, 0x77, 0x97, 0x1a, 0x18, 0x00, 0x09, 0x89, 0x60, 0x00, 0x02, 0x62, 0x5a, 0x2e, 0xf2, 0xe1, 0xf0, 0x88, 0x45, 0x16, 0xfe };
static unsigned char type3_H264_SPS_1080p60_other[] = { 0x27, 0x64, 0x00, 0x2a, 0xac, 0x34, 0xc8, 0x07, 0x80, 0x22, 0x7e, 0x5c, 0x05, 0xa8, 0x08, 0x08, 0x0a, 0x00, 0x00, 0x07, 0xd2, 0x00, 0x03, 0xa9, 0x81, 0xd0, 0xc0, 0x00, 0x26, 0x25, 0xa0, 0x00, 0x02, 0xae, 0xa5, 0x57, 0x79, 0x71, 0xa1, 0x80, 0x00, 0x4c, 0x4b, 0x40, 0x00, 0x05, 0x5d, 0x4a, 0xae, 0xf2, 0xe1, 0xf0, 0x88, 0x45, 0x16, 0xfe };
static unsigned char type3_H264_SPS_1080p50_MavicMini[] = { 0x67, 0x64, 0x00, 0x2a, 0xac, 0x34, 0xc8, 0x07, 0x80, 0x22, 0x7e, 0x5c, 0x05, 0xa8, 0x08, 0x08, 0x0a, 0x00, 0x00, 0x07, 0xd0, 0x00, 0x03, 0x0d, 0x41, 0xd0, 0xc0, 0x00, 0x4c, 0x4b, 0x00, 0x00, 0x13, 0x12, 0xd1, 0x77, 0x97, 0x1a, 0x18, 0x00, 0x09, 0x89, 0x60, 0x00, 0x02, 0x62, 0x5a, 0x2e, 0xf2, 0xe1, 0xf0, 0x88, 0x45, 0x16, 0xfe };
static unsigned char type3_H264_SPS_1080p48_DJIMini2[] = { 0x67, 0x64, 0x00, 0x2a, 0xac, 0x34, 0xc8, 0x07, 0x80, 0x22, 0x7e, 0x5c, 0x05, 0xa8, 0x08, 0x08, 0x0a, 0x00, 0x00, 0x07, 0xd2, 0x00, 0x02, 0xee, 0x01, 0xd0, 0xc0, 0x00, 0x4c, 0x4b, 0x00, 0x00, 0x13, 0x12, 0xd1, 0x77, 0x97, 0x1a, 0x18, 0x00, 0x09, 0x89, 0x60, 0x00, 0x02, 0x62, 0x5a, 0x2e, 0xf2, 0xe1, 0xf0, 0x88, 0x45, 0x16, 0xfe };
static unsigned char type3_H264_SPS_1080p30_MavicMini[] = { 0x67, 0x64, 0x00, 0x28, 0xac, 0x34, 0xc8, 0x07, 0x80, 0x22, 0x7e, 0x5c, 0x05, 0xa8, 0x08, 0x08, 0x0a, 0x00, 0x00, 0x07, 0xd2, 0x00, 0x01, 0xd4, 0xc1, 0xd0, 0xc0, 0x00, 0x42, 0xc1, 0x80, 0x00, 0x10, 0xb0, 0x75, 0x77, 0x97, 0x1a, 0x18, 0x00, 0x08, 0x58, 0x30, 0x00, 0x02, 0x16, 0x0e, 0xae, 0xf2, 0xe1, 0xf0, 0x88, 0x45, 0x16, 0xfe };
static unsigned char type3_H264_SPS_1080p30_other[] = { 0x27, 0x64, 0x00, 0x28, 0xac, 0x34, 0xc8, 0x07, 0x80, 0x22, 0x7e, 0x5c, 0x05, 0xa8, 0x08, 0x08, 0x0a, 0x00, 0x00, 0x07, 0xd2, 0x00, 0x01, 0xd4, 0xc1, 0xd0, 0xc0, 0x00, 0x72, 0x70, 0x80, 0x00, 0x08, 0x0b, 0xef, 0x5d, 0xe5, 0xc6, 0x86, 0x00, 0x03, 0x93, 0x84, 0x00, 0x00, 0x40, 0x5f, 0x7a, 0xef, 0x2e, 0x1f, 0x08, 0x84, 0x51, 0x60, 0xfe };
static unsigned char type3_H265_SPS_1080p25[] = { 0x40, 0x01, 0x0c, 0x01, 0xff, 0xff, 0x21, 0x60, 0x00, 0x00, 0x03, 0x00, 0x00, 0x03, 0x00, 0x00, 0x03, 0x00, 0x00, 0x03, 0x00, 0x7b, 0xac, 0x09, 0xfe };
static unsigned char type3_H264_SPS_1080p25_MavicMini[] = { 0x67, 0x64, 0x00, 0x28, 0xac, 0x34, 0xc8, 0x07, 0x80, 0x22, 0x7e, 0x5c, 0x05, 0xa8, 0x08, 0x08, 0x0a, 0x00, 0x00, 0x07, 0xd0, 0x00, 0x01, 0x86, 0xa1, 0xd0, 0xc0, 0x00, 0x42, 0xc1, 0x80, 0x00, 0x10, 0xb0, 0x75, 0x77, 0x97, 0x1a, 0x18, 0x00, 0x08, 0x58, 0x30, 0x00, 0x02, 0x16, 0x0e, 0xae, 0xf2, 0xe1, 0xf0, 0x88, 0x45, 0x16, 0xfe };
static unsigned char type3_H264_SPS_1080p25_other[] = { 0x27, 0x64, 0x00, 0x28, 0xac, 0x34, 0xc8, 0x07, 0x80, 0x22, 0x7e, 0x5c, 0x05, 0xa8, 0x08, 0x08, 0x0a, 0x00, 0x00, 0x07, 0xd0, 0x00, 0x01, 0x86, 0xa1, 0xd0, 0xc0, 0x00, 0x4c, 0x4b, 0x00, 0x00, 0x15, 0x75, 0x29, 0x77, 0x97, 0x1a, 0x18, 0x00, 0x09, 0x89, 0x60, 0x00, 0x02, 0xae, 0xa5, 0x2e, 0xf2, 0xe1, 0xf0, 0x88, 0x45, 0x16, 0xfe };
static unsigned char type3_H264_SPS_1080p24_MavicMini[] = { 0x67, 0x64, 0x00, 0x28, 0xac, 0x34, 0xc8, 0x07, 0x80, 0x22, 0x7e, 0x5c, 0x05, 0xa8, 0x08, 0x08, 0x0a, 0x00, 0x00, 0x07, 0xd2, 0x00, 0x01, 0x77, 0x01, 0xd0, 0xc0, 0x00, 0x42, 0xc1, 0x80, 0x00, 0x10, 0xb0, 0x75, 0x77, 0x97, 0x1a, 0x18, 0x00, 0x08, 0x58, 0x30, 0x00, 0x02, 0x16, 0x0e, 0xae, 0xf2, 0xe1, 0xf0, 0x88, 0x45, 0x16, 0xfe };
static unsigned char type3_H264_SPS_1080p24_other[] = { 0x27, 0x64, 0x00, 0x28, 0xac, 0x34, 0xc8, 0x07, 0x80, 0x22, 0x7e, 0x5c, 0x05, 0xa8, 0x08, 0x08, 0x0a, 0x00, 0x00, 0x07, 0xd2, 0x00, 0x01, 0x77, 0x01, 0xd0, 0xc0, 0x00, 0x72, 0x70, 0x80, 0x00, 0x08, 0x0b, 0xef, 0x5d, 0xe5, 0xc6, 0x86, 0x00, 0x03, 0x93, 0x84, 0x00, 0x00, 0x40, 0x5f, 0x7a, 0xef, 0x2e, 0x1f, 0x08, 0x84, 0x51, 0x60, 0xfe };
static unsigned char type3_H264_SPS_720p30[] = { 0x27, 0x64, 0x00, 0x28, 0xac, 0x34, 0xc8, 0x05, 0x00, 0x5b, 0xb0, 0x16, 0xa0, 0x20, 0x20, 0x28, 0x00, 0x00, 0x1f, 0x48, 0x00, 0x07, 0x53, 0x07, 0x43, 0x00, 0x03, 0x93, 0x80, 0x00, 0x01, 0x01, 0x7d, 0xd7, 0x79, 0x71, 0xa1, 0x80, 0x01, 0xc9, 0xc0, 0x00, 0x00, 0x80, 0xbe, 0xeb, 0xbc, 0xb8, 0x7c, 0x22, 0x11, 0x47, 0x80, 0xfe };
static unsigned char type3_H264_SPS_480p30[] = { 0x67, 0x64, 0x00, 0x32, 0xac, 0xb4, 0x05, 0xa1, 0xed, 0x2a, 0x40, 0x00, 0x00, 0xfa, 0x00, 0x00, 0x3a, 0x98, 0x18, 0x10, 0x00, 0x1e, 0x84, 0x80, 0x06, 0xdd, 0xef, 0x7b, 0xe1, 0x78, 0x44, 0x23, 0x50, 0xfe };

static unsigned char type3_H264_PPS_default[] = { 0x28, 0xee, 0x38, 0xb0, 0xfe };
static unsigned char type3_H264_PPS_MavicMini[] = { 0x68, 0xee, 0x38, 0x30, 0xfe };
static unsigned char type3_H264_PPS_3000p30[] = { 0x28, 0xee, 0x38, 0xe1, 0x18, 0x46, 0x11, 0x84, 0x61, 0x18, 0x46, 0x11, 0x84, 0x70, 0xfe };
static unsigned char type3_H265_PPS_2160x4096p30[] = { 0x42, 0x01, 0x01, 0x21, 0x60, 0x00, 0x00, 0x03, 0x00, 0x00, 0x03, 0x00, 0x00, 0x03, 0x00, 0x00, 0x03, 0x00, 0x96, 0xa0, 0x00, 0x80, 0x08, 0x00, 0x87, 0x1f, 0xe5, 0xae, 0xed, 0x4d, 0xdd, 0xc9, 0x75, 0x80, 0xb5, 0x01, 0x01, 0x01, 0x04, 0x00, 0x00, 0x0f, 0xa0, 0x00, 0x01, 0x86, 0xa0, 0xae, 0x11, 0x08, 0x20, 0xfe };
static unsigned char type3_H265_PPS_2160x3840p30[] = { 0x42, 0x01, 0x01, 0x21, 0x60, 0x00, 0x00, 0x03, 0x00, 0x00, 0x03, 0x00, 0x00, 0x03, 0x00, 0x00, 0x03, 0x00, 0x96, 0xa0, 0x01, 0xe0, 0x20, 0x02, 0x1c, 0x7f, 0x96, 0xbb, 0xb5, 0x37, 0x77, 0x25, 0xd6, 0x02, 0xd4, 0x04, 0x04, 0x04, 0x10, 0x00, 0x00, 0x3e, 0x90, 0x00, 0x07, 0x53, 0x02, 0xb8, 0x44, 0x20, 0x80, 0xfe };
static unsigned char type3_H265_PPS_2160x3840p25[] = { 0x42, 0x01, 0x01, 0x21, 0x60, 0x00, 0x00, 0x03, 0x00, 0x00, 0x03, 0x00, 0x00, 0x03, 0x00, 0x00, 0x03, 0x00, 0x96, 0xa0, 0x01, 0xe0, 0x20, 0x02, 0x1c, 0x7f, 0x96, 0xbb, 0xb5, 0x37, 0x77, 0x25, 0xd6, 0x02, 0xd4, 0x04, 0x04, 0x04, 0x10, 0x00, 0x00, 0x3e, 0x80, 0x00, 0x06, 0x1a, 0x82, 0xb8, 0x44, 0x20, 0x80, 0xfe };
static unsigned char type3_H265_PPS_1530p50[] = { 0x42, 0x01, 0x01, 0x21, 0x60, 0x00, 0x00, 0x03, 0x00, 0x00, 0x03, 0x00, 0x00, 0x03, 0x00, 0x00, 0x03, 0x00, 0x96, 0xa0, 0x01, 0x54, 0x20, 0x06, 0x01, 0xf2, 0x65, 0xae, 0xed, 0x4d, 0xdd, 0xc9, 0x75, 0x80, 0xb5, 0x01, 0x01, 0x01, 0x04, 0x00, 0x00, 0x0f, 0xa4, 0x00, 0x03, 0x0d, 0x40, 0xae, 0x11, 0x08, 0x20, 0xfe };
static unsigned char type3_H265_PPS_1080p120[] = { 0x42, 0x01, 0x01, 0x21, 0x60, 0x00, 0x00, 0x03, 0x00, 0x00, 0x03, 0x00, 0x00, 0x03, 0x00, 0x00, 0x03, 0x00, 0x96, 0xa0, 0x03, 0xc0, 0x80, 0x10, 0xe7, 0xf9, 0x6b, 0xbb, 0x53, 0x77, 0x72, 0x5d, 0x60, 0x2d, 0x40, 0x40, 0x40, 0x41, 0x00, 0x00, 0x03, 0x03, 0xe9, 0x00, 0x01, 0xd4, 0xc0, 0x2b, 0x84, 0x42, 0x08, 0xfe };
static unsigned char type3_H265_PPS_1080p60[] = { 0x42, 0x01, 0x01, 0x21, 0x60, 0x00, 0x00, 0x03, 0x00, 0x00, 0x03, 0x00, 0x00, 0x03, 0x00, 0x00, 0x03, 0x00, 0x7b, 0xa0, 0x03, 0xc0, 0x80, 0x10, 0xe7, 0xf9, 0x6b, 0xbb, 0x53, 0x77, 0x72, 0x5d, 0x60, 0x2d, 0x40, 0x40, 0x40, 0x41, 0x00, 0x00, 0x03, 0x03, 0xe9, 0x00, 0x00, 0xea, 0x60, 0x2b, 0x84, 0x42, 0x08, 0xfe };
static unsigned char type3_H265_PPS_1080p25[] = { 0x42, 0x01, 0x01, 0x21, 0x60, 0x00, 0x00, 0x03, 0x00, 0x00, 0x03, 0x00, 0x00, 0x03, 0x00, 0x00, 0x03, 0x00, 0x7b, 0xa0, 0x03, 0xc0, 0x80, 0x10, 0xa7, 0xf9, 0x6b, 0xbb, 0x53, 0x77, 0x72, 0x5d, 0x60, 0x2d, 0x40, 0x40, 0x40, 0x41, 0x00, 0x00, 0x03, 0x03, 0xe8, 0x00, 0x00, 0x61, 0xa8, 0x2b, 0x84, 0x42, 0x08, 0xfe };
static unsigned char type3_H264_PPS_480p[] = { 0x68, 0xee, 0x3c, 0xb0, 0xfe };

static unsigned char type3_H265_VPS_2160x4096p30[] = { 0x44, 0x01, 0xc1, 0x72, 0xb0, 0x9c, 0x0a, 0xc1, 0x5e, 0x24, 0xfe };
static unsigned char type3_H265_VPS_2160x3840[] = { 0x44, 0x01, 0xc1, 0x72, 0xb0, 0x9c, 0x0a, 0x01, 0x46, 0x24, 0xfe };
static unsigned char type3_H265_VPS_1530p[] = { 0x44, 0x01, 0xc1, 0x72, 0xb0, 0x9c, 0x1d, 0x0e, 0xe2, 0x40, 0xfe };
static unsigned char type3_H265_VPS_1080p[] = { 0x44, 0x01, 0xc1, 0x72, 0xb0, 0x9c, 0x14, 0x0a, 0x62, 0x40, 0xfe };


static unsigned printableMetadataCount = 0;

static void doRepairType3(FILE* inputFID, FILE* outputFID) {
  /* Begin the repair by writing SPS, PPS, and (for H.265) VPS NAL units
     (each preceded by a 'start code'):
  */
  {
    int formatCode;
    unsigned char* sps;
    unsigned char* pps;
    unsigned char* vps = NULL; /* by default, for H.264 */
    unsigned char c;

    /* The content of the SPS, PPS, and VPS NAL units depends upon which video format was used.
       Prompt the user for this now:
    */
    while (1) {
      fprintf(stderr, "First, however, we need to know which video format was used.  Enter this now.\n");
      fprintf(stderr, "\tIf the video format was H.264, 2160(x4096)p(4K), 60fps: Type 0, then the \"Return\" key.\n");
      fprintf(stderr, "\tIf the video format was H.264, 2160(x3840)p(UHD-1), 60fps: Type 1, then the \"Return\" key.\n");
      fprintf(stderr, "\tIf the video format was H.264, 2160(x4096)p(4K), 50fps: Type 2, then the \"Return\" key.\n");
      fprintf(stderr, "\tIf the video format was H.264, 2160(x3840)p(UHD-1), 50fps: Type 3, then the \"Return\" key.\n");
      fprintf(stderr, "\tIf the video format was H.264, 2160(x4096)p(4K), 48fps: Type 4, then the \"Return\" key.\n");
      fprintf(stderr, "\tIf the video format was H.264, 2160(x3840)p(UHD-1), 48fps: Type 5, then the \"Return\" key.\n");
      fprintf(stderr, "\tIf the video format was H.265, 2160(x4096)p(4K), 30fps: Type 6, then the \"Return\" key.\n");
      fprintf(stderr, "\tIf the video format was H.264, 2160(x4096)p(4K), 30fps: Type 7, then the \"Return\" key.\n");
      fprintf(stderr, "\tIf the video format was H.265, 2160(x3840)p(UHD-1), 30fps: Type 8, then the \"Return\" key.\n");
      fprintf(stderr, "\tIf the video format was H.264, 2160(x3840)p(UHD-1), 30fps (DJI Mini 2): Type 9, then the \"Return\" key.\n");
      fprintf(stderr, "\tIf the video format was H.264, 2160(x3840)p(UHD-1), 30fps (other DJI drones): Type a, then the \"Return\" key.\n");
      fprintf(stderr, "\tIf the video format was H.264, 2160(x4096)p(4K), 25fps: Type b, then the \"Return\" key.\n");
      fprintf(stderr, "\tIf the video format was H.265, 2160(x3840)p(UHD-1), 25fps: Type c, then the \"Return\" key.\n");
      fprintf(stderr, "\tIf the video format was H.264, 2160(x3840)p(UHD-1), 25fps: Type d, then the \"Return\" key.\n");
      fprintf(stderr, "\tIf the video format was H.264, 2160(x3840)p(UHD-1), 24fps (DJI Mini 2): Type e, then the \"Return\" key.\n");
      fprintf(stderr, "\tIf the video format was H.264, 2160(x3840)p(UHD-1), 24fps (other DJI drones): Type f, then the \"Return\" key.\n");
      fprintf(stderr, "\tIf the video format was H.264, 1530p, 60fps: Type g, then the \"Return\" key.\n");
      fprintf(stderr, "\tIf the video format was H.265, 1530p, 50fps: Type h, then the \"Return\" key.\n");
      fprintf(stderr, "\tIf the video format was H.264, 1530p, 48fps: Type i, then the \"Return\" key.\n");
      fprintf(stderr, "\tIf the video format was H.264, 1530p, 30fps: Type j, then the \"Return\" key.\n");
      fprintf(stderr, "\tIf the video format was H.264, 1530p, 25fps: Type k, then the \"Return\" key.\n");
      fprintf(stderr, "\tIf the video format was H.264, 1530p, 24fps (Mavic Mini): Type l, then the \"Return\" key.\n");
      fprintf(stderr, "\tIf the video format was H.264, 1530p, 24fps (other DJI drones): Type m, then the \"Return\" key.\n");
      //      fprintf(stderr, "\tIf the video format was H.265, 1080p, 120fps: Type m, then the \"Return\" key.\n");
      //      fprintf(stderr, "\tIf the video format was H.264, 1080p, 120fps: Type n, then the \"Return\" key.\n");
      fprintf(stderr, "\tIf the video format was H.265, 1080p, 60fps: Type n, then the \"Return\" key.\n");
      fprintf(stderr, "\tIf the video format was H.264, 1080p, 60fps (Mavic Mini): Type o, then the \"Return\" key.\n");
      fprintf(stderr, "\tIf the video format was H.264, 1080p, 60fps (other DJI drones): Type p, then the \"Return\" key.\n");
      fprintf(stderr, "\tIf the video format was H.264, 1080p, 50fps: Type q, then the \"Return\" key.\n");
      fprintf(stderr, "\tIf the video format was H.264, 1080p, 48fps: Type r, then the \"Return\" key.\n");
      fprintf(stderr, "\tIf the video format was H.264, 1080p, 30fps (Mavic Mini): Type s, then the \"Return\" key.\n");
      fprintf(stderr, "\tIf the video format was H.264, 1080p, 30fps (other DJI drones): Type t, then the \"Return\" key.\n");
      fprintf(stderr, "\tIf the video format was H.265, 1080p, 25fps: Type u, then the \"Return\" key.\n");
      fprintf(stderr, "\tIf the video format was H.264, 1080p, 25fps (Mavic Mini): Type v, then the \"Return\" key.\n");
      fprintf(stderr, "\tIf the video format was H.264, 1080p, 25fps (other DJI drones): Type w, then the \"Return\" key.\n");
      fprintf(stderr, "\tIf the video format was H.264, 1080p, 24fps (Mavic Mini): Type x, then the \"Return\" key.\n");
      fprintf(stderr, "\tIf the video format was H.264, 1080p, 24fps (other DJI drones): Type y, then the \"Return\" key.\n");
      //      fprintf(stderr, "\tIf the video format was H.264, 720p, 30fps: Type y, then the \"Return\" key.\n");
      fprintf(stderr, "\tIf the video format was H.264, 480p, 30fps (e.g., from a XL FLIR camera): Type z, then the \"Return\" key.\n");
      fprintf(stderr, " If the resulting file is unplayable by VLC or IINA, then you may have guessed the wrong format;\n");
      fprintf(stderr, " try again with another format.)\n");
      fprintf(stderr, "If you know for sure that your video format was *not* one of the ones listed above, then please read FAQ number 4 at \"http://djifix.live555.com/#faq\", and we'll try to update the software to support your video format.\n");
      do {formatCode = getchar(); } while (formatCode == '\r' && formatCode == '\n');
      if ((formatCode >= '0' && formatCode <= '9') ||
	  (formatCode >= 'a' && formatCode <= 'z') ||
	  (formatCode >= 'A' && formatCode <= 'Z')) {
	break;
      }
      fprintf(stderr, "Invalid entry!\n");
    }     

    fprintf(stderr, "%s", startingToRepair);
    switch (formatCode) {
      case '0': { sps = type3_H264_SPS_2160x4096p60; pps = type3_H264_PPS_default; break; }
      case '1': { sps = type3_H264_SPS_2160x3840p60; pps = type3_H264_PPS_default; break; }
      case '2': { sps = type3_H264_SPS_2160x4096p50; pps = type3_H264_PPS_default; break; }
      case '3': { sps = type3_H264_SPS_2160x3840p50; pps = type3_H264_PPS_default; break; }
      case '4': { sps = type3_H264_SPS_2160x4096p48; pps = type3_H264_PPS_default; break; }
      case '5': { sps = type3_H264_SPS_2160x3840p48; pps = type3_H264_PPS_default; break; }
      case '6': { sps = type3_H265_SPS_2160x4096p30; pps = type3_H265_PPS_2160x4096p30; vps = type3_H265_VPS_2160x4096p30; break; }
      case '7': { sps = type3_H264_SPS_2160x4096p30; pps = type3_H264_PPS_default; break; }
      case '8': { sps = type3_H265_SPS_2160x3840p30; pps = type3_H265_PPS_2160x3840p30; vps = type3_H265_VPS_2160x3840; break; }
      case '9': { sps = type3_H264_SPS_2160x3840p30_DJIMini2; pps = type3_H264_PPS_MavicMini; break; }
      case 'a': case 'A': { sps = type3_H264_SPS_2160x3840p30_other; pps = type3_H264_PPS_default; break; }
      case 'b': case 'B': { sps = type3_H264_SPS_2160x4096p25; pps = type3_H264_PPS_default; break; }
      case 'c': case 'C': { sps = type3_H265_SPS_2160x3840p25; pps = type3_H265_PPS_2160x3840p25; vps = type3_H265_VPS_2160x3840; break; }
      case 'd': case 'D': { sps = type3_H264_SPS_2160x3840p25; pps = type3_H264_PPS_default; break; }
      case 'e': case 'E': { sps = type3_H264_SPS_2160x3840p24_DJIMini2; pps = type3_H264_PPS_MavicMini; break; }
      case 'f': case 'F': { sps = type3_H264_SPS_2160x3840p24_other; pps = type3_H264_PPS_default; break; }
      case 'g': case 'G': { sps = type3_H264_SPS_1530p60; pps = type3_H264_PPS_MavicMini; break; }
      case 'h': case 'H': { sps = type3_H265_SPS_1530p50; pps = type3_H265_PPS_1530p50; vps = type3_H265_VPS_1530p; break; }
      case 'i': case 'I': { sps = type3_H264_SPS_1530p48; pps = type3_H264_PPS_MavicMini; break; }
      case 'j': case 'J': { sps = type3_H264_SPS_1530p30; pps = type3_H264_PPS_MavicMini; break; }
      case 'k': case 'K': { sps = type3_H264_SPS_1530p25; pps = type3_H264_PPS_MavicMini; break; }
      case 'l': case 'L': { sps = type3_H264_SPS_1530p24_MavicMini; pps = type3_H264_PPS_MavicMini; break; }
      case 'm': case 'M': { sps = type3_H264_SPS_1530p24_other; pps = type3_H264_PPS_default; break; }
	//      case 'n': case 'N': { sps = type3_H265_SPS_1080p120; pps = type3_H265_PPS_1080p120; vps = type3_H265_VPS_1080p; break; }
	//      case 'o': case 'O': { sps = type3_H264_SPS_1080p120; pps = type3_H264_PPS_default; break; }
      case 'n': case 'N': { sps = type3_H265_SPS_1080p60; pps = type3_H265_PPS_1080p60; vps = type3_H265_VPS_1080p; break; }
      case 'o': case 'O': { sps = type3_H264_SPS_1080p60_MavicMini; pps = type3_H264_PPS_MavicMini; break; }
      case 'p': case 'P': { sps = type3_H264_SPS_1080p60_other; pps = type3_H264_PPS_default; break; }
      case 'q': case 'Q': { sps = type3_H264_SPS_1080p50_MavicMini; pps = type3_H264_PPS_MavicMini; break; }
      case 'r': case 'R': { sps = type3_H264_SPS_1080p48_DJIMini2; pps = type3_H264_PPS_MavicMini; break; }
      case 's': case 'S': { sps = type3_H264_SPS_1080p30_MavicMini; pps = type3_H264_PPS_MavicMini; break; }
      case 't': case 'T': { sps = type3_H264_SPS_1080p30_other; pps = type3_H264_PPS_default; break; }
      case 'u': case 'U': { sps = type3_H265_SPS_1080p25; pps = type3_H265_PPS_1080p25; vps = type3_H265_VPS_1080p; break; }
      case 'v': case 'V': { sps = type3_H264_SPS_1080p25_MavicMini; pps = type3_H264_PPS_MavicMini; break; }
      case 'w': case 'W': { sps = type3_H264_SPS_1080p25_other; pps = type3_H264_PPS_default; break; }
      case 'x': case 'X': { sps = type3_H264_SPS_1080p24_MavicMini; pps = type3_H264_PPS_MavicMini; break; }
      case 'y': case 'Y': { sps = type3_H264_SPS_1080p24_other; pps = type3_H264_PPS_default; break; }
      case 'z': case 'Z': { sps = type3_H264_SPS_480p30; pps = type3_H264_PPS_480p; break; }
//      case 'y': case 'Y': { sps = type3_H264_SPS_720p30; pps = type3_H264_PPS_default; break; }
      default: { sps = type3_H264_SPS_2160x3840p30_other; pps = type3_H264_PPS_default; break; } /* shouldn't happen */
    };

    /*SPS*/
    putStartCode(outputFID);
    while ((c = *sps++) != 0xfe) wr(c);

    /*PPS*/
    putStartCode(outputFID);
    while ((c = *pps++) != 0xfe) wr(c);

    /*VPS*/
    if (vps != NULL) {
      putStartCode(outputFID);
      while ((c = *vps++) != 0xfe) wr(c);
    }
  }

  doRepairType3or5Common(inputFID, outputFID);
}

static void doRepairType4(FILE* inputFID, FILE* outputFID) {
  /* A special type of repair, when we already know that the file begins with a SPS (etc.).
     Repeatedly:
     1/ Read a 4-byte NAL unit size.
     2/ Write a 'start code'.
     3/ Read 'NAL unit size' bytes, and write them to the output file.
  */
  unsigned nalSize;

  fprintf(stderr, "%s", startingToRepair);
  while (!feof(inputFID)) {
    if (!get4Bytes(inputFID, &nalSize)) return;
    if (nalSize == 0 || nalSize > 0x008FFFFF) {
      /* An anomalous situation (we got a NAL size that's 0, or much bigger than normal).
	 This suggests that the data here is not really video (or is corrupt in some other way).
	 Try to recover from this by repeatedly reading bytes until we see what we think is
	 video.  With luck, that will begin sane data once again.
	*/
      unsigned next4Bytes;
      unsigned long filePosition = ftell(inputFID)-4;

      fprintf(stderr, "\n(Skipping over anomalous bytes (nalSize 0x%08x), starting at file position 0x%08lx (%lu MBytes))...\n", nalSize, filePosition, filePosition/1000000);
      if (!get4Bytes(inputFID, &next4Bytes)) return; /*eof*/
      while (!checkForVideoType4(nalSize, next4Bytes)) {
	unsigned char c;

	if (!get1Byte(inputFID, &c)) return;/*eof*/
	nalSize = ((nalSize<<8)&0xFFFFFF00) | ((next4Bytes>>24)&0x000000FF);
	next4Bytes = ((next4Bytes<<8)&0xFFFFFF00) | c;
      }
      fseek(inputFID, -4, SEEK_CUR);
      filePosition = ftell(inputFID)-4;
      fprintf(stderr, "...resuming at file position 0x%08lx (%lu MBytes)).  Continuing to repair the file (please wait)...", filePosition, filePosition/1000000);
    }
#ifdef CODE_COUNT
    else {
      unsigned next4Bytes;
      if (!get4Bytes(inputFID, &next4Bytes)) return;
      fseek(inputFID, -4, SEEK_CUR);
      ++codeCount[next4Bytes>>16];
      //fprintf(stderr, "#####@@@@@ nalSize 0x%08x, next4Bytes 0x%08x\n", nalSize, next4Bytes);
    }
#endif

    putStartCode(outputFID);
    while (nalSize-- > 0) {
      wr(fgetc(inputFID));
    }
  }
}

#define type5_H264_SPS_2160x3840p30_DJIMini2 type3_H264_SPS_2160x3840p30_DJIMini2 /* same */
static unsigned char type5_H264_SPS_2160x3840p25[] = { 0x67, 0x64, 0x00, 0x33, 0xac, 0x4d, 0x00, 0x78, 0x00, 0x87, 0xd0, 0x80, 0x00, 0x01, 0xf4, 0x00, 0x00, 0x61, 0xa8, 0x47, 0x8a, 0x15, 0x50, 0xfe };
#define type5_H264_SPS_2160x3840p24_DJIMini2 type3_H264_SPS_2160x3840p24_DJIMini2 /* same */
#define type5_H264_SPS_1080p48_DJIMini2 type3_H264_SPS_1080p48_DJIMini2 /* same */
static unsigned char type5_H264_SPS_1080p30_MavicAir[] = { 0x67, 0x64, 0x00, 0x29, 0xac, 0x4d, 0x00, 0xf0, 0x04, 0x4f, 0xca, 0x80, 0xfe };
static unsigned char type5_H264_SPS_1080p25_MavicAir[] = { 0x67, 0x64, 0x00, 0x32, 0xac, 0x4d, 0x00, 0xf0, 0x04, 0x4f, 0xca, 0x80, 0xfe };
static unsigned char type5_H264_SPS_720p24[] = { 0x67, 0x42, 0x80, 0x1f, 0xda, 0x01, 0x40, 0x16, 0xe9, 0x48, 0x28, 0x30, 0x30, 0x36, 0x85, 0x09, 0xa8, 0xfe };

#define type5_H264_PPS_DJIMini2 type3_H264_PPS_MavicMini /* same */
static unsigned char type5_H264_PPS_MavicAir[] = { 0x68, 0xea, 0x8f, 0x2c, 0xfe };
static unsigned char type5_H264_PPS_720p24[] = { 0x68, 0xce, 0x06, 0xf2, 0xfe };

static void doRepairType5(FILE* inputFID, FILE* outputFID) {
  /* This is identical to 'type 3', except that the possible video formats are assumed
     to be those for "DJI Mini 2" drones only.
  */
  /* Begin the repair by writing SPS, PPS, and (for H.265) VPS NAL units
     (each preceded by a 'start code'):
  */
  {
    int formatCode;
    unsigned char* sps;
    unsigned char* pps;
    unsigned char* vps = NULL; /* by default, for H.264 */
    unsigned char c;

    /* The content of the SPS, PPS, and VPS NAL units depends upon which video format was used.
       Prompt the user for this now:
    */
    while (1) {
      fprintf(stderr, "First, however, we need to know which video format was used.  Enter this now.\n");
      fprintf(stderr, "\tIf the video format was H.264, 2160(x3840)p(UHD-1), 30fps: Type 0, then the \"Return\" key.\n");
      fprintf(stderr, "\tIf the video format was H.264, 2160(x3840)p(UHD-1), 25fps: Type 1, then the \"Return\" key.\n");
      fprintf(stderr, "\tIf the video format was H.264, 2160(x3840)p(UHD-1), 24fps: Type 2, then the \"Return\" key.\n");
      fprintf(stderr, "\tIf the video format was H.264, 1080p, 48fps: Type 3, then the \"Return\" key.\n");
      fprintf(stderr, "\tIf the video format was H.264, 1080p, 30fps: Type 4, then the \"Return\" key.\n");
      fprintf(stderr, "\tIf the video format was H.264, 1080p, 25fps: Type 5, then the \"Return\" key.\n");
      fprintf(stderr, "\tIf the video format was H.264, 720p, 24fps: Type 6, then the \"Return\" key.\n");
      fprintf(stderr, " If the resulting file is unplayable by VLC or IINA, then you may have guessed the wrong format;\n");
      fprintf(stderr, " try again with another format.)\n");
      fprintf(stderr, "If you know for sure that your video format was *not* one of the ones listed above, then please read FAQ number 4 at \"http://djifix.live555.com/#faq\", and we'll try to update the software to support your video format.\n");
      do {formatCode = getchar(); } while (formatCode == '\r' && formatCode == '\n');
      if ((formatCode >= '0' && formatCode <= '6')) {
	break;
      }
      fprintf(stderr, "Invalid entry!\n");
    }     

    fprintf(stderr, "%s", startingToRepair);
    switch (formatCode) {
      case '0': { sps = type5_H264_SPS_2160x3840p30_DJIMini2; pps = type5_H264_PPS_DJIMini2; break; }
      case '1': { sps = type5_H264_SPS_2160x3840p25; pps = type5_H264_PPS_MavicAir; break; }
      case '2': { sps = type5_H264_SPS_2160x3840p24_DJIMini2; pps = type5_H264_PPS_DJIMini2; break; }
      case '3': { sps = type5_H264_SPS_1080p48_DJIMini2; pps = type5_H264_PPS_DJIMini2; break; }
      case '4': { sps = type5_H264_SPS_1080p30_MavicAir; pps = type5_H264_PPS_MavicAir; break; }
      case '5': { sps = type5_H264_SPS_1080p25_MavicAir; pps = type5_H264_PPS_MavicAir; break; }
      case '6': { sps = type5_H264_SPS_720p24; pps = type5_H264_PPS_720p24; break; }
      default: { sps = type5_H264_SPS_2160x3840p30_DJIMini2; pps = type5_H264_PPS_DJIMini2; break; } /* shouldn't happen */
    };

    /*SPS*/
    putStartCode(outputFID);
    while ((c = *sps++) != 0xfe) wr(c);

    /*PPS*/
    putStartCode(outputFID);
    while ((c = *pps++) != 0xfe) wr(c);

    /*VPS*/
    if (vps != NULL) {
      putStartCode(outputFID);
      while ((c = *vps++) != 0xfe) wr(c);
    }
  }

  doRepairType3or5Common(inputFID, outputFID);
}

static int metadataIsPrintable = 1;

static void doRepairType3or5Common(FILE* inputFID, FILE* outputFID) {
  /* Repeatedly:
     1/ Read a 4-byte NAL unit size.
     2/ Write a 'start code'.
     3/ Read 'NAL unit size' bytes, and write them to the output file.
  */
  {
    unsigned nalSize, next4Bytes;

    while (!feof(inputFID)) {
      if (!get4Bytes(inputFID, &nalSize)) return;
      if (!get4Bytes(inputFID, &next4Bytes)) return;
      fseek(inputFID, -4, SEEK_CUR); // seek back over "next4Bytes"

      if ((nalSize&0xFFFF0000) == 0x01FE0000) {
	/* This 4-byte 'NAL size' is really the start of a 0x200-byte block of 'track 2' data.
	   Skip over it:
	*/
	if (fseek(inputFID, 0x200-4, SEEK_CUR) != 0) break;
	continue;
      } else if ((nalSize&0xFFFF0000) == 0x211C0000 ||
		 (nalSize&0xFFFF0000) == 0x212C0000 ||
		 (nalSize&0xFFFF0000) == 0x214E0000 ||
		 (nalSize&0xFFFF0000) == 0x217C0000 ||
		 (nalSize&0xFFFF0000) == 0x2ECF0000 ||
		 (nalSize&0xFFFF0000) == 0x38110000 ||
		 (nalSize&0xFFFF0000) == 0x5D9C0000 ||
		 (nalSize&0xFFFF0000) == 0x5DBB0000 ||
		 (nalSize&0xFFFF0000) == 0x80210000) {
	/* This 4-byte 'NAL size' is really the start of a 0x1F9-byte block of 'track 2' data.
	   Skip over it:
	*/
	if (fseek(inputFID, 0x1F9-4, SEEK_CUR) != 0) break;
	continue;
      } else if (nalSize == 0x05c64e6f ||
		 ( ((nalSize&0xFFFF0000) == 0x00f80000) && (next4Bytes == 0x20303020) )) { 
	/* This 4-byte 'NAL size' is really the start of a block from a 'metadata' track.
	   Skip over it:
	*/
	unsigned remainingMetadataSize;
	if (nalSize == 0x05c64e6f) {
	  /* In this case, there is no initial binary stuff */
	  remainingMetadataSize = 0x05c6;
	  if (fseek(inputFID, -2, SEEK_CUR) != 0) break; /* back up to the printable metadata */
	} else {
	  if (fseek(inputFID, 0xF6, SEEK_CUR) != 0) break; /* skip over initial binary stuff */

	  /* The next two bytes might be a length count for the rest of the metadata: */
	  if (!get2Bytes(inputFID, &remainingMetadataSize)) return;
	}

	// Check whether the first 4 bytes of this 'remaining data' really is printable ASCII.
	// If it's not, then the 'two-byte count' was really the start of the next "nalSize":
	if (remainingMetadataSize >= 4 && metadataIsPrintable) {
	  if (!get4Bytes(inputFID, &next4Bytes)) return;
	  fseek(inputFID, -4, SEEK_CUR); // seek back over "next4Bytes" 
	  
	  if (((next4Bytes>>24)&0xFF) < 0x20 || ((next4Bytes>>24)&0xFF) > 0x7E ||
	      ((next4Bytes>>16)&0xFF) < 0x20 || ((next4Bytes>>16)&0xFF) > 0x7E ||
	      ((next4Bytes>>8)&0xFF) < 0x20 || ((next4Bytes>>8)&0xFF) > 0x7E ||
	      (next4Bytes&0xFF) < 0x20 || (next4Bytes&0xFF) > 0x7E) {
	    // Some of these are non-printable => assume that it's not printable ASCII:
	    remainingMetadataSize = 0;
	  }
	} else {
	  remainingMetadataSize = 0;
	}

	if (remainingMetadataSize > 0) {
	  /* Assume that printable metadata continues */
	  if (++printableMetadataCount == 1) {
	    /* For the first occurrence of this metadata, print it out: */
	    unsigned long savePos = ftell(inputFID);
	    unsigned char c;

	    fprintf(stderr, "\nSaw initial metadata block:");
	    do {
	      c = fgetc(inputFID);
	      fprintf(stderr, "%c", c);
	    } while (c != '\n' && c != 0x00);
	    fseek(inputFID, savePos, SEEK_SET); /* restore our old position */
	  }
	  if (fseek(inputFID, remainingMetadataSize, SEEK_CUR) != 0) break;
	} else {
	  /* Backup to the assumed "nalSize" position */
	  if (fseek(inputFID, -2, SEEK_CUR) != 0) break;
	  metadataIsPrintable = 0; // assumed from now on
	}
	continue;
      } else if (nalSize == 0x00fe462f) {
	/* This 4-byte 'NAL size' is really the start of a 0x100-byte block from a 'metadata' track.
	   Skip over it:
	*/
	if (++printableMetadataCount == 1) {
	  // For the first occurrence of this metadata, print it out:
	  unsigned long savePos = ftell(inputFID);
	  unsigned char c;

	  fprintf(stderr, "\nSaw initial metadata block:");
	  fprintf(stderr, "%c", 0x46); fprintf(stderr, "%c", 0x2f); // start of printable data
	  do {
	    c = fgetc(inputFID);
	    fprintf(stderr, "%c", c);
	  } while (c != '\n');
	  fseek(inputFID, savePos, SEEK_SET); /* restore our old position */
	}
	if (fseek(inputFID, 0x100-4, SEEK_CUR) != 0) break;
	continue;
      } else if (nalSize == 0 || nalSize > 0x00FFFFFF) {
	unsigned long filePosition = ftell(inputFID)-4;

	fprintf(stderr, "\n(Anomalous NAL unit size 0x%08x @ file position 0x%08lx (%lu MBytes))\n", nalSize, filePosition, filePosition/1000000);
	fprintf(stderr, "(We can't repair any more than %lu MBytes of this file - sorry...)\n", filePosition/1000000);
	/* We can't recover from this, so stop here: */
	break;
      }

      putStartCode(outputFID);
      while (nalSize-- > 0) {
	wr(fgetc(inputFID));
      }
    }
  }
}
