//
//  Filters.cpp -- implementation of the base Filter class and its standard subclasses
//
//	See the copyright notice and acknowledgment of authors in the file COPYRIGHT
//

#include <stdlib.h>
#include <string.h>
#include <math.h>

#include "Filters.h"

using namespace csl;

// FrequencyAmount -- Constructors

FrequencyAmount::FrequencyAmount() {
#ifdef CSL_DEBUG
	logMsg("FrequencyAmount::add null inputs");
#endif		
}

FrequencyAmount::~FrequencyAmount() { /* no-op for now */ }

// FrequencyAmount -- Accessors

void FrequencyAmount::setFrequency(UnitGenerator & frequency) { 
	this->addInput(CSL_FILTER_FREQUENCY, frequency);
#ifdef CSL_DEBUG
	logMsg("FrequencyAmount::set scale input UG");
#endif		
}

void FrequencyAmount::setFrequency(float frequency) { 
	this->addInput(CSL_FILTER_FREQUENCY, frequency);
#ifdef CSL_DEBUG
	logMsg("FrequencyAmount::set scale input value");
#endif		
}

float FrequencyAmount::getFrequency() {
	return(getPort(CSL_FILTER_FREQUENCY)->nextValue());
}

void FrequencyAmount::setAmount(UnitGenerator & amount) { 
	this->addInput(CSL_FILTER_AMOUNT, amount);
#ifdef CSL_DEBUG
	logMsg("FrequencyAmount::set offset input UG");
#endif		
}

void FrequencyAmount::setAmount(float amount) { 
	this->addInput(CSL_FILTER_AMOUNT, amount);
#ifdef CSL_DEBUG
	logMsg("FrequencyAmount::set offset input value");
#endif		
}

/// Generic Filter class with scalable order and generic next_buffer method
/// that implememnts the canonical filter diference equation.
/// Subclasses must supply filter order and override the setupCoeffs() method.

/// Default constructor generates a zeroth order "do-nothing" filter
Filter::Filter () : Effect(), Scalable(1.f,0.f) {
	init(1,1);
};


Filter::Filter(unsigned num_b, unsigned num_a) : Effect(), Scalable(1.f,0.f) {
	init(num_a, num_b);
}

Filter::Filter(UnitGenerator & in, unsigned num_b, unsigned num_a) : Effect(in), Scalable(1.f,0.f) {
	init(num_a, num_b);
};

/// This constructor takes arrays of coefficients and constructs the filter accordingly.
Filter::Filter(UnitGenerator & in, SampleBuffer bCoeffs, SampleBuffer aCoeffs, unsigned num_b, unsigned num_a) 
		: Effect(in), Scalable(1.f,0.f) {
	init(num_a, num_b);
	setupCoeffs(bCoeffs, aCoeffs, num_b, num_a);
};

void Filter::init(unsigned a, unsigned b) {
	mBNum = b;
	mANum = a;
	mBCoeff[0] = 1.f;
	mACoeff[0] = 1.f;
	mPrevOutputs = new Buffer(1, mANum);
	mPrevInputs  = new Buffer(1, mBNum);
	mPrevOutputs->allocateBuffers();
	mPrevInputs->allocateBuffers();
	mFrameRate = CGestalt::frameRate();
}

/// Filter destructor frees temp memory

Filter::~Filter (void) { 
	if (mPrevOutputs) delete mPrevOutputs;
	if (mPrevInputs) delete mPrevInputs;
};

// This version does in-place filtering

void Filter::nextBuffer(Buffer & outputBuffer, unsigned outBufNum) throw (CException) {
#ifdef CSL_DEBUG
	logMsg("Filter nextBuffer");
#endif	
	SampleBuffer 	out = outputBuffer.monoBuffer(outBufNum);	// get ptr to output channel
	unsigned numFrames = outputBuffer.mNumFrames;				// get buffer length
	SampleBuffer 	prevOuts = mPrevOutputs->monoBuffer(0);
	SampleBuffer 	prevIns = mPrevInputs->monoBuffer(0);	

	DECLARE_SCALABLE_CONTROLS;						// declare the scale/offset buffers and values
	DECLARE_FILTER_CONTROLS;						// declare the freq/bw buffers and values
	LOAD_SCALABLE_CONTROLS;
	LOAD_FILTER_CONTROLS;

	bool isDynamic = false;
	if ((freqPort && ( ! freqPort->isFixed())) || (bwPort && ( ! bwPort->isFixed())))
		isDynamic = true;

	Effect::pullInput(numFrames);					// get some input
	SampleBuffer inputPtr = mInputPtr;				// get a pointer to the input samples

	for (unsigned i = 0; i < numFrames; i++) {		// here's the canonical N-quad filter loop
		if (isDynamic)
			this->setupCoeffs();					// calculate new coefficients for next sample
		prevOuts[0] = 0.f;
		prevIns[0] = scaleValue * *inputPtr++ + offsetValue;	// get next input sample, scale & offset
		for (unsigned j = mBNum - 1; j > 0; j--) {
			prevOuts[0] += mBCoeff[j] * prevIns[j];				// prevIns or prevOuts?
			prevIns[j] = prevIns[j-1];
		}
		prevOuts[0] += mBCoeff[0] * prevIns[0];
		for (unsigned j = mANum - 1; j > 0; j--) {
			prevOuts[0] += -mACoeff[j] * prevOuts[j];
			prevOuts[j] = prevOuts[j-1];
		}
													// put current output in the buffer and increment
		*out++ = (prevOuts[0] * scaleValue) + offsetValue;	

		UPDATE_SCALABLE_CONTROLS;					// update the dynamic scale/offset
	} 
}

/// this version is to be inherited by the subclasses. provides a way to directly supply the filter info

void Filter::setupCoeffs(SampleBuffer bCoeffs, SampleBuffer aCoeffs, unsigned num_b, unsigned num_a ) {
	for (unsigned i = 0; i < num_b; i++)
		mBCoeff[i] = bCoeffs[i];
	for (unsigned i = 0; i < num_a; i++)
		mACoeff[i] = aCoeffs[i];
}

void Filter::clear(void) {
	mPrevInputs->zeroBuffers();
	mPrevOutputs->zeroBuffers();
}

/// log information about myself
void Filter::dump() {
	logMsg("a Filter");
	Scalable::dump();
	UnitGenerator::dump();
	
	printf("A coefficients ");
	for (unsigned i=0;i<mANum;i++) {
		printf("%.2f, ",mACoeff[i]);
	}
	printf("\n");
	printf("B coefficients ");
	for (unsigned i=0;i<mBNum;i++) {
		printf("%.2f, ",mBCoeff[i]);
	}
	printf("\n");
}

/// Butterworth IIR (2nd order recursive) filter.
/// This operates upon a buffer of frames of amplitude samples by applying the following equation
/// y(n) = a0*x(n) + a1*x(n-1) + a2*x(n-2) - b1*y(n-1) - b2*y(n-2) where x is an amplitude sample.
/// It has constructors that can calculate the coefficients from a given cutoff frequency.

Butter::Butter () { }

Butter::Butter (ButterworthType type, float cutoff) : Filter(3,3) {
	mFilterType = type;
	setFrequency(cutoff);
	setAmount(cutoff / 10.0);
	setupCoeffs();
	clear(); 
}

Butter::Butter (UnitGenerator & in, ButterworthType type, float cutoff) : Filter(in,3,3) {
	mFilterType = type;
	setFrequency(cutoff);
	setAmount(cutoff / 10.0);
	setupCoeffs();
	clear(); 
}

Butter::Butter (UnitGenerator & in, ButterworthType type, UnitGenerator & cutoff) : Filter(in,3,3) {
	mFilterType = type;
	setFrequency(cutoff);
	setAmount(100);
	setupCoeffs();
	clear(); 
}

// constructor for dual filter parameters (i.e. band pass and reject)

Butter::Butter (ButterworthType type, float center, float bandwidth) : Filter(3,3) {
	mFilterType = type;
	setFrequency(center);
	setAmount(bandwidth);
	setupCoeffs();
	clear(); 
}

Butter::Butter (UnitGenerator & in, ButterworthType type, float center, float bandwidth) 
		: Filter(in,3,3) {
	mFilterType = type;
	setFrequency(center);
	setAmount(bandwidth);
	setupCoeffs();
	clear(); 
}

Butter::Butter (UnitGenerator & in, ButterworthType type, UnitGenerator & center, UnitGenerator & bandwidth) 
		: Filter(in,3,3) {
	mFilterType = type;
	setFrequency(center);
	setAmount(bandwidth);
	setupCoeffs();
	clear(); 
}

//	 Calculate the filter coefficients based on the frequency characteristics

void Butter::setupCoeffs () {
	float C, D; 					// handy intermediate variables
	float centreFreq = mInputs[CSL_FILTER_FREQUENCY]->nextValue();
	float bandwidth = mInputs[CSL_FILTER_AMOUNT]->nextValue();

	mACoeff[0] = 0.f;
	switch (mFilterType) {			// These are the Butterworth equations
		case BW_LOW_PASS :	
			C = 1 / (tanf (CSL_PI * (centreFreq/mFrameRate)) );
			mBCoeff[0]	= 1 / (1 + (CSL_SQRT_TWO * C) + (C * C) );
			mBCoeff[1]	= 2 * mBCoeff[0];
			mBCoeff[2]	= mBCoeff[0];
			mACoeff[1]	= 2 * mBCoeff[0] * (1 - (C * C));
			mACoeff[2]	= mBCoeff[0] * (1 - (CSL_SQRT_TWO * C) + (C * C) );
			break;
		case BW_HIGH_PASS :
			C = tanf (CSL_PI * centreFreq / mFrameRate);
			mBCoeff[0]	= 1 / (1 + (CSL_SQRT_TWO * C) + (C * C) );
			mBCoeff[1]	= -2 * mBCoeff[0];
			mBCoeff[2]	= mBCoeff[0];
			mACoeff[1]	= 2 * mBCoeff[0] * ((C * C) - 1);
			mACoeff[2]	= mBCoeff[0] * (1 - (CSL_SQRT_TWO * C) + (C * C) );
			break;
		case BW_BAND_PASS :
			C = 1 / (tanf (CSL_PI * bandwidth / mFrameRate) );
			D = 2 * cos (2 * CSL_PI * centreFreq / mFrameRate);
			mBCoeff[0]	= 1 / (1 + C);
			mBCoeff[1]	= 0;
			mBCoeff[2]	= -1 * mBCoeff[0];
			mACoeff[1]	= -1 * mBCoeff[0] * C * D;
			mACoeff[2]	= mBCoeff[0] * (C - 1);
			break;
		case BW_BAND_STOP :
			C = tanf (CSL_PI * bandwidth / mFrameRate);
			D = 2 * cos (2 * CSL_PI * centreFreq / mFrameRate);
			mBCoeff[0]	= 1 / (1 + C);
			mBCoeff[1]	= -1 * mBCoeff[0] * D;
			mBCoeff[2]	= mBCoeff[0];
			mACoeff[1]	= -1 * mBCoeff[0] * D;
			mACoeff[2]	= mBCoeff[0] * (1 - C);
			break;
	} // switch 
}


Formant::Formant (UnitGenerator & in, float cutoff, float radius) : Filter (in, 3,3) {
	mNormalize = true;
	setFrequency(cutoff);
	setAmount(radius);
	setupCoeffs();
	clear(); 
}

Formant::Formant (UnitGenerator & in, UnitGenerator & cutoff, float radius) : Filter (in, 3,3) {
	mNormalize = true;
	setFrequency(cutoff);
	setAmount(radius);
	setupCoeffs();
	clear(); 
}

void Formant::setNormalize(bool normalize) {
	mNormalize = normalize;
	setupCoeffs();
};

//	Calculate the filter coefficients based on the frequency characteristics
//	to be done every sample for dynamic controls

void Formant::setupCoeffs () {
	float centreFreq = mInputs[CSL_FILTER_FREQUENCY]->nextValue();
	float radius = mInputs[CSL_FILTER_AMOUNT]->nextValue();
	    
	mACoeff[0] = 1.0F;
	mACoeff[1] = cos(CSL_TWOPI * centreFreq * 1.f / mFrameRate ) * (-2.0F) * radius;
	mACoeff[2] = radius * radius;
	if (mNormalize ) {						// Use zeros at +- 1 and normalize the filter peak gain.
		mBCoeff[0] = 0.5 - 0.5 * mACoeff[2];
		mBCoeff[1] = 0.0;
		mBCoeff[2] = -mBCoeff[0];
	} else {
		mBCoeff[0] = 1.0F;
		mBCoeff[1] = 0.0F;
		mBCoeff[2] = -1.0F;
	} 
}

Notch::Notch (UnitGenerator & in, float cutoff, float radius) : Filter (in,3,3) {
	setFrequency(cutoff);
	setAmount(radius);
	clear(); 
}

Notch::Notch (UnitGenerator & in, UnitGenerator & cutoff, float radius) : Filter(in,3,3) {
	setFrequency(cutoff);
	setAmount(radius);
	clear();
}

//	Calculate the filter coefficients based on the frequency characteristics
void Notch::setupCoeffs () {
	float centreFreq = mInputs[CSL_FILTER_FREQUENCY]->nextValue();
	float radius = mInputs[CSL_FILTER_AMOUNT]->nextValue();
	
	//coeff's similar to formant but opposite
	mBCoeff[0] = 1.0F;
	mBCoeff[1] = cos(CSL_TWOPI * centreFreq * 1.0f / mFrameRate ) * (-2.0F) * radius;
	mBCoeff[2] = radius * radius;
	mACoeff[0] = 1.0F;
	mACoeff[1] = 0.0F;
	mACoeff[2] = 0.0F;
}

// the Frequency input of FrequencyAmount is used as the Allpass coefficient
Allpass::Allpass (UnitGenerator & in, float coeff) : Filter(in,2,2) {		// 1st order 1-pole 1-zero filter
	setFrequency(coeff);
	setAmount(1);
	clear();
}

Allpass::Allpass (UnitGenerator & in, UnitGenerator & coeff) : Filter(in,2,2) {
	setFrequency(coeff);
	setAmount(1);
	clear();
}


//	Calculate the filter coefficients based on supplied coeffs
void Allpass::setupCoeffs () {
	float coefficient = mInputs[CSL_FILTER_FREQUENCY]->nextValue();
	
	mACoeff[0] = mBCoeff[1] = 1.0;
	mBCoeff[0] = mACoeff[1] = coefficient; 
}

Moog::Moog (UnitGenerator & in) : Filter(in) {
	setFrequency(500.0);
	setAmount(0.99);
	y1 = y2 = y3 = y4 = oldy1 = oldy2 = oldy3 = x = oldx = 0.f;
}

Moog::Moog (UnitGenerator & in, UnitGenerator & cutoff) : Filter(in) {
	setFrequency(cutoff);
	setAmount(0.99);
	y1 = y2 = y3 = y4 = oldy1 = oldy2 = oldy3 = x = oldx = 0.f;
}

Moog::Moog (UnitGenerator & in, UnitGenerator & cutoff, UnitGenerator & resonance) : Filter(in) {
	setFrequency(cutoff);
	setAmount(resonance);
	y1 = y2 = y3 = y4 = oldy1 = oldy2 = oldy3 = x = oldx = 0.f;
}

Moog::Moog (UnitGenerator & in, float cutoff) : Filter(in) {
	setFrequency(cutoff);
	setAmount(0.5);
	y1 = y2 = y3 = y4 = oldy1 = oldy2 = oldy3 = x = oldx = 0.f;
}

Moog::Moog (UnitGenerator & in, float cutoff, float resonance) : Filter(in) {
	setFrequency(cutoff);
	setAmount(resonance);
	y1 = y2 = y3 = y4 = oldy1 = oldy2 = oldy3 = x = oldx = 0.f;
}

// Filter the next buffer of the input -- adapted from moog filter at music.dsp source code archive

void Moog::nextBuffer(Buffer & outputBuffer, unsigned outBufNum) throw (CException) {
#ifdef CSL_DEBUG
	logMsg("Moog Filter nextBuffer");
#endif	
	sample* out = outputBuffer.monoBuffer(outBufNum);	// get ptr to output channel
	unsigned numFrames = outputBuffer.mNumFrames;		// get buffer length
	SampleBuffer inputPtr = mInputs[CSL_INPUT]->mBuffer->mBuffers[outBufNum];
	DECLARE_SCALABLE_CONTROLS;							// declare the scale/offset buffers and values
	DECLARE_FILTER_CONTROLS;							// declare the freq/bw buffers and values
	LOAD_SCALABLE_CONTROLS;
	LOAD_FILTER_CONTROLS;
	this->setupCoeffs();
	
	for (unsigned i = 0; i < numFrames; i++) 	{
//		this->setupCoeffs();
				// --Inverted feed back for corner peaking 
		x = *inputPtr++ - r * y4; 
				// Four cascaded onepole filters (bilinear transform) 
		y1 = x * p + oldx * p - k * y1; 
		y2 = y1 * p + oldy1 * p - k * y2; 
		y3 = y2 * p + oldy2 * p - k * y3; 
		y4 = y3 * p + oldy3 * p - k * y4; 
				// Clipper band limited sigmoid 
		y4 = y4 - (y4*y4*y4) / 6; 
		oldx = x; 
		oldy1 = y1; 
		oldy2 = y2; 
		oldy3 = y3;
		*out++ = y4;
		UPDATE_SCALABLE_CONTROLS;							// update the dynamic scale/offset
	}
}

// Calculate the filter coefficients based on the frequency characteristics

void Moog::setupCoeffs () {
	float centreFreq = mInputs[CSL_FILTER_FREQUENCY]->nextValue();
	float resonance = mInputs[CSL_FILTER_AMOUNT]->nextValue();

	float f, scale;
	f = 2 * centreFreq / mFrameRate;				 //[0 - 1] 
	k = 3.6 * f - 1.6 * f * f - 1;		 //(Empirical tunning) 
	p =	(k + 1 ) * 0.5; 
	scale = exp((1 - p ) * 1.386249 ); 
	r = resonance * scale; 
}
