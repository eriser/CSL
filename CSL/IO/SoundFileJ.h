///
/// SoundFileJ.h -- CSL's concrete sound file class using JUCE
///
///	See the copyright notice and acknowledgment of authors in the file COPYRIGHT
///

#ifndef CSL_SoundFileJ_H
#define CSL_SoundFileJ_H

#include "SoundFile.h"

#include <juce.h>

class AudioFormatReader;
class AudioFormatWriter;

namespace csl {

///
/// JUCE sound file
///

class JSoundFile : public Abst_SoundFile {
public:
	JSoundFile(string path, bool load = false);
	JSoundFile(string folder, string path, bool load = false);
//	JSoundFile(string path, int start = -1, int stop = -1);
//	JSoundFile(string folder, string path, int start = -1, int stop = -1);
	JSoundFile(JSoundFile & otherSndFile);			///< Copy constructor -- shares sample buffer
	~JSoundFile();

	unsigned duration() const;						///< number of frames in the sound file
	SoundFileFormat format();						///< get format
	
	void openForRead() throw (CException);			///< open file and get stats
													/// Open a file for write. Default values are some common format.
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

	juce::AudioFormatReader * mAFReader;			///< my reader
	juce::AudioFormatWriter * mAFWriter;			///< and my writer
	juce::File * mIOFile;							///< my JUCE file
	juce::FileOutputStream * mOutStream;

protected:
	void initFromSndfile();							///< read SF header
//	void convertFormat(unsigned num, unsigned start);
	
};

}

#endif
