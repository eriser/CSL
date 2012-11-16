///
/// SoundFileMP3.h -- concrete sound file class using libMAD for mp3 and libFAAD for mp4
///
///	See the copyright notice and acknowledgment of authors in the file COPYRIGHT
///
/// These classes load their target file into a buffer on openForRead().
/// There is no MP3/4 writing (encoding) at present.
///

#ifndef CSL_SoundFileMP3_H
#define CSL_SoundFileMP3_H

#include "SoundFileL.h"			// abstract class header
#include <sndfile.h>			// libsndfile header file
#include <string.h>
								// NB: set these as compile-time flags now
//#define CSL_USE_SRConv			// Support sample rate conversion								
//#define DEFAULT_MP3_RATE 44100	// default sample rate for MP3 files

namespace csl {

///
/// DecodedFile - abstract class for MP3/4/AAC - read-only decoded files
/// Provides a BufferVector for reading files of unknown size.
///

class DecodedFile : public LSoundFile {
public:							/// Constructor.
	DecodedFile(string path = "", int start = -1, int stop = -1, float maxDurInSecs = 0.0);
	~DecodedFile();
//	virtual void openForRead() throw (CException);			///< open file and get stats
															/// Open a file for write (an error for decoded files).
	void openForWrite(SoundFileFormat format = kSoundFileFormatAIFF, 
					  unsigned channels = 1, 
					  unsigned rate = 44100,
					  unsigned bitDepth = 16) throw (CException);
	void openForReadWrite() throw (CException);				///< open r/w (error)
															/// seek to some position
//	unsigned seekTo(int position, SeekPosition whence = kPositionStart) throw(CException);
	void nextBuffer(Buffer &outB) throw (CException);		///< copy next buffer from cache

								/// store samples into the receiver, adding another buffer if necessary. Returns true if we should do so, false if there are no more samples.
	bool writeBuffer (float * stereo_buffer, unsigned num_samples);
	bool writeBuffer (float * L_buffer, float * R_buffer, unsigned num_samples);
	bool writeBuffer (int * L_buffer, int * R_buffer, unsigned num_samples);

protected:
	BufferVector mBuffers;		///< temp decoder buffers
								/// merge (concatenate) the buffers into 1 after loading
	void mergeBuffers();
	bool checkBufferStack(unsigned numFrames);
	bool addBuffer(unsigned numChannels, unsigned numFrames);
	unsigned totalSize();		///< answer the total size of the buffer list
};

//-----------------------------------------------------------------------------//

#ifdef USE_libMAD				// Support MP3 file reading using libMAD

#include "math.h"
#include <fcntl.h>
#include <sys/mman.h>
#include <sys/stat.h>
#include "mad.h"

///
/// MP3File - decodes MP3s into a buffer upon openForRead()
///

class MP3File : public DecodedFile {
public:							/// Constructor.
	MP3File(string path = "", int start = -1, int stop = -1);
	MP3File(float maxDurInSecs, string path);
	~MP3File();

	void openForRead() throw (CException);			///< open file and get stats

	unsigned mMP3Rate;								///< the actual rate of the MP3 file

protected:
	int decodeMP3() throw (CException);				///< decode an MP3 file into a buffer
};

#endif // USE_libMAD

//-----------------------------------------------------------------------------//

#ifdef USE_libFAAD				// Support AAC/MP4 file reading using libFAAD

#define HAVE_STDINT_H

#include <faad.h>
#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <fcntl.h>
#include <neaacdec.h>
#include <mp4ff.h>

#define MAX_CHANNELS 6			// make this higher to support files with more channels

/// FAAD file/buffer struct

typedef struct {
    long bytes_into_buffer;		///< read pos
    long bytes_consumed;		///< write pos
    long file_offset;			///< offset in file
    unsigned char *buffer;		///< storage
    int at_eof;					///< if at EOF
    FILE *infile;				///< input FILE ptr
} aac_buffer;

///
/// MP4File - decodes AAC & MP4 into a buffer upon openForRead()
///

class MP4File : public DecodedFile {
public:							/// Constructor.
	MP4File(string path = "", int start = -1, int stop = -1);
	MP4File(float maxDurInSecs, string path);
	~MP4File();

	void openForRead() throw (CException);			///< open file and get stats
	
protected:
	csl::Status decodeAACfile(FILE * inFile) throw (CException);	///< read and decode an AAC file
	csl::Status decodeMP4file(FILE * inFile) throw (CException);	///< read and decode an MP4 file
	void printMP4(mp4ff_t * infile, int track, unsigned *song_length);

								/// inst vars
	NeAACDecHandle hDecoder;
	NeAACDecFrameInfo frameInfo;
	NeAACDecConfigurationPtr config;
	mp4AudioSpecificConfig mp4ASC;
	aac_buffer aacBuff;
};

#endif // USE_libFAAD

} // end of namespace

#endif
