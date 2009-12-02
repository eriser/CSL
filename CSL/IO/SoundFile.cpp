//
//  Abst_SoundFile.cpp -- CSL's abstract sound file class
//	See the copyright notice and acknowledgment of authors in the file COPYRIGHT
//

#include "SoundFile.h"

using namespace csl;

// Abst_SoundFile Constructors

Abst_SoundFile::Abst_SoundFile(string tpath, int tstart, int tstop) : 
			WavetableOscillator(), Writeable(), Seekable(), 
			mPath(string(tpath)), 
			mIsValid(false), 
			mIsLooping(false),
			mStart(tstart), 
			mStop(tstop),
			mRate(1.0) {
	mCurrentFrame = 0;
//	this->initFromSndfile();							// now do the init fcn
//	logMsg("Opened sound file \"%s\"", mPath.c_str());
}

Abst_SoundFile::Abst_SoundFile(string folder, string tpath, int tstart, int tstop) : 
			WavetableOscillator(), Writeable(), Seekable(), 
			mPath(folder + tpath), 
			mIsValid(false), 
			mIsLooping(false),
			mStart(tstart), 
			mStop(tstop),
			mRate(1.0) {
	mCurrentFrame = 0;
//	printf("SF: \"%s\"\n", mPath.c_str());
//	this->initFromSndfile();							// now do the init fcn
//	logMsg("Opened sound file \"%s\"", mPath.c_str());
}

///< Copy constructor -- shares sample buffer

Abst_SoundFile::Abst_SoundFile(Abst_SoundFile & otherSndFile) :
			WavetableOscillator(), Writeable(), Seekable(), 
			mMode(otherSndFile.mode()), 
			mIsValid(otherSndFile.isValid()), 
			mIsLooping(otherSndFile.isLooping()),
			mStart(otherSndFile.startFrame()), 
			mStop(otherSndFile.stopFrame()),
			mRate(otherSndFile.playbackRate()) {
	this->setWaveform(otherSndFile.mWavetable);
	mCurrentFrame = 0;
	if ( ! otherSndFile.isCached())
		logMsg(kLogError, "Cannot copy uncached sound file \"%s\"", mPath.c_str());
	setPath(otherSndFile.path());
//	logMsg("Open sound file \"%s\"", mPath.c_str());
}

// Clean up data allocated

Abst_SoundFile::~Abst_SoundFile() {
//	freeBuffer();		this is done by the WaveTableOsc destructor
}

void Abst_SoundFile::setPath(string tpath) {
	mPath = tpath;
}

unsigned Abst_SoundFile::channels() const {
	return mIsValid ? mNumChannels : 1;
}

float Abst_SoundFile::durationInSecs() {
	return mIsValid ? ((float) (mStop - mStart) / (float) mFrameRate) : 0.0;
}

void Abst_SoundFile::freeBuffer() {
	if (mWavetable.mAreBuffersAllocated)			// if no buffer allocated
		mWavetable.freeBuffers();
}

// check if the buffer's big enough

void Abst_SoundFile::checkBuffer(unsigned numFrames) {
	if ( ! mWavetable.mAreBuffersAllocated) {				// if no sample read buffer allocated
		mWavetable.setSize(mNumChannels, numFrames);
		mWavetable.allocateBuffers();
	} else if (mWavetable.mNumFrames < numFrames) {			// if asking for more samples than fit in the buffer
		logMsg("Reallocating sound file buffers (%d)", numFrames * mNumChannels);	
		mWavetable.freeBuffers();
		mWavetable.setSize(mNumChannels, numFrames);
		mWavetable.allocateBuffers();
	}
//#ifdef CSL_USE_SRConv
//	if ( ! mSRConvBuffer.mAreBuffersAllocated) {					// if no SRC buffer allocated
//		mSRConvBuffer.setSize(1, CGestalt::maxBufferFrames() * mNumChannels);
//		mSRConvBuffer.allocateBuffers();
//	}
//#endif
}

// average all the channels to mono

void Abst_SoundFile::mergeToMono() {
	if ( ! mWavetable.mAreBuffersAllocated)
		return;
	if (mNumChannels == 1)
		return;
	Buffer newBuf(1, mWavetable.mNumFrames);	// create a new mono buffer
	newBuf.allocateBuffers();
												// sum channels
	for (unsigned i = 0; i < mWavetable.mNumFrames; i++) {
		sample sum = 0.0f;
		for (unsigned j = 0; j < mNumChannels; j++)
			sum += mWavetable.mBuffers[j][i];
												// store the average
		newBuf.mBuffers[0][i] = sum / mNumChannels;
	}											// now overwrite my own mWavetable
	mWavetable.copyFrom(newBuf);
	newBuf.mDidIAllocateBuffers = false;
	mWavetable.mDidIAllocateBuffers = true;
}

// ~~~~~~~ Accessors ~~~~~~~~

unsigned Abst_SoundFile::duration() const {
	return mNumFrames;
}

void Abst_SoundFile::setStart(int val) { 
	mStart = val;
	if (mStart < 0) 
		mStart = 0;
	if ((unsigned) mStart >= duration()) 
		mStart = (int) duration();
	if (mIsValid)
		seekTo(mStart);
}

void Abst_SoundFile::setStartSec(float val) {
	setStart((int) (val * mFrameRate));
}

void Abst_SoundFile::setStartRatio(float val) {
	setStart((int) (val * duration()));
}

void Abst_SoundFile::setStop(int val) { 
	mStop = val;
	if (mStop < 0) 
		mStop = 0;
	int du = (int) duration();
	if (mStop > du) 
		mStop = du;
}

void Abst_SoundFile::setStopSec(float val) {
	setStop((int) (val * mFrameRate));
}

void Abst_SoundFile::setStopRatio(float val) {
	setStop((int) (val * duration()));
}

// Rate and transposition

void Abst_SoundFile::setRate(UnitGenerator & frequency) { 
	if ( ! mInputs[CSL_RATE])
		this->addInput(CSL_RATE, frequency);
	else
		mInputs[CSL_RATE]->mUGen = & frequency;
}

void Abst_SoundFile::setRate(float frequency) { 
	mRate = frequency;
#ifdef CSL_DEBUG
	logMsg("FrequencyAmount::set scale input value");
#endif
}

bool Abst_SoundFile::isActive() {
	if ( ! mIsValid)
		return false;
	return (mCurrentFrame < (unsigned) mStop);
}

bool Abst_SoundFile::isCached() { 
	return (mWavetable.mNumFrames == duration()); 
}

// trigger the file to start

void Abst_SoundFile::trigger() {
	if (mStart > 0) {
		mCurrentFrame = mStart;
		seekTo(mStart);
	} else {
		mCurrentFrame = 0;
		seekTo(0);
	}
}

// set the pointer to the end of the file

void Abst_SoundFile::setToEnd() {
	mCurrentFrame = mStop;
}


// log snd file props

void Abst_SoundFile::dump() {
	const char * nam = path().c_str();
	if (strlen(nam) > 50)
		logMsg("SndFile \"%s\"\n\t\t%d Hz, %d ch, %5.3f sec", 
				nam, frameRate(), channels(), durationInSecs());
	else
		logMsg("SndFile \"%s\" - %d Hz, %d ch, %5.3f sec", 
				nam, frameRate(), channels(), durationInSecs());
}

////////////// next_buffer -- the work is done here //////////

void Abst_SoundFile::nextBuffer(Buffer &outputBuffer) throw(CException) {
	unsigned numFrames = outputBuffer.mNumFrames;
	unsigned currentFrame = mCurrentFrame;
	DECLARE_SCALABLE_CONTROLS;						// declare the scale/offset buffers and values

	if (currentFrame >= (unsigned) mStop) {			// if done
		outputBuffer.zeroBuffers();
		return;
	}
	if (currentFrame + numFrames >= (unsigned) mStop) {		// if final window
		numFrames = mStop - currentFrame;
		outputBuffer.zeroBuffers();
	}
	if ( ! this->isCached()) {						// if not playing from cache buffer
		this->readBufferFromFile(numFrames);		// read from file
	}

	LOAD_SCALABLE_CONTROLS;							// load the scaleC and offsetC from the constant or dynamic value

	if (mRate == 1) {												// if playing at normal rate
		if (scalePort->isFixed() && offsetPort->isFixed()			// if fixed scale/offset, use memcpy
					&& (scaleValue == 1) && (offsetValue == 0)) {
			unsigned numBytes = numFrames * sizeof(sample);			// buffer copy loop
			for (unsigned i = 0; i < outputBuffer.mNumChannels; i++) {
				int which = csl_min(i, (mNumChannels - 1));
				SampleBuffer sndPtr = mWavetable.monoBuffer(which) + currentFrame;
				SampleBuffer outPtr = outputBuffer.monoBuffer(i);
				memcpy(outPtr, sndPtr, numBytes);
			}
		} else {													// else loop applying scale/offset
			sample samp;
			for (unsigned i = 0; i < outputBuffer.mNumChannels; i++) {
				SampleBuffer buffer = outputBuffer.monoBuffer(i);	// get pointer to the selected output channel
				SampleBuffer dPtr = mWavetable.monoBuffer(csl_min(i, (mNumChannels - 1))) + currentFrame;
				for (unsigned j = 0; j < numFrames; j++) {			// sample loop
					samp = (*dPtr++ * scaleValue) + offsetValue; 	// get and scale the file sample
					*buffer++ = samp;
					UPDATE_SCALABLE_CONTROLS;						// update the dynamic scale/offset
				}
				scalePort->resetPtr();
				offsetPort->resetPtr();
			}
		}
	} else {										// if mRate != 1.0, 
													// use wavetable interpolation
		this->setFrequency(mRate);
		for (unsigned i = 0; i < mNumChannels; i++)
			WavetableOscillator::nextBuffer(outputBuffer, i);	
	}
	currentFrame += numFrames;								// incrememnt buf ptr
	if ((currentFrame >= (unsigned) mStop) && mIsLooping)	// if we are past the end of the file...
		currentFrame = 0;							// this will click, have to call nextBuffer() recursively here
	mCurrentFrame = currentFrame;					// store back to member
	return;
}


////////////////////////////////////////////////////////////////////////////////////////////
//
// Sound Cue implementation
//

// Generic constructors

SoundCue::SoundCue() {
	mFile = NULL;
	mCurrent = 0;
}

SoundCue::SoundCue(string name, Abst_SoundFile *file, int start, int stop) : 
			mName(name), 
			mFile(file), 
			mStart(start), 
			mStop(stop) { 
	mCurrent = mStart;
	mReadRate = (UnitGenerator*) 0;
}

SoundCue::~SoundCue() { }

// Read an instance's data from a file
// The format is that used in the SeSpSp layer index file:
// 	 name 	   start 	stop
//	b5b.r21e.aiff 13230080 15876096		-- NB: these are in FRAMES

void SoundCue::readFrom (FILE *input) {
	char cmName[128];
	unsigned start, stop;

	fscanf(input, "%s %u %u\n", cmName, &start, &stop);
	mName = cmName;
	mStart = start;
	mStop = stop;
	mCurrent = mStart;
}

// Pretty-print the receiver

void SoundCue::dump() {
	logMsg("\tSC: \"%s\" %d - %d (%.3f)\n", mName.c_str(), mStart, mStop, ((float)(mStop - mStart) / (float)(mFile->frameRate() * mFile->channels())));
}

// Copy samples from the file's sample buffer to the output
// I assume that the # of channels in the output is the same as in my file, i.e.,
// To play a mono file, "wrap" it in a Deinterleaver

void SoundCue::nextBuffer(Buffer &outputBuffer) throw(CException) {
	unsigned numFrames = outputBuffer.mNumFrames;
	unsigned toCopy, toZero;
	SampleBuffer out;
	
	for (unsigned h = 0; h < outputBuffer.mNumChannels; h++) {
		out = outputBuffer.mBuffers[h];
		if (mCurrent >= mStop) {			// if done
			for (unsigned i = 0; i < numFrames; i++) 
				*out++ = 0.0;
			continue;
		}	
		SampleBuffer samps = (mFile->mWavetable.monoBuffer(0)) + (mCurrent * mFile->channels()) + h;
		if ( ! mFile->isValid()) {		// if no file data
			printf("\tCannot play uncached sound file \"%s\"n", mName.c_str());
			for (unsigned i = 0; i < numFrames; i++) 
				*out++ = 0.0;
			continue;
		}
		toCopy = numFrames;		// figure out how many samples to copy
		toZero = 0;	
		if ((mCurrent + numFrames) >= (unsigned) mStop) {
			toCopy = mStop - mCurrent;
			toZero = numFrames - toCopy;
		//	printf("\tSample \"%s\" finished\n", mName);
		}						// Now do the copy loops
		for (unsigned i = 0; i < toCopy; i++) 
			*out++ = *samps++;
		for (unsigned i = 0; i < toZero; i++) 
			*out++ = 0.0;
	}
	mCurrent += toCopy;			// increment the receiver's sample pointer
	return;
}

// Utility functions

bool SoundCue::isActive() { 
	if (mFile == 0)
		return (false);
	return (mCurrent < mStop);
}

void SoundCue::setToEnd(void) {
	mCurrent = mStop;
}

void SoundCue::trigger(void) {
	mCurrent = mStart;
	mFloatCurrent = 0.0;
}

#ifdef UNDEFINED

//class SampleFile : public Abst_SoundFile {
//public:
//	SampleFile();							/// Constructor
//	SampleFile(string name, Abst_SoundFile *file = 0, int start = 1, int stop = -1);
//	~SampleFile();
//							/// Data members
//	unsigned mMIDIKey;			// sample's MIDI key #
//	double mFrequency;			// sample's actual frequency
//	double mMinRatio;			// min transp.
//	double mMaxRatio;			// max transp. (often 1.0)
//	
//	double ratioForKey(int desiredMIDI);
//	double ratioForPitch(int desiredMIDI);

SampleFile::SampleFile() {

}

SampleFile::SampleFile(string tpath, unsigned MIDIKey, double frequency, double minRatio, double maxRatio) {

}

SampleFile::~SampleFile() {

}

#endif
