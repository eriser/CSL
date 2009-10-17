///
/// SoundFileL.h -- concrete sound file class using libsndfile
///
///	See the copyright notice and acknowledgment of authors in the file COPYRIGHT
///
/// The MP3 file playback is a mild hack; it first converts each MP3 file to a temp AIFF file and uses 
/// that for playback. There is no MP3 writing (encoding) at present.
///

#ifndef CSL_SoundFileL_H
#define CSL_SoundFileL_H

#include "SoundFile.h"			// abstract class header
#include <sndfile.h>			// libsndfile header file

								// NB: set these as compile-time flags now
//#define USE_MP3				// Support MP3 file reading using libMAD
								// otherwise assume AIFF or WAV file
//#define CSL_USE_SRConv		// Support sample rate conversion								
								
#include "CSL_Includes.h"
#include <sndfile.h>			// libsndfile header file
#include <string.h>

#ifdef CSL_USE_SRConv
	#include <samplerate.h>		// libsamplerate header file

								// Sample Rate Convertor interpolation modes -- slow to fast
	#define CSL_SRC_MODE SRC_SINC_BEST_QUALITY
	// #define CSL_SRC_MODE SRC_SINC_MEDIUM_QUALITY
	// #define CSL_SRC_MODE SRC_SINC_FASTEST
	// #define CSL_SRC_MODE SRC_ZERO_ORDER_HOLD
	// #define CSL_SRC_MODE SRC_LINEAR
#endif

namespace csl {

///
/// Here's the sound file reader/writer class; it assumes libSndFile and interleaved sample buffers
///

class LSoundFile : public Abst_SoundFile {
public:							/// Constructor. Values not passed default to null.
	LSoundFile(string path, int start = -1, int stop = -1);
	LSoundFile(string folder, string path, int start = -1, int stop = -1);
	LSoundFile(LSoundFile & otherSndFile);			///< Copy constructor -- shares sample buffer
	~LSoundFile();

	SoundFileFormat format();						///< get format
	
	void openForRead() throw (CException);	///< open file and get stats
													/// Open a file for write. 
													/// Default values are some common format.
	void openForWrite(SoundFileFormat format = kSoundFileFormatAIFF, 
					unsigned channels = 1, 
					unsigned rate = 44100,
					unsigned bitDepth = 16) throw (CException);
	void openForReadWrite() throw (CException);		///< open r/w
	void close();									///< close file
															/// seek to some position
	unsigned seekTo(int position, SeekPosition whence = kPositionStart) throw(CException);
													/// read a buffer from the file (possibly all of it)
	void readBufferFromFile(unsigned numFrames);

								/// UGen operations
	void nextBuffer(Buffer &outB) throw (CException);				///< copy next buffer from cache
	void writeBuffer(Buffer &inputBuffer) throw (CException);		///< write a buffer of data into the file

#ifdef CSL_USE_SRConv								/// perform sample-rate conversion
	void convertRate(char * mTempPath, int fromRate, int toRate);
#endif

	SF_INFO * sfInfo() { return mSFInfo; }			///< libsndfile sf-info struct
	SNDFILE * sndFile() { return mSndfile; }		///< libsndfile handle

protected:
	SF_INFO * mSFInfo;								///< libsndfile sf-info struct
	SNDFILE * mSndfile;								///< libsndfile handle
	Interleaver mInterleaver;						///< File IO interleaver/deinterleaver
#ifdef CSL_USE_SRConv
	Buffer mSRConvBuffer;							///< used by the sample rate convertor
	SRC_STATE * mSRateConv;							///< sample rate convertor (SRC) state struct
	SRC_DATA mSRateData;							///< SRC call data struct
	int mSRCReturn;									///< SRC error flag
#endif
	void initFromSndfile();							///< read SF header
	void checkBuffer(unsigned numFrames);			///< allocate buffer lazily
};

///
/// MP3File - decodes MP3s into a temp AIFF SoundFile upon openForRead()
///

#ifdef USE_MP3

#define MP3_TEMP_EXT ".aiff"	// what format to save temp decoded MP3 files

#ifdef CSL_MACOSX				// where to save temp decoded MP3 files - platform-dependent
#define MP3_TEMP_DIR "/tmp/"
#else
#define MP3_TEMP_DIR "/tmp/"		
// #define MP3_TEMP_DIR "."		
#endif

#define DEFAULT_MP3_RATE 44100			// default sample rate for MP3 files

class MP3File : public LSoundFile {
public:							/// Constructor.
	MP3File(string path = "", int start = -1, int stop = -1);
	~MP3File();

	char * tempPath() { return mTempPath; }			///< temp file name, e.g., /tmp/xxx.aiff
	virtual void openForRead() throw (CException);	///< open file and get stats
													/// Open a file for write (an error for MP3 files).
	virtual void openForWrite(SoundFileFormat format = kSoundFileFormatAIFF, 
					unsigned channels = 1, 
					unsigned rate = 44100,
					unsigned bitDepth = 16) throw (CException);
	virtual void openForReadWrite() throw (CException);		///< open r/w (error)
	
	char * mTempDir;								///< static for temp dir name - e.g., "/tmp/"
	unsigned mMP3Rate;								///< the actual rate of the MP3 file

protected:
	int decodeMP3() throw (CException);				///< create a copy of an MP3 file for reading
//	int closef(int which);
	char * mTempPath;								///< name of temp AIFF file
	LSoundFile * mTempFile;							///< temp AIFF file
};

// Temp name macro -- also used by other services
// NB: This is UNIX-specific - ToDo: use run-time file sep

#define MP3_TEMP_NAME(in_path, out_path, temp_dir) {				\
	sprintf(out_path, "%s%s", temp_dir, in_path);					\
	char * lastDot = strrchr(out_path, (int) '.');					\
	if (lastDot) sprintf(lastDot, MP3_TEMP_EXT);					\
	char * pos = out_path;											\
	pos += strlen(temp_dir);										\
	pos = strchr(pos, (int) '/');									\
	while (pos) {													\
		*pos = '_';													\
		pos = strchr(pos, (int) '/');								\
	}}

// AIFF name macro -- also used by other services
// This assumes the in_path ends with ".mp3"

#define AIFF_TEMP_NAME(in_path, out_path) {							\
		strcpy(out_path, in_path);									\
		strcpy((out_path + strlen(out_path) - 4), ".aiff"); }

#endif // USE_MP3

} // end of namespace

#endif
