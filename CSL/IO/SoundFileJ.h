///
/// SoundFileJ.h -- CSL's concrete sound file class using JUCE
///
///	See the copyright notice and acknowledgment of authors in the file COPYRIGHT
///

#ifndef CSL_SoundFileJ_H
#define CSL_SoundFileJ_H

#include "SoundFile.h"
#include "../JuceLibraryCode/JuceHeader.h"

namespace csl {

///
/// JUCE sound file
///

class JSoundFile : public Abst_SoundFile {
public:
	JSoundFile(string path, int start = -1, int stop = -1);
	JSoundFile(JSoundFile & otherSndFile);			///< Copy constructor -- shares sample buffer
	~JSoundFile();

													/// Factory methods
	static JSoundFile * openSndfile(string path, int start = -1, int stop = -1, bool doRead = true);
//	static JSoundFile * openSndfile(float maxDurInSecs, string path);

	unsigned duration() const;						///< number of frames in the sound file
	SoundFileFormat format();						///< get format
													// open file and get stats
	void openForRead(bool load = true) throw (CException);
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

	AudioFormatReader * mAFReader;					///< my reader
	AudioFormatWriter * mAFWriter;					///< and my writer
	File * mIOFile;									///< my JUCE file
	FileOutputStream * mOutStream;

protected:
	void initFromSndfile();							///< read SF header
//	void convertFormat(unsigned num, unsigned start);
	
};

}

#endif
