//
//	Mixer.cpp -- The multi-channel panner and mixer classes.
//	See the copyright notice and acknowledgment of authors in the file COPYRIGHT
//

#include "Mixer.h"
#include <stdlib.h>
#include <math.h>
#include <stdio.h>

using namespace csl;

//// Mixer class implementation

// Constructors

Mixer::Mixer() : UnitGenerator(), Scalable(1, 0) {
	mNumChannels = 1;
	allocateOpBuffer(1);			// assume mono input
}

Mixer::Mixer(unsigned chans) : UnitGenerator(), Scalable(1, 0) {
	allocateOpBuffer(chans);
}

Mixer::Mixer(UnitGenerator & aa) : UnitGenerator(), Scalable(aa, 0) {
	allocateOpBuffer(1);
}

Mixer::Mixer(unsigned chans, UnitGenerator & aa) : UnitGenerator(), Scalable(aa, 0) {
	allocateOpBuffer(chans);
}

Mixer::~Mixer() {
	mOpBuffer.freeBuffers();
}

// add a new input to the list

void Mixer::addInput(UnitGenerator & inp) {
	 mSources.push_back(& inp);
}

void Mixer::addInput(UnitGenerator * inp) {
	 mSources.push_back(inp);
}

// delete from the list

void Mixer::removeInput(UnitGenerator & inp) {
	for (UGenVector::iterator pos = mSources.begin(); pos != mSources.end(); ++pos) {
		if (*pos == & inp) {
			mSources.erase(pos);
			return;
		}
	}
}

// free all the inputs

void Mixer::deleteInputs() {
	for (UGenVector::iterator pos = mSources.begin(); pos != mSources.end(); ++pos)
		delete * pos;
}

// Allocate an input operation buffer

void Mixer::allocateOpBuffer(unsigned chans) {
	mNumChannels = chans;
	mOpBuffer.setSize(chans, CGestalt::blockSize());
	mOpBuffer.allocateBuffers();
}

// Count active sources

unsigned Mixer::activeSources() {
	unsigned answer = 0;
	for (UGenVector::iterator pos = mSources.begin(); pos != mSources.end(); ++pos) {
		UnitGenerator * input = *pos;
		if (input->isActive()) 
			answer++;
	}
	return answer;
}

// Here's the mixing part -- fill the buffer with the next num_frames values

void Mixer::nextBuffer(Buffer & outputBuffer) throw (CException) {
	SampleBuffer out1, out2, opp;
	unsigned numFrames = outputBuffer.mNumFrames;
	unsigned j, k, ich;

//	printf("mix: next_b %d\n", numFrames);
	outputBuffer.zeroBuffers();		// clear output buffer
	mOpBuffer.mNumFrames = numFrames;
	for (UGenVector::iterator pos = mSources.begin(); pos != mSources.end(); ++pos) {
		UnitGenerator * input = *pos;
//		printf("%d=%x  ", i, input); fflush(stdout);
		if (input->isActive()) {						// if active input, get a buffer and sum it to output
			ich = input->numChannels();
//			printf("mix: next_b %d, inp %x active, %d ch\n", numFrames, input, ich);

			mOpBuffer.mNumChannels = ich;
			input->nextBuffer(mOpBuffer);				// get the input's nextBuffer
			
			if (ich == mNumChannels) {					// if input and mixer have same # of channels
				for (j = 0; j < ich; j++)	{			// j loops through channels
					out1 = outputBuffer.mBuffers[j];
					opp = mOpBuffer.mBuffers[j];
					if ((out1 == 0) || (opp == 0))
						return;
					for (k = 0; k < numFrames; k++)		// k loops through sample buffers
						*out1++ += *opp++;				// summing samples into the output buffer
				}
			}											// special case: mix mono to stereo
			else if ((ich == 1) && (mNumChannels == 2)) {
				out1 = outputBuffer.mBuffers[0];
				out2 = outputBuffer.mBuffers[1];
				opp = mOpBuffer.mBuffers[0];
				for (k = 0; k < numFrames; k++) {		// k loops through sample buffers
					*out1++ += *opp;					// sum mono-to-stereo
					*out2++ += *opp++;
				}
			} else {									// ??? -- channel # mismatch
				logMsg(kLogError, "Error in mix: in = %d ch, out = %d ch\n", ich, mNumChannels);
			}
		}
	}						// end of input loop, process scale/offset
#ifdef DO_MIX_AS_SCALABLE
	DECLARE_SCALABLE_CONTROLS;
	LOAD_SCALABLE_CONTROLS;
	sample samp;			// loop through output buffer per-channel applying scale/offset
	for (j = 0; j < mNumChannels; j++) {
		out1 = outputBuffer.mBuffers[j];
		for (k = 0; k < numFrames; k++)	 {
			samp = *out1;
			samp = (samp * scaleValue) + offsetValue;
			*out1++ = samp;
			UPDATE_SCALABLE_CONTROLS;
		}
		scalePort->resetPtr();
		offsetPort->resetPtr();
		scaleValue = scalePort->nextValue();
		offsetValue = offsetPort->nextValue();
	}
#endif
}

// print info about this instance

void Mixer::dump() {
	fprintf(stderr, "Mixer -- %d inputs: ", (int) mSources.size());
	for (unsigned i = 0; i < mSources.size(); i++) {
		mSources[i]->dump();
		fprintf(stderr, "\n");
	}
}

// Panner methods

Panner::Panner() : Effect(), Scalable() {
	mNumChannels = 2;
};

Panner::Panner(UnitGenerator &input) : Effect(input), Scalable(1.0, 0.0) {
	mNumChannels = 2;
	this->addInput(CSL_POSITION, 0.0);
}

Panner::Panner(UnitGenerator &input, UnitGenerator &position) : Effect(input), Scalable(1.0, 0.0) {
	mNumChannels = 2;
	this->addInput(CSL_POSITION, position);
}

Panner::Panner(UnitGenerator &input, UnitGenerator &position, UnitGenerator & amp) : Effect(input), Scalable(amp, 0.0) {
	mNumChannels = 2;
	this->addInput(CSL_POSITION, position);
}

Panner::Panner(UnitGenerator &input, UnitGenerator &position, float amp) : Effect(input), Scalable(amp, 0.0) {
	mNumChannels = 2;
	this->addInput(CSL_POSITION, position);
}

Panner::Panner(UnitGenerator &input, float position) : Effect(input), Scalable(1.0, 0.0) {
	mNumChannels = 2;
	this->addInput(CSL_POSITION, position);
}

Panner::Panner(UnitGenerator &input, float position, float amp) : Effect(input), Scalable(amp, 0.0) {
	mNumChannels = 2;
	this->addInput(CSL_POSITION, position);
}

Panner::~Panner(void) {
}

void Panner::setPosition(UnitGenerator &pan) {
	this->addInput(CSL_POSITION, pan);
}

void Panner::setPosition(float pan) {
	this->addInput(CSL_POSITION, pan);
}

// To get my next buffer, get a buffer from the input, and then "process" it...

void Panner::nextBuffer(Buffer &outputBuffer) throw (CException) {
	SampleBuffer out1 = outputBuffer.mBuffers[0];
	SampleBuffer out2 = outputBuffer.mBuffers[1];
	unsigned numFrames = outputBuffer.mNumFrames;

	if (outputBuffer.mNumChannels != 2)
		logMsg(kLogError, "Panner output ch = %d", outputBuffer.mNumChannels);

	DECLARE_SCALABLE_CONTROLS;						// declare the scale/offset buffers and values
	LOAD_SCALABLE_CONTROLS;

	Effect::pullInput(numFrames);					// get the input samples via Effect
	SampleBuffer inpp = mInputPtr;

	Port * posPort = mInputs[CSL_POSITION];
	Controllable::pullInput(posPort, numFrames);				// get the position UGen's data
	float posValue = posPort->nextValue() * 0.5;				// get and scale the first position value
	
	for (unsigned i = 0; i < numFrames; i++) {
		*out1++ = (*inpp) * (0.5 - posValue) * scaleValue;		// L sample
		*out2++ = (*inpp++) * (posValue + 0.5) * scaleValue;	// R sample
		UPDATE_SCALABLE_CONTROLS;								// update the dynamic scale/offset
		posValue = (posPort->nextValue()) * 0.5;
	}
}


// NtoMPanner -- Many variations on the constructor

NtoMPanner::NtoMPanner(UnitGenerator & i, float a, unsigned in_c, unsigned out_c) : Panner(i, a) {
	this->addInput(CSL_POSITIONX, 0.0);
	this->addInput(CSL_POSITIONY, 0.0);
	mInCh = in_c;
	mOutCh = out_c;
	mSpread = CSL_PI / 2.0;
	initSpeakers();
}

NtoMPanner::NtoMPanner(UnitGenerator & i, UnitGenerator & pX, UnitGenerator & pY, UnitGenerator & a,
		unsigned in_c, unsigned out_c)
				: Panner(i, pX, a) {
	mInCh = in_c;
	mOutCh = out_c;
	this->addInput(CSL_POSITIONX, pX);
	this->addInput(CSL_POSITIONY, pY);
	mSpread = CSL_PI / 2.0;
	initSpeakers();
}

NtoMPanner::NtoMPanner(UnitGenerator & i, UnitGenerator & pX, UnitGenerator & pY, UnitGenerator & a,
		unsigned in_c, unsigned out_c, float spr)
				: Panner(i, pX, a) {
	this->addInput(CSL_POSITIONX, pX);
	this->addInput(CSL_POSITIONY, pY);
	mInCh = in_c;
	mOutCh = out_c;
	mSpread = spr;
	initSpeakers();
}

NtoMPanner::NtoMPanner(UnitGenerator & i, UnitGenerator & pX, UnitGenerator & pY, float a,
		unsigned in_c, unsigned out_c, float spr)
				: Panner(i, pX, a) {
	this->addInput(CSL_POSITIONX, pX);
	this->addInput(CSL_POSITIONY, pY);
	mInCh = in_c;
	mOutCh = out_c;
	mSpread = spr;
	initSpeakers();
}

NtoMPanner::~NtoMPanner() { /* _op_buffer.freeBuffers(); */ }

void NtoMPanner::setX(float pan) {
	this->addInput(CSL_POSITIONX, 0.0);
}

void NtoMPanner::setY(float pan) {
	this->addInput(CSL_POSITIONY, 0.0);
}

// To get my next buffer, get a bufffer from the input, and then mix it to each of the outputs

void NtoMPanner::nextBuffer(Buffer &outputBuffer) throw (CException) {
	unsigned numFrames = outputBuffer.mNumFrames;
	float a_scale, dist, l_samp, r_samp, o_samp;
	CPoint l_pos, r_pos;
#ifdef CSL_WINDOWS
	float l_weights[MAX_OUTPUTS], r_weights[MAX_OUTPUTS];
#else
	float l_weights[mOutCh], r_weights[mOutCh];
#endif
										// Get an n-channel buffer from the input signal
	DECLARE_SCALABLE_CONTROLS;					// declare the scale/offset buffers and values
	LOAD_SCALABLE_CONTROLS;

	Effect::pullInput(numFrames);					// get the input samples via Effect

	Port * posXPort = mInputs[CSL_POSITIONX];
	Controllable::pullInput(posXPort, numFrames);	// get the position UGen's data
	float posXValue = posXPort->nextValue();			// get and scale the first position value
	Port * posYPort = mInputs[CSL_POSITIONY];
	Controllable::pullInput(posYPort, numFrames);	// get the position UGen's data
	float posYValue = posYPort->nextValue();			// get and scale the first position value

	l_pos.set(posXValue, posYValue);					// set current source positions
	r_pos.set(posXValue, posYValue);
	r_pos.rotateBy(mSpread);

	for (unsigned j = 0; j < mOutCh; j++) {			// calculate the weights per-speaker
		dist = mSpeakers[j]->distance(& l_pos);			// distance between j'th speaker and source position
		if (dist < 1.0) dist = 1.0;						// limit to 1.0 (ft)
		l_weights[j] = 1.0 / dist;				// (dist * dist);	// inverse-square law
		if (mInCh > 1) {
			dist = mSpeakers[j]->distance(& r_pos);
			if (dist < 1.0) dist = 1.0;
			r_weights[j] = 1.0 / dist; //(dist * dist);
		}
	}
//	printf("L: %6.4f %6.4f %6.4f %6.4f\n", l_weights[0], l_weights[1], l_weights[2], l_weights[3]);
//	printf("R: %6.4f %6.4f %6.4f %6.4f\n", r_weights[0], r_weights[1], r_weights[2], r_weights[3]);
//	printf("L: %6.4f %6.4f\n", l_weights[0], l_weights[1]);
//	printf("R: %6.4f %6.4f\n", r_weights[0], r_weights[1]);

//	for (unsigned i = 0; i < numFrames; i++) {
//		*out1++ = (*inpp) * (0.5 - posValue) * scaleValue;			// L sample
//		*out2++ = (*inpp++) * (posValue + 0.5) * scaleValue;		// R sample
//		UPDATE_SCALABLE_CONTROLS;							// update the dynamic scale/offset
//		posValue = (posPort->nextValue()) * 0.5;
//	}

	SampleBuffer inL = mInputPtr;
	SampleBuffer inR = NULL;
	if (mInCh > 1)
		inR = mInputs[CSL_INPUT]->mBuffer->mBuffers[1];
//	outputBuffer.zeroBuffers();
	for (unsigned i = 0; i < numFrames; i++) {			// now do the output loop -- per frame
		l_samp = *inL++;								// get input sample(s)
		if (mInCh > 1)
			r_samp = *inR++;
		for (unsigned j = 0; j < mOutCh; j++) {		// generate output per channel
			o_samp = l_samp * a_scale * l_weights[j];	// scale input sample
			if (mInCh > 1)								// right channel if stereo
				o_samp += r_samp * a_scale * r_weights[j];
			outputBuffer.mBuffers[j][i] = o_samp;   // write output sample
		}
	}
}

// Speaker array init methods

void NtoMPanner::initSpeakers() {
	switch(mOutCh) {
		case 2:
			init_stereo(10.0);		break;
		case 4:
			init_quad(10.0);		break;
		case 5:
			init_5point1(10.0);		break;
		case 6:
			init_6ch(15.0, 20.0);	break;
		default:
			printf("ERROR: unknown # of channels in NtoM_Panner: %d\n", mOutCh);
	}
}

// Stereo -- assume 45 degree separation

void NtoMPanner::init_stereo(float dist) {
	float coord = dist * 0.7071;
	mSpeakers = (CPoint **) malloc(2 * sizeof(void *));
	mSpeakers[0] = new CPoint((0.0f - coord), coord);
	mSpeakers[1] = new CPoint(coord, coord);
}

// Square quad

void NtoMPanner::init_quad(float dist) {
	float coord = dist * 0.7071;
	mSpeakers = (CPoint **) malloc(4 * sizeof(void *));
	mSpeakers[0] = new CPoint((0.0f - coord), coord);
	mSpeakers[1] = new CPoint(coord, coord);
	mSpeakers[2] = new CPoint(coord, (0.0f - coord));
	mSpeakers[3] = new CPoint((0.0f - coord), (0.0f - coord));
}

// ITU standard 5.1-channel layout

void NtoMPanner::init_5point1(float dist) {
	mSpeakers = (CPoint **) malloc(5 * sizeof(void *));
	mSpeakers[0] = new CPoint(kPolar, dist, (float)(2.0 / 3.0 / CSL_PI));
	mSpeakers[1] = new CPoint(kPolar, dist, (float)(1.0 / 2.0 / CSL_PI));
	mSpeakers[2] = new CPoint(kPolar, dist, (float)(1.0 / 3.0 / CSL_PI));
	mSpeakers[3] = new CPoint(kPolar, dist, (float)(17.0 / 9.0 / CSL_PI));
	mSpeakers[4] = new CPoint(kPolar, dist, (float)(10.0 / 9.0 / CSL_PI));
}

// X-by-Y 6-channels along the sides (0 = LF, 2 = RF, 3 = RC, ...)

void NtoMPanner::init_6ch(float x, float y) {
	mSpeakers = (CPoint **) malloc(6 * sizeof(void *));
	mSpeakers[0] = new CPoint((0.0f - x), y);
	mSpeakers[1] = new CPoint(x, y);
	mSpeakers[2] = new CPoint(x, 0.0f);
	mSpeakers[3] = new CPoint(x, (0.0f - y));
	mSpeakers[4] = new CPoint((0.0f - x), (0.0f - y));
	mSpeakers[5] = new CPoint((0.0f - x), 0.0f);
}

// StereoWidth implementation

StereoWidth::StereoWidth() : Effect(), mWidth(0.0f), mGain(1.0f), mPan(0.5f) { }

StereoWidth::~StereoWidth() { }

void StereoWidth::nextBuffer(Buffer & outputBuffer) throw (CException) {
	Effect::pullInput(outputBuffer);

	// we'll process in place in the output buffer
	SampleBuffer outL = outputBuffer.mBuffers[0];
	SampleBuffer outR = outputBuffer.mBuffers[1];

	float gain = mGain;
	// equal power panning -- of sorts
	float panL = sqrt(2*(1.0 - mPan));
	float panR = sqrt(2*(mPan));

	float widthFactor = mWidth;
	if ((fabs(1.0 - panL) < 0.0001) && (fabs(1.0 - panR) < 0.0001) && (fabs(widthFactor) < 0.0001) && (fabs(1.0 - gain) < 0.0001)) {
		// nothing to do
		return;
	}
#ifdef DEBUG
	static int count = 0;
	if (count < 1) {
		printf("Pan L/R %f/%f \tWidth Factor/Gain %f/%f\n", panL, panR, widthFactor, widthGainFactor);
		count = 100;
	} else count--;
#endif
	unsigned numFrames = outputBuffer.mNumFrames;
	float newL, newR;
	for (unsigned i = 0; i < numFrames; i++) {
		newL = *outL + (*outR * widthFactor);
		newR = *outR + (*outL * widthFactor);
		*outL++ = newL * panL * gain;
		*outR++ = newR * panR * gain;
	}
	return;
}
