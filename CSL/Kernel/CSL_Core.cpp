//
//  CSL_Core.cpp -- the implementation of Buffer, UnitGenerator, IO, mix-ins, etc.
//
//	See the copyright notice and acknowledgment of authors in the file COPYRIGHT
//

#include "CSL_Core.h"	// it's all declared here
#include "RingBuffer.h"	// UnitGenerator uses RingBuffers
#include <string.h>		// for bzero / memset
#include <stdlib.h>		// for malloc

using namespace csl;

//
// Basic buffer methods
//

// Constructor with size args does not allocate

Buffer::Buffer(unsigned numChannels, unsigned numFrames) :
		mBuffers(0),
		mNumChannels(0),
		mNumFrames(0),
		mMonoBufferByteSize(0),
		mSequence(0),
		mAreBuffersAllocated(false),
		mDidIAllocateBuffers(false),
		mIsPopulated(false),
		mAreBuffersZero(true),
		mType(kSamples) {
	setSize(numChannels, numFrames);
}

// Free on destruction

Buffer::~Buffer() {
	if (mDidIAllocateBuffers)
		freeBuffers();
}

// SetSize creates the vector of sample pointers, does not allocate the sample storage


void Buffer::setSize(unsigned numChannels, unsigned numFrames) {
	if (mAreBuffersAllocated && mDidIAllocateBuffers && (mNumChannels != numChannels) 
				&& (mNumFrames < numFrames)) {
		freeBuffers();
		mDidIAllocateBuffers = false;
		mAreBuffersAllocated = false;
	}
	if (mNumChannels != numChannels) {
		mNumChannels = numChannels;					// Transfer parameters to member variables
		if (mBuffers)
			delete mBuffers;						// free buffer pointers
		mBuffers = new SampleBuffer[numChannels];	// reserve space for buffers
		for (unsigned i = 0; i < mNumChannels; i++)
			mBuffers[i] = 0;
	}
	if (mNumFrames != numFrames) {
		mNumFrames = numFrames;
		mMonoBufferByteSize = mNumFrames * sizeof(sample);
	}
	mIsPopulated = false;
}

// set size and don't allocate vector space either

void Buffer::setSizeOnly(unsigned numChannels, unsigned numFrames) {
	mNumChannels = numChannels;
	mNumFrames = numFrames;
	mMonoBufferByteSize = numFrames * sizeof(sample);
}

// allocate the sample arrays

void Buffer::allocateBuffers() throw (MemoryError) {
	for (unsigned i = 0; i < mNumChannels; i++) {
		SAFE_MALLOC(mBuffers[i], sample, mNumFrames);
	}
	mAreBuffersAllocated = true;
	mDidIAllocateBuffers = true;
}

// free them carefully

void Buffer::freeBuffers() {
	if ( ! mAreBuffersAllocated)
		return;					// they aren't allocated -- I shouldn't free them
	if ( ! mDidIAllocateBuffers)
		return;					// I didn't allocate them -- I shouldn't free them
#ifdef CSL_DEBUG
	logMsg("Buffer::freeBuffers(%d, %d)", mNumChannels, mNumFrames);
#endif
	if (& mBuffers == NULL)
		return;
	for (unsigned i = 0; i < mNumChannels; i++) {
		if (mBuffers[i]) {
			delete mBuffers[i];		// do the delete
			mBuffers[i] = NULL;		// set to zero right away
		}
	}
	mAreBuffersAllocated = false;
	mDidIAllocateBuffers = false;
	delete mBuffers;
	mBuffers = 0;
}

// empty the sample buffers

void Buffer::zeroBuffers() {
	if ( ! mAreBuffersAllocated)
		return;
	for (unsigned i = 0; i < mNumChannels; i++)
		memset(mBuffers[i], 0, mMonoBufferByteSize);
	mAreBuffersZero = true;
}

// fill with a constant

void Buffer::fillWith(sample value) {
	float* buffer = NULL;
	unsigned outBufNum, i;
	unsigned numChans = mNumChannels;
	unsigned numFrames = mNumFrames;
	for (outBufNum = 0; outBufNum < numChans; outBufNum++) {
		buffer = mBuffers[outBufNum];
		for (i = 0; i < numFrames; i++)
			*buffer++ = value;
	}
	mAreBuffersZero = false;
}

// Copy everything but samples from the argument to the receiver

void Buffer::copyFrom(Buffer & source) throw (RunTimeError) {
	this->freeBuffers();						// free everything
	delete mBuffers;
	mNumChannels = source.mNumChannels;			// copy members
	mNumFrames = source.mNumFrames;
	mMonoBufferByteSize = source.mMonoBufferByteSize;
	mBuffers = new SampleBuffer[mNumChannels];	// reserve space for buffers
												// copy sample pointers
	for (unsigned outBufNum = 0; outBufNum < mNumChannels; outBufNum++) {
		mBuffers[outBufNum] = source.monoBuffer(outBufNum);
	}
	mAreBuffersZero = false;					// set flags
	mAreBuffersAllocated = true;
	mDidIAllocateBuffers = false;
}

// Copy samples from the argument to the receiver; be paranoid about size limits

void Buffer::copySamplesFrom(Buffer & source) throw (RunTimeError) {
	if ((source.mNumChannels > mNumChannels) || (source.mNumFrames > mNumFrames))
		throw RunTimeError("Can't reallocate buffers at run-time");
	mNumChannels = source.mNumChannels;
	mNumFrames = source.mNumFrames;
	mMonoBufferByteSize = source.mMonoBufferByteSize;
	for (unsigned outBufNum = 0; outBufNum < mNumChannels; outBufNum++) {
		memcpy(mBuffers[outBufNum], source.monoBuffer(outBufNum), mMonoBufferByteSize);
	}
	mAreBuffersZero = false;
}

///////////////////////////////////

#ifdef CSL_DSP_BUFFER				/// Buffer Sample Processing (optional)

#include <libtsp.h>					// for the autocorrelation

///< get the root-mean-square of the samples

sample Buffer::rms(unsigned chan) {
	unsigned thech = chan % mNumChannels;
	unsigned numFrames = mNumFrames;
	sample val, sum = 0.0;
	sample* buffer = mBuffers[thech];
	for (unsigned i = 0; i < numFrames; i++) {
		val = *buffer++;
		sum += (val * val);
	}
	return sum / numFrames;
}

///< get the average of the samples

sample Buffer::avg(unsigned chan) {
	unsigned thech = chan % mNumChannels;
	unsigned numFrames = mNumFrames;
	sample sum = 0.0;
	sample* buffer = mBuffers[thech];
	for (unsigned i = 0; i < numFrames; i++)
		sum += *buffer++;
	return sum / numFrames;
}

///< get the max of the samples

sample Buffer::max(unsigned chan) {
	unsigned thech = chan % mNumChannels;
	unsigned numFrames = mNumFrames;
	sample val, tmax = 0.0;
	sample* buffer = mBuffers[thech];
	for (unsigned i = 0; i < numFrames; i++) {
		val = *buffer++;
		if (fabs(val) > tmax)		tmax - fabs(val);
	}
	return tmax;
}

///< get the min of the samples

sample Buffer::min(unsigned chan) {
	unsigned thech = chan % mNumChannels;
	unsigned numFrames = mNumFrames;
	sample val, tmax = 0.0;
	sample* buffer = mBuffers[thech];
	for (unsigned i = 0; i < numFrames; i++) {
		val = *buffer++;
		if (fabs(val) > tmax)		tmax - fabs(val);
	}
	return tmax;
}

///< count the zero-crossings in the samples

unsigned int Buffer::zeroX(unsigned chan) {
	unsigned thech = chan % mNumChannels;
	unsigned numFrames = mNumFrames, count = 0, sign = 0;
	sample val;
	sample* buffer = mBuffers[thech];
	for (unsigned i = 0; i < numFrames; i++) {
		val = *buffer++;
		if (((val > 0) && sign) || ((val < 0) && ! sign)) {
			count += 1;
			sign = 1 - sign;
		}
	}
	return count;
}

///< answer the index of the peak value

unsigned int Buffer::indexOfPeak(unsigned chan) {
	unsigned thech = chan % mNumChannels;
	unsigned numFrames = mNumFrames, index;
	sample val, tmax = -1000000.0;
	sample* buffer = mBuffers[thech];
	for (unsigned i = 0; i < numFrames; i++) {
		val = *buffer++;
		if (val > tmax) {
			tmax = val;
			index = i;
		}
	}
	return index;
}

///< answer the index of the peak value

unsigned int Buffer::indexOfPeak(unsigned chan, unsigned lo, unsigned hi) {
	unsigned index = 0, thech = chan % mNumChannels;
	sample val, tmax = -100000.0;
	sample* buffer = mBuffers[thech] + lo;
	for (unsigned i = lo; i < hi; i++) {
		val = *buffer++;
		if (val > tmax) {
			tmax = val;
			index = i;
		}
	}
	return index;
}

///< answer the index of the peak value

unsigned int Buffer::indexOfMin(unsigned chan) {
	unsigned thech = chan % mNumChannels;
	unsigned numFrames = mNumFrames, index;
	sample val, tmin = 1000000.0;
	sample* buffer = mBuffers[thech];
	for (unsigned i = 0; i < numFrames; i++) {
		val = *buffer++;
		if (val < tmin) {
			tmin = val;
			index = i;
		}
	}
	return index;
}

///< answer the index of the peak value

unsigned int Buffer::indexOfMin(unsigned chan, unsigned lo, unsigned hi) {
	unsigned index = 0, thech = chan % mNumChannels;
	sample val, tmin = 1000000.0;
	sample* buffer = mBuffers[thech] + lo;
	for (unsigned i = lo; i < hi; i++) {
		val = *buffer++;
		if (val < tmin) {
			tmin = val;
			index = i;
		}
	}
	return index;
}

///< write the autocorrelation into the given array

void Buffer::autocorrelation(unsigned chan, SampleBuffer result) {
	unsigned thech = chan % mNumChannels;
	unsigned numFrames = mNumFrames;
	sample* buffer = mBuffers[thech];
		//	SPautoc (const float in[], int in_size, float out[], int out_size)
		//		- out[i] is autocorrelation with lag i
	SPautoc (buffer, numFrames, result, numFrames);			// perform autocorrelation
}

#endif // CSL_DSP_BUFFER


// Sample buffer with channel map and count
// the map is so that one can have (e.g.,) a buffer that stands for 3 channels within an 8-channel space

BufferCMap::BufferCMap() : Buffer() { }

BufferCMap::BufferCMap(unsigned numChannels, unsigned numFrames) :
			Buffer(numChannels, numFrames),
			mRealNumChannels(numChannels) { }

BufferCMap::BufferCMap(unsigned numChannels, unsigned realNumChannels, unsigned numFrames) :
			Buffer(numChannels, numFrames),
			mRealNumChannels(realNumChannels) { }

BufferCMap::~BufferCMap() { }


/////////////////////////////////////////////////
//
// UnitGenerator implementation
//

// mono by default

UnitGenerator::UnitGenerator(unsigned rate, unsigned chans) :  Model(),
				mFrameRate(rate),
				mNumChannels(chans),
				mCopyPolicy(kCopy),
				mNumOutputs(0),
				mOutputCache(0),
				mSequence(0),
				mName(0) { }

void UnitGenerator::zeroBuffer(Buffer & outputBuffer, unsigned outBufNum) {
	float * buffer = outputBuffer.monoBuffer(outBufNum);
	memset(buffer, 0, outputBuffer.mMonoBufferByteSize);
}

// output management and auto-fan-out

void UnitGenerator::addOutput(UnitGenerator * ugen) {
	mOutputs.push_back(ugen);				// if adding the 2nd output, set up fan-out cache
	if ((mNumOutputs == 1) && (mOutputCache == 0)) {
		mOutputCache = new RingBuffer();
		mOutputCache->mBuffer.setSize(mNumChannels, CGestalt::maxBufferFrames());
		mOutputCache->mBuffer.allocateBuffers();
	}
	mNumOutputs++;
}

void UnitGenerator::removeOutput(UnitGenerator * ugen) {
	UGenVector::iterator pos;
	for (pos = mOutputs.begin(); pos != mOutputs.end(); ++pos) {
		if (*pos == ugen) {
			mOutputs.erase(pos);
			break;
		}
	}
	mNumOutputs--;
}

// pretty-print the receiver

void UnitGenerator::dump() {
	logMsg("a UnitGenerator with %d outputs", mOutputs.size());
}

//
// Generic next buffer function: call the private (mono) version for each I/O channel;
// copy or expand depending on the mCopyPolicy;
// copy to the private cache in case of fan-out.
//

void UnitGenerator::nextBuffer(Buffer & outputBuffer) throw (CException) {
	unsigned numOutputChannels = outputBuffer.mNumChannels;
	unsigned bufferByteSize = outputBuffer.mMonoBufferByteSize;
	SampleBuffer buffer0 = outputBuffer.monoBuffer(0);
#ifdef CSL_DEBUG
	logMsg("UnitGenerator::nextBuffer");
#endif
	if (mNumOutputs > 1) {					// if we're doing auto-fan-out
		if (outputBuffer.mSequence <= mSequence) {		// if we've already computed this seq #
											//  reset the ring buffer read tap back just enough
			mOutputCache->seekTo((int) (0 - outputBuffer.mNumFrames));
											// and block copy samples from the cache into the output
			mOutputCache->nextBuffer(outputBuffer);
			return;							// finished!
		}
	}
											// Copy the output buffer samples
	switch (mCopyPolicy) {
	default:
	case kCopy:								// compute 1 channel and copy it
		this->nextBuffer(outputBuffer, 0);	// this is where most of the work gets done in CSL
		for (unsigned i = 1; i < numOutputChannels; i += mNumChannels)
			memcpy (outputBuffer.monoBuffer(i), buffer0, bufferByteSize);
		break;
	case kExpand:							// loop through the requested output channels
		for (unsigned i = 0; i < numOutputChannels; i += mNumChannels)
			nextBuffer(outputBuffer, i);	// call private nextBuffer per-channel
		break;
	case kIgnore:							// Only do as many channels as I have
		this->nextBuffer(outputBuffer, 0);
		break;
	}
	
	if (mNumOutputs > 1)					// if we're doing auto-fan-out and this is the first time
		mOutputCache->writeBuffer(outputBuffer);				// store it in the ring buffer
	mSequence = csl_max(mSequence, outputBuffer.mSequence);		// remember my seq #
	
	this->changed((void *) & outputBuffer);						// signal dependents (if any) of my change
}

// Generic private next buffer implementation; by default, just zero out the buffer

void UnitGenerator::nextBuffer(Buffer & outputBuffer,  unsigned outBufNum) throw (CException) {
	for (unsigned i = 0; i < mNumChannels; i++)
		zeroBuffer(outputBuffer, i);		// my subclasses do more interesting things here
}

/////////////////////////////////////////////////
//
// Port implementation

Port::Port() :
				mUGen(NULL),
				mBuffer(new Buffer(1, CGestalt::blockSize())),
				mValue(0),
				mValuePtr(& mValue),
				mPtrIncrement(0) { }

Port::Port(UnitGenerator * ug) :
				mUGen(ug),
				mBuffer(new Buffer(ug->numChannels(), CGestalt::blockSize())),
				mValue(0),
				mPtrIncrement(1) {
	mBuffer->allocateBuffers();
	mValuePtr = mBuffer->monoBuffer(0) - 1;		// set to -1 since nextValue does pre-increment
}

Port::Port(float value) :
				mUGen(NULL),
				mBuffer(NULL),
				mValue(value),
				mValuePtr(& mValue),
				mPtrIncrement(0) { }

Port::~Port() { }

// check the port's buffer and allocate it if needed

void Port::checkBuffer() throw (LogicError) {
	Buffer * buf = mBuffer;
	UnitGenerator * ug = mUGen;

	if (buf == 0)
		throw LogicError("Checking an unassigned buffer");
	if ((buf->mNumChannels != ug->numChannels()) || (buf->mNumFrames == 1)) {
		buf->freeBuffers();
		buf->setSize(ug->numChannels(), CGestalt::blockSize());
		buf->allocateBuffers();
	}
}

// Call this to reset the pointer without re-pulling the input (see SumOfSines)

void Port::resetPtr() {
	if (mPtrIncrement)								// if I'm dynamic
		mValuePtr = (mBuffer->monoBuffer(0)) - 1;	// set to -1 since nextValue does pre-increment
}

// Answer whether the receiver is active

bool Port::isActive() {
	if (mUGen == NULL)
		return (true);
	else
		return mUGen->isActive();
}

// pretty-print the receiver

void Port::dump() {
	logMsg("a Port with UGen = %x, value = %g, incr = %d", mUGen, mValue, mPtrIncrement);
	if (mUGen != 0) {
		printf("\t");
		mUGen->dump();
	}
}

/////////////////////////////////////////////////
//
// Controllable implementation -- Grab the dynamic values for the scale and offset controls

void Controllable::pullInput(Port * thePort, unsigned numFrames) throw (CException) {
#ifdef CSL_DEBUG
	logMsg("Controllable::pullInput");
#endif
	if (thePort == NULL) {
//		logMsg("port == null!");
		return;
	}
	UnitGenerator * theUG = thePort->mUGen;			// get its UGen
	if (theUG == NULL) {							// if it's a static variable
//		logMsg("UG == null!");
		return;										// ignore it
	}
	Buffer * theBuffer = thePort->mBuffer;			// else get the buffer
	if (theBuffer == NULL) {						// if it's a static variable
//		logMsg("Buffer == null!");
		return;										// ignore it
	}
	theBuffer->mNumFrames = numFrames;
	theBuffer->mType = kSamples;
	
	theUG->nextBuffer(* theBuffer);				// and ask the UGen for nextBuffer()
	
	theBuffer->mIsPopulated = true;
	thePort->mValuePtr = (thePort->mBuffer->monoBuffer(0)) - 1;
	thePort->mValueIndex = 0;
}

// Controllable implementation -- this version writes into the buffer you pass it

void Controllable::pullInput(Port * thePort, Buffer & theBuffer) throw (CException) {
	if (thePort == NULL)
		return;
	UnitGenerator * theUG = thePort->mUGen;			// get its UGen
	if (theUG == NULL)								// if it's a static variable
		return;										// ignore it
	theUG->nextBuffer(theBuffer);					// and ask the UGen for nextBuffer()
	
	theBuffer.mIsPopulated = true;
	thePort->mValuePtr = (theBuffer.monoBuffer(0)) - 1;
	thePort->mValueIndex = 0;
}

// Plug in a unit generator to the named input slot

void Controllable::addInput(CSL_MAP_KEY key, UnitGenerator & uGen) {
#ifdef CSL_DEBUG
	logMsg("Controllable::set input \"%d\" UGen", key);
#endif
	Port * thePort = mInputs[key];		// get the named port
	if (thePort != 0) 					// if port found
		delete thePort;
	thePort = new Port(&uGen);
	mInputs[key] = thePort;				// add it to the list of inputs
	uGen.addOutput((UnitGenerator *) this);	// be sure to add me as an output of the other guy
}

void Controllable::addInput(CSL_MAP_KEY key, float value) {
#ifdef CSL_DEBUG
	logMsg("Controllable::set input \"%d\" to %g", key, value);
#endif
	Port * thePort = mInputs[key];		// get the named port
	if (thePort == 0) {					// if no port found
		thePort = new Port(value);
		mInputs[key] = thePort;			// add it to the list of inputs
	} else
		thePort->mValue = value;
}

// get a port

Port * Controllable::getPort(CSL_MAP_KEY name) {
	return(mInputs[name]);
}

// Pretty-print the receiver

void Controllable::dump() {
	logMsg("a Controllable with the map:");
	for (PortMap::iterator pos = mInputs.begin(); pos != mInputs.end(); ++pos) {
		switch (pos->first) {
		case CSL_FREQUENCY:
			logMsg("	key: Frequency = UG: ");
			break;
		case CSL_SCALE:
			logMsg("	key: Scale = UG: ");
			break;
		case CSL_OFFSET:
			logMsg("	key: Offset = UG: ");
			break;
		case CSL_INPUT:
//		case CSL_INPUT_L:
//		case CSL_INPUT_R:
			logMsg("	key: Input = UG: ");
			break;
		default:
			logMsg("	key: Other = UG: ");

		}
		pos->second->dump(); // go up the graph tree dumping inputs and controls
	}
}

/////////////////////////////////////////////////
//
// Phased implementation -- Constructors

Phased::Phased() : mPhase(0.0f) {
	mInputs[CSL_FREQUENCY] = new Port;
#ifdef CSL_DEBUG
	logMsg("Phased::add freq input");
#endif
}

Phased::Phased(UnitGenerator & frequency, float phase) : mPhase(phase) {
	this->addInput(CSL_FREQUENCY, frequency);
#ifdef CSL_DEBUG
	logMsg("Phased::add freq input");
#endif
}

Phased::Phased(float frequency, float phase) : mPhase(phase) {
	this->addInput(CSL_FREQUENCY, frequency);
#ifdef CSL_DEBUG
	logMsg("Phased::add freq input");
#endif
}

Phased::~Phased() { /* ?? */ }

// Accessors

void Phased::setFrequency(UnitGenerator & frequency) {
	this->addInput(CSL_FREQUENCY, frequency);
}

void Phased::setFrequency(float frequency) {
	this->addInput(CSL_FREQUENCY, frequency);
}

// Scalable -- Constructors

Scalable::Scalable() {
	mInputs[CSL_SCALE] = new Port;
	mInputs[CSL_OFFSET] = new Port;
#ifdef CSL_DEBUG
	logMsg("Scalable::add null inputs");
#endif
}

Scalable::Scalable(float scale) {
	this->addInput(CSL_SCALE, scale);
	mInputs[CSL_OFFSET] = new Port;
#ifdef CSL_DEBUG
	logMsg("Scalable::add scale input");
#endif
}

Scalable::Scalable(float scale, float offset)  {
	this->addInput(CSL_SCALE, scale);
	this->addInput(CSL_OFFSET, offset);
#ifdef CSL_DEBUG
	logMsg("Scalable::add scale/offset input values");
#endif
}

Scalable::Scalable(UnitGenerator & scale, float offset) {
	this->addInput(CSL_SCALE, scale);
	this->addInput(CSL_OFFSET, offset);
#ifdef CSL_DEBUG
	logMsg("Scalable::add scale/offset input values");
#endif
}

Scalable::Scalable(UnitGenerator & scale, UnitGenerator & offset) {
	this->addInput(CSL_SCALE, scale);
	this->addInput(CSL_OFFSET, offset);
#ifdef CSL_DEBUG
	logMsg("Scalable::add scale/offset input values");
#endif
}

Scalable::~Scalable() { /* no-op for now */ }

// Scalable -- Accessors

void Scalable::setScale(UnitGenerator & scale) {
	this->addInput(CSL_SCALE, scale);
#ifdef CSL_DEBUG
	logMsg("Scalable::set scale input UG");
#endif
}

void Scalable::setScale(float scale) {
	this->addInput(CSL_SCALE, scale);
#ifdef CSL_DEBUG
	logMsg("Scalable::set scale input value");
#endif
}

void Scalable::setOffset(UnitGenerator & offset) {
	this->addInput(CSL_OFFSET, offset);
#ifdef CSL_DEBUG
	logMsg("Scalable::set offset input UG");
#endif
}

void Scalable::setOffset(float offset) {
	this->addInput(CSL_OFFSET, offset);
#ifdef CSL_DEBUG
	logMsg("Scalable::set offset input value");
#endif
}

/////////////////////////////////////////////////
//
// Effect implementation

Effect::Effect() : UnitGenerator() {
	isInline = false;
//	mInputs[CSL_INPUT] = new Port;
#ifdef CSL_DEBUG
	logMsg("Effect::add null input");
#endif
 }

Effect::Effect(UnitGenerator & input) : UnitGenerator() {
	isInline = false;
	this->addInput(CSL_INPUT, input);
#ifdef CSL_DEBUG
	logMsg("Effect::add input UG");
#endif
}

bool Effect::isActive() {
	Port * iPort = mInputs[CSL_INPUT];
	if (iPort)
		return (iPort->isActive());
	return false;
};

void Effect::setInput(UnitGenerator & input) {
	isInline = false;
	this->addInput(CSL_INPUT, input);
#ifdef CSL_DEBUG
	logMsg("Effect::add input");
#endif
}

// Get my input's next buffer

void Effect::pullInput(Buffer & outputBuffer) throw (CException) {
#ifdef CSL_DEBUG
	logMsg("Effect::pullInput");
#endif
	if (isInline)			// if inline, just use the input pointer
		mInputPtr = outputBuffer.mBuffers[0];
	else {
		Port * iPort = mInputs[CSL_INPUT];
							// else pull a buffer from my input
		Controllable::pullInput(iPort, outputBuffer);
		
		mInputPtr = outputBuffer.mBuffers[0];
	}
}

void Effect::pullInput(unsigned numFrames) throw (CException) {
#ifdef CSL_DEBUG
	logMsg("Effect::pullInput");
#endif
	Port * iPort = mInputs[CSL_INPUT];
	Controllable::pullInput(iPort, numFrames);
	mInputPtr = iPort->mBuffer->mBuffers[0];
}

// FanOut methods

FanOut::FanOut(UnitGenerator & in, unsigned taps)
			: Effect(in), mOutputs(taps), mCurrent(taps) {
#ifdef CSL_DEBUG
	logMsg("FanOut::FanOut");
#endif
}

// nextBuffer just pulls the input every "mOutputs" calls

void FanOut::nextBuffer(Buffer & outputBuffer) throw (CException) {
	if (++mCurrent >= mOutputs) {
		Effect::pullInput(mBuffer);
		mCurrent = 0;
	}						// Copy the output buffer samples
	outputBuffer.copySamplesFrom(mBuffer);
}

// Splitter class -- a de-multiplexer for multi-channel signals

Splitter::Splitter(UnitGenerator & in, unsigned taps) : FanOut(in, taps) { }

// Like for FanOut, next-buffer calls the input every mOutput calls, 
// but it only copies one channel at a time to the output

void Splitter::nextBuffer(Buffer & outputBuffer) throw (CException) {
	unsigned numOutputChans = outputBuffer.mNumChannels;
	Buffer * buf = inPort()->mBuffer;

	if (numOutputChans != 1) {
		throw LogicError("Asking for a stereo output of a channel splitter");
	}
	if (mCurrent == mOutputs) {
		Effect::pullInput(outputBuffer.mNumFrames);
		mCurrent = 0;
	}
	unsigned bufferByteSize = outputBuffer.mMonoBufferByteSize;
	SampleBuffer dest = outputBuffer.monoBuffer(0);
	SampleBuffer src = buf->monoBuffer(mCurrent);
	memcpy(dest, src, bufferByteSize);
	mCurrent++;
}

// Joiner class -- a multiplexer for multi-channel signals

Joiner::Joiner(UnitGenerator & in1, UnitGenerator & in2) {
	mInputs.push_back(& in1);
	mInputs.push_back(& in2);
}

void Joiner::addInput(UnitGenerator & in) {	///< add the argument to vector of inputs
	mInputs.push_back(& in);
}

// nextBuffer calls each of the input channels as a mono channel

void Joiner::nextBuffer(Buffer & outputBuffer) throw (CException) {
	unsigned numOutputChans = outputBuffer.mNumChannels;
	Buffer tempBuffer;

	if (numOutputChans != mInputs.size()) {
		throw LogicError("Asking for the wrong number of channels of a channel joiner");
	}
						// set up the fake mono buffer
	tempBuffer.setSize(1, outputBuffer.mNumFrames);
						// loop through the mono inputs
	for (unsigned i = 0; i < mInputs.size(); i++) {
						// put the mono in samples into 1 channel of the output
		tempBuffer.mBuffers[0] = outputBuffer.monoBuffer(i);
						// get a buffer of mono samples from one of the inputs
		mInputs[i]->nextBuffer(tempBuffer);
	}
}

// Writeable implementation

void Writeable::writeBuffer(Buffer & inputBuffer, unsigned bufNum) throw (CException) {
				// no-op here
}

void Writeable::writeBuffer(Buffer & inputBuffer) throw (CException) {
	unsigned numBufs = inputBuffer.mNumChannels;
	for (unsigned i = 0; i < numBufs; i++)
		writeBuffer(inputBuffer, i);
}

// Seekable

void Seekable::reset() throw (CException) {		// reset-to-zero
	int pos = seekTo(0, kPositionStart);
	if (pos == 0)
		throw IOError("Error seeking");
}

// Interleave = copy from CSL-style Buffer object to an interleaved sample vector

void Interleaver::interleave(Buffer & output, SampleBuffer samples, 
			unsigned numFrames, unsigned numChannels) throw (CException) {

	unsigned numOutputChannels = output.mNumChannels;
	unsigned numChannelsToInterleave = csl_min(numOutputChannels, numChannels);
	unsigned numChannelsBeyond = numChannels - numChannelsToInterleave;

	for (unsigned frame = 0; frame < numFrames; frame++) {
		for (unsigned channel = 0; channel < numChannelsToInterleave; channel++)
			*samples++ = output.monoBuffer(channel)[frame];
		for (unsigned j = 0; j < numChannelsBeyond; j++)
			*samples++ = 0;
	}
}

// Interleave, short * version

void Interleaver::interleave(Buffer & output, short * samples, unsigned numFrames, 
			unsigned numChannels) throw (CException) {

	unsigned numOutputChannels = output.mNumChannels;
	unsigned numChannelsToInterleave = csl_min(numOutputChannels, numChannels);
	unsigned numChannelsBeyond = numChannels - numChannelsToInterleave;

	for (unsigned frame = 0; frame < numFrames; frame++) {
		for (unsigned channel = 0; channel < numChannelsToInterleave; channel++)
			*samples++ = (short) ((output.monoBuffer(channel)[frame]) * 32767.0);
		for (unsigned j = 0; j < numChannelsBeyond; j++)
			*samples++ = 0;
	}
}

/// Remap = re-assign channels from the source buffer to the target while interleaving

void Interleaver::interleaveAndRemap(Buffer & output, SampleBuffer samples, unsigned numFrames, 
			unsigned numChannels, unsigned *channelMap) throw (CException) {

	unsigned numOutputChannels = output.mNumChannels;
	unsigned numChannelsToInterleave = csl_min(numOutputChannels, numChannels);
	unsigned numChannelsBeyond = numChannels - numChannelsToInterleave;
	unsigned *map = channelMap;

	for (unsigned frame = 0; frame < numFrames; frame++) {
		for (unsigned channel = 0; channel < numChannelsToInterleave; channel++)
			*samples++ = output.monoBuffer(map[channel])[frame];
		for (unsigned j = 0; j < numChannelsBeyond; j++)
			*samples++ = 0;
	}
}

// De-interleave = copy from interleaved SampleBuffer to CSL Buffer object

void Interleaver::deinterleave(Buffer & output, SampleBuffer samples, unsigned numFrames, 
			unsigned numChannels) throw (CException) {

	unsigned numOutputChannels = output.mNumChannels;
	unsigned numChannelsToDeinterleave = csl_min(numOutputChannels, numChannels);
	SampleBuffer currentOutputBuffer;
	SampleBuffer inputBuffer;
	unsigned i;

	for (i = 0; i < numChannelsToDeinterleave; i++) {
		currentOutputBuffer = output.monoBuffer(i);
		inputBuffer = samples + i;
		for (unsigned j = 0; j < numFrames; j++, inputBuffer += numChannels) {
			*currentOutputBuffer++ = *inputBuffer;
		}
	}
	if (numOutputChannels > numChannelsToDeinterleave) {
		for (i = numChannelsToDeinterleave; i < numOutputChannels; i++)
			memset(output.monoBuffer(i), 0, output.mMonoBufferByteSize);
	}
}

// De-interleave, short * version

void Interleaver::deinterleave(Buffer & output, short * samples, unsigned numFrames, 
			unsigned numChannels) throw (CException) {

	unsigned numOutputChannels = output.mNumChannels;
	unsigned numChannelsToDeinterleave = csl_min(numOutputChannels, numChannels);
	SampleBuffer currentOutputBuffer;
	short * inputBuffer;
	unsigned i;

	for (i = 0; i < numChannelsToDeinterleave; i++) {
		currentOutputBuffer = output.monoBuffer(i);
		inputBuffer = samples + i;
		for (unsigned j = 0; j < numFrames; j++, inputBuffer += numChannels) {
			*currentOutputBuffer++ = ((float )*inputBuffer) / 32767.0f;
		}
	}
	if (numOutputChannels > numChannelsToDeinterleave) {
		for (i = numChannelsToDeinterleave; i < numOutputChannels; i++)
			memset(output.monoBuffer(i), 0, output.mMonoBufferByteSize);
	}
}

//////////////////////////// IO class methods /////////////////////////////////

// Global array of all known IO devices

vector < IODevice *> gIODevices;

// General IO Constructors in all modes

IO::IO(unsigned s_rate, unsigned b_size, int in_device, int out_device, 
				unsigned in_chans, unsigned out_chans)
		: mGraph(NULL), mNumFramesPlayed(0), mSequence(0), 
		  mLoggingPeriod(CGestalt::loggingPeriod()),
		  mNumInChannels(in_chans), mNumOutChannels(out_chans), 
		  mNumRealInChannels(in_chans), mNumRealOutChannels(out_chans),
		  mStatus(kIONew) {
	logMsg("Create IO: %d s @ %d Hz; %d i %d o", b_size, s_rate, in_chans, out_chans);
}

// set/clear the root generator

void IO::setRoot(UnitGenerator & root) {
	root.addOutput((UnitGenerator *) this);
	mGraph = & root;
}

void IO::clearRoot() {
	if (mGraph != NULL)
		mGraph->removeOutput((UnitGenerator *) this);
	mGraph = NULL;
}

// increment and answer my seq #

unsigned IO::getAndIncrementSequence() {
	mSequence++;
	return mSequence;
}

// IO method to call a CSL client's DSP graph

void IO::pullInput(Buffer & outBuffer, SampleBuffer out) throw(CException) {

#ifdef DO_TIMING
	static struct timeval * mte = & mThen;
	static struct timeval * mno = & mNow;
	GET_TIME(mte);
#endif
//	unsigned numFrames = outBuffer.mNumFrames;
//	unsigned numChans = outBuffer.mNumChannels;
	
	if (mGraph) {
		try {
			outBuffer.mSequence = this->getAndIncrementSequence();
			
			mGraph->nextBuffer(outBuffer);		////// call the graph's nextBuffer method //////
			
		} catch (CException ex) {
							// handler: log error and play silence
			logMsg(kLogError, "An error occured in the CSL nextBuffer method: %s\n", 
					ex.mMessage.c_str());
//			memset(out, 0, (numFrames * numChans * sizeof(sample)));
		}
	}
#ifdef DO_TIMING
	GET_TIME(mno);
	printTimeStatistics(mno, mte, & mThisSec, & mTimeSum, & mTimeVals);
#endif
}

#ifdef CSL_WINDOWS

#ifdef DO_TIMING		// DO_TIMING 

int (timeval *val, void * e) {
	LARGE_INTEGER ticksPerSecond;
	LARGE_INTEGER tick;   // A point in time
					// get the high resolution counter's accuracy
	QueryPerformanceFrequency(&ticksPerSecond);
	float ticksPerUSecond = ticksPerSecond.QuadPart / 1000000;
					// what time is it?
	QueryPerformanceCounter(&tick);
	val->tv_sec = tick.QuadPart/ticksPerSecond.QuadPart;
	val->tv_usec = tick.QuadPart/ticksPerUSecond;
//	cout << "Fraction: " << val->tv_sec << "/" << val->tv_usec  << " " << ticksPerUSecond << endl;
	return 0;
}
#endif

#endif

// IO Timing method

#ifdef DO_TIMING
//#include "Mixer.h"
//#include "Instrument.h"
//extern std::vector<Instrument *> gInstLibrary;

void IO::printTimeStatistics(struct timeval * now, struct timeval * then, long * thisSec, 
						long * timeSum, long * timeVals) {

	if (now->tv_sec - * thisSec > (long) mLoggingPeriod) {		// print stats once every so often
		*thisSec = now->tv_sec;
		if (*timeSum != 0.0) {									// print and reset counters
			float cycleTime = (float) CGestalt::blockSize() * 1000000.0f / (float) CGestalt::frameRate();
																// remember % usage
			mUsage = (float) *timeSum / *timeVals * 100.0f / cycleTime;
			*timeVals = 0;
			*timeSum = 0;
			logMsg("\tCPU usage: %.2f percent.", mUsage);
//			logMsg("\tIO:: %d active clients", ((Mixer *) mGraph)->activeSources());
//			unsigned num_instruments = library.size();
//			for (unsigned i = 0; i < num_instruments; i++) {
//				Instrument * instr = library[i];
//				if (instr->graph()->isActive())
//					logMsg("\t\tInstrument %s.%d", instr->name().c_str(), i);
//			}
		}
	} else {								// count blocks with active sounds and sum exec. times
		*timeVals += 1;
		*timeSum += SUB_TIMES(now, then);
	}
	// cout << "here now/this: " << now->tvSec << " " << then->tvSec << endl;
}

#endif

// Answer the most recent input buffer

Buffer & IO:: getInput() throw(CException) {
	return getInput(mInputBuffer.mNumFrames, mInputBuffer.mNumChannels);
}

// really get the desired format of input

Buffer & IO:: getInput(unsigned numFrames, unsigned numChannels) throw(CException) {
	if (mNumInChannels == 0)
		throw IOError("Can't get unopened input");
	Interleaver interleaver;
	interleaver.deinterleave(mInputBuffer, mInputPointer, numFrames, numChannels);
	mInputBuffer.mIsPopulated = true;
	return(mInputBuffer);
}

// IO Device

IODevice::IODevice(char * name, unsigned index, unsigned maxIn, unsigned maxOut, bool isIn, bool isOut)
			: mIndex(index), mMaxInputChannels(maxIn), mMaxOutputChannels(maxOut),
				mIsDefaultIn(isIn), mIsDefaultOut(isOut) {
		strcpy(mName, name);
}

IODevice::IODevice(string name, unsigned index, unsigned maxIn, unsigned maxOut, bool isIn, bool isOut)
			: mIndex(index), mMaxInputChannels(maxIn), mMaxOutputChannels(maxOut),
				mIsDefaultIn(isIn), mIsDefaultOut(isOut) {
		strcpy(mName, name.c_str());
}

// pretty-print an IO Device

void IODevice::dump() {
	logMsg("	IO: %d = %s - %d i %d o %g Hz%s%s",
			mIndex,
			mName,
			mMaxInputChannels, mMaxOutputChannels,
			mFrameRate,
			(mIsDefaultIn ? " - def in" : ""),
			(mIsDefaultOut ? " - def out" : ""));
}
