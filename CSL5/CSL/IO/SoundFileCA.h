///
/// SoundFileCA.h -- CSL's concrete sound file class for CoreAudio files
///		Note: this is experimental and incomplete as of CSL 5.0
///
///	See the copyright notice and acknowledgment of authors in the file COPYRIGHT
///

#ifndef CSL_SoundFileCA_H
#define CSL_SoundFileCA_H

#include "SoundFile.h"
#import <AudioToolbox/ExtendedAudioFile.h>

namespace csl {

///
/// CoreAudio sound file
///

class CASoundFile : public Abst_SoundFile {
public:
	CASoundFile(CFURLRef path);
	CASoundFile(string path, int start = -1, int stop = -1);
	CASoundFile(string folder, string path, int start = -1, int stop = -1);
	CASoundFile(CASoundFile & otherSndFile);		///< Copy constructor -- shares sample buffer
	~CASoundFile();

	SoundFileFormat format();						///< get format
	void dump();									///< log snd file props
	void openForRead() throw (CException);			///< open file and get stats
													/// Open a file for write. 
													/// Default values are some common format.
	void openForWrite(SoundFileFormat format = kSoundFileFormatAIFF, 
					unsigned channels = 1, 
					unsigned rate = 44100,
					unsigned bitDepth = 16) throw (CException);
	void openForReadWrite() throw (CException);		///< open r/w
	void close();									///< close file
													/// seek to some position
	unsigned seekTo(int position, SeekPosition whence) throw(CException);
	void readBufferFromFile(unsigned numFrames);	///< read a buffer from the file (possibly all of it)

	void writeBuffer(Buffer &inputBuffer) throw (CException);	///< write a buffer of data into the file

protected:
	void initFromSndfile();							///< read SF header
//	void convertFormat(unsigned num, unsigned start);

	CFURLRef mURL;									///< the full URL of the file
	AudioFileID mSoundID;							///< the CS audio file ID
	bool mBigEndian;								///< flag for endian-ness of sound file (varies byformat)
};

}

#endif
