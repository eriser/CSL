//
//  InOut.h -- implementation of the class that copies the input buffer to the output buffer
//	See the copyright notice and acknowledgment of authors in the file COPYRIGHT
//

#include "InOut.h"
#include <math.h>
#include <stdlib.h>
#include <string.h>

using namespace csl;

// Generic InOut implementation

// Arguments passed to the constructor are optional. Defaults to 2 channels in and out.

InOut::InOut(IO * anIO, unsigned inChan, unsigned outChan, InOutFlags f) 
		: mIO(anIO), mMap(outChan,  CGestalt::blockSize()),
			mInChan(inChan), mOutChan(outChan), 
			mFlags(f) {
	mGains = (float *) malloc(mOutChan * sizeof(float));
}

//InOut::InOut(IO * anIO, unsigned inChan, unsigned outChan, InOutFlags f, ...) 
//		: mIO(anIO), mMap(outChan,  CGestalt::blockSize()),
//			mInChan(inChan), mOutChan(outChan), 
//			mFlags(f) {
//	mGains = (float *) malloc(mOutChan * sizeof(float));
//	va_list ap;
//	va_start(ap, f);
//	for (unsigned i = 0; i < outChan; i++) 
//		mMap.mChannelMap[i] = va_arg(ap, unsigned);
//	va_end(ap);
//}

InOut::InOut(UnitGenerator & myInput, unsigned inChan, unsigned outChan, InOutFlags f)
		: mIO(NULL), mMap(outChan, CGestalt::blockSize()), 
			mInChan(inChan), mOutChan(outChan), 
			mFlags(f) {
	this->addInput(CSL_INPUT, myInput);
	mGains = (float *) malloc(mOutChan * sizeof(float));
}

//InOut::InOut(UnitGenerator & myInput, unsigned inChan, unsigned outChan, InOutFlags f, ...)
//		: mIO(NULL), mMap(outChan,  CGestalt::blockSize()),
//			mInChan(inChan), mOutChan(outChan), 
//			mFlags(f) {
//	this->addInput(CSL_INPUT, myInput);
//	mGains = (float *) malloc(mOutChan * sizeof(float));
//	va_list ap;
//	va_start(ap, f);
//	for (unsigned i = 0; i < outChan; i++) 
//		mMap.mChannelMap[i] = va_arg(ap, unsigned);
//	va_end(ap);
//}

InOut::~InOut() { }

void InOut::setChanMap(unsigned * chans) {					///< set channel map
	for (unsigned i = 0; i < mOutChan; i++)
		mMap.mChannelMap[i] = chans[i];
}

void InOut::setChanGains(float * values) {					///< set gain array
	for (unsigned i = 0; i < mOutChan; i++)
		mGains[i] = values[i];
}

void InOut::setGain(unsigned index, float tvalue) {			///< set gain value at index
		mGains[index] = tvalue;
}

//	float *mGains;		///< amplitude scales
//	sample * monoBuffer(unsigned bufNum) { return mBuffers[mChannelMap[bufNum]]; }
//	IO * mIO;				///< The (Singleton) IO pointer (or NULL, to act as an effect)
//	BufferCMap mMap;		///< the mapped buffer
//	float *mGains;			///< amplitude scales

// nextBuffer simply grabs the IO's input buffer

void InOut::nextBuffer(Buffer & outputBuffer) throw (CException) {
	unsigned numFrames = outputBuffer.mNumFrames;
//	unsigned numOut = outputBuffer.mNumChannels;
	unsigned monoBufferByteSize = outputBuffer.mMonoBufferByteSize;
	SampleBufferVector outBuffers = outputBuffer.mBuffers;
	SampleBufferVector inBuffers;

	if (mIO) {						// either grab the mic input, or my effect in chain
		Buffer & theInput = mIO->getInput(outputBuffer.mNumFrames, outputBuffer.mNumChannels);
		inBuffers = theInput.mBuffers;
	} else {
		Effect::pullInput(numFrames);
		Port * tinPort = mInputs[CSL_INPUT];
		inBuffers = tinPort->mBuffer->mBuffers;
	}

	switch (mFlags) {
		case kNoProc:
			for (unsigned i = 0; i < mOutChan; i++)
				memcpy(outBuffers[i], inBuffers[i%mInChan], monoBufferByteSize);
			break;
		case kLR2M:
			for (unsigned i = 0; i < mOutChan; i++) {
				sample * outPtr =  outBuffers[i];
				sample * inPtr1 =  inBuffers[i%mInChan];
				sample * inPtr2 =  inBuffers[(i+1)%mInChan];
				for (unsigned j = 0; j < numFrames; j++)
					*outPtr++ = (*inPtr1++ * mGains[i]) + (*inPtr2++* mGains[i]);
			}
			break;
		case kL2M:
		case kR2M:
			break;
		case kN2M:					// N-to-M-channel mapping with bufferCMap
			for (unsigned i = 0; i < mOutChan; i++) {
				sample * outPtr =  outBuffers[i];
				int which = mMap.mChannelMap[i];
				if (which < 0) continue;
				sample * inPtr1 =  inBuffers[which];
				for (unsigned j = 0; j < numFrames; j++)
					*outPtr++ = *inPtr1++ * mGains[i];
			}
			break;
	}
}

