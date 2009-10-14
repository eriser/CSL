//
//	Test_Effects.cpp -- C main functions for the basic CSL effect and filter tests.
//	See the copyright notice and acknowledgment of authors in the file COPYRIGHT
//
// This program simply reads the run_tests() function (at the bottom of this file)
//  and executes a list of basic CSL tests
//
// Examples to add
//		chords
//		Microphone input
//		JUCE MIDI

#ifndef USE_JUCE
#define USE_TEST_MAIN			// use the main() function in test_support.h
#include "Test_Support.cpp"		// include all of CSL core and the test support functions
#else
#include "Test_Support.h"
#endif

#include "RingBuffer.h"			/// Utility circular buffer
#include "Clipper.h"
#include "FIR.h"
#include "InOut.h"

using namespace csl;	// this is the namespace, dummy!

/////////////////////// Here are the actual unit tests ////////////////////

/// Clip some sound

void testClipper() {
	Osc vox(110);								// We need some sound, so . . . let's get a sine.
//	ADSR vol(3.0, 0.3, 2.0, 0.2, 0.5);			// The amplitude envelope.
//	vox.setScale(vol);							// We set the envelope to affect the sine.
	Clipper clipIt(vox, -0.3, 0.3);				// To use clipper, just specify a min and max value.
	logMsg("playing clipped sin...");
	runTest(clipIt);
	logMsg("done.\n");
}

/// Test the FIR filter BP 200-300 Hz on pink noise

void testFIR() {
	PinkNoise noise;							// the sound source
	double resp[2] = { 0, 1 };					// amplitudes in the 2 freq bands (i.e., hi-pass)
	double freq[4] = { 0, 2000, 3000, 22050 };	// corner freqs of the pass, transition, and stop bands
	double weight[2] = { 10, 20 };				// weights for error (ripple) in the 2 bands
	FilterSpecification fs(64, 2, freq, resp, weight);	// 64 taps (64-step IR), 2 bands
	FIR vox(noise, fs);							// create the filter
	MulOp mul(vox, 10);							// scale it back up
	logMsg("playing FIR filtered noise...");
	runTest(mul);
	logMsg("FIR done.");	
}

/// Filter tests

void testFilters() {
	float dur = 2.f;						// seconds to play each test for
					// Butterworth high pass filter
	WhiteNoise white1(0.1);
	Butter butter(white1, BW_HIGH_PASS, 4000.0, 333.3);	// corresponds to Q of 3
	logMsg("playing Butterworth high-passed white noise...");
//	butter.dump();
	runTest(butter, dur);
	logMsg("done.");

					// Butterworth band pass filter
	WhiteNoise white2;
	Butter butter1(white2, BW_BAND_PASS, 1000.f, 100.f);
//	butter1.dump();
	logMsg("playing Butterworth band-passed white noise...");
//	butter1.dump();
	runTest(butter1, dur);
	logMsg("done.");
//	
					// Butterworth low pass filter
	WhiteNoise white3;
	Butter butter2(white3, BW_LOW_PASS, 250.f, 100.f);
	logMsg("playing Butterworth low-passed white noise...");
//	butter2.dump();
	runTest(butter2, dur);
	logMsg("done.");
//
					// Butterworth band-stop filter
	WhiteNoise white4(0.1);
	Butter butter3(white4, BW_BAND_STOP, 3000.f, 2500.f);
	logMsg("playing Butterworth band-rejected white noise...");
//	butter3.dump();
	runTest(butter3, dur);
	logMsg("done.");

//	if (true) return;
					// Notch filter
	WhiteNoise white5(0.3);
	Notch notch(white5, 500.f, 0.99995f);
	logMsg("playing Notch filtered white noise...");
//	notch.dump();
	runTest(notch, dur);
	logMsg("done.");
	
					// Formant filter
	WhiteNoise white6(0.3);
	Formant formant(white6, 100.f, 0.5f);
	logMsg("playing Formant filtered white noise...");
//	formant.dump();
	runTest(formant, dur);
	logMsg("done.");
	
					// Allpass filter
	Square osc(400,0.2);
	Allpass allpass(osc, 0.9995f);
	logMsg("playing All-passed square wave...");
	runTest(allpass, dur);
	logMsg("done.");

					// a hand-built band pass filter
	float bcoeffs[3] = {0.05f, 0.f, -0.0496f};
	float acoeffs[2] = {-1.843f, 0.9f};
	WhiteNoise white0;
	Filter filter(white0, bcoeffs ,acoeffs, 3, 2);
	logMsg("playing BP filtered white noise...");
	runTest(filter, dur);
	logMsg("done.");
	
					// Moog filter
//	WhiteNoise white8;
//	Moog moog(white8, 1000.f, 0.9f);
//	logMsg("playing Moog filtered white noise...");
//	runTest(moog, dur);
//	logMsg("done.");

}

/// Test dynamic BP filter

void testDynamicFilters() {
	float dur = 6.0f;								// seconds to play each test for
	WhiteNoise white(1.0);							// noise
	RandEnvelope center(3, 1000, 2000, 900);		// center/bw freq random walk
	RandEnvelope bw(3, 100, 160, 80);				// (frq, amp, offset)
	Butter butter(white, BW_BAND_PASS, center, bw);	// Butterworth BP filter
	logMsg("playing dynamic Butterworth band-passed white noise...");
//	butter.dump();
//	center.trigger();	
//	bw.trigger();	
	runTest(butter, dur);
	logMsg("done.");
}

/// Test dynamic BP filter on a sound file

void testDynamicVoice() {
	float dur = 6.0f;								// seconds to play each test for
	RandEnvelope center(4, 1400, 1800, 600);		// center/bw freq random walk
	RandEnvelope bw(5, 60, 160, 40);				// (frq, amp, offset, step)
	SoundFile sfile(CGestalt::dataFolder(), "sns.aiff", true);		// open a speak'n'spell file
	Butter butter(sfile, BW_BAND_PASS, center, bw);	// Butterworth BP filter
	logMsg("playing filtered snd file...");
	runTest(butter, dur);
	logMsg("done.");
}

/// Pan and mix many sines

void testManyDynamicFilters() {
	int num = 20;							// # of layers
	float scale = 3.0f / (float) num;		// ampl scale
	Mixer mix(2);							// stereo mixer
	for (int i = 0; i < num; i++) {			// loop to add a panning, LFO-controlled osc to the mix
											// (frq, amp, offset, step)
		RandEnvelope * center = new RandEnvelope(3, 1800, fRandM(2000, 4000), 200);
		RandEnvelope * bw = new RandEnvelope(3, 200, 400, 100);
		WhiteNoise * white = new WhiteNoise(1.0);		// noise
		Butter * butter = new Butter(*white, BW_BAND_PASS, *center, *bw);
		MulOp * mul = new MulOp(*butter, scale);
		Osc * lfo = new Osc(fRandM(0.3, 0.6), 1, 0, fRandM(0, CSL_PI));	// panning LFO with rand phase
		Panner * pan = new Panner(*mul, *lfo);
		mix.addInput(*pan);
	}
	logMsg("playing mix of %d Butterworth band-passed white noise layers...", num);
//	srand(getpid());						// seed the rand generator -- UNIX SPECIFIC CODE HERE
	runTest(mix);
	logMsg("done.\n");
	mix.deleteInputs();						// clean up
}

///  Play noise bursts into reverb

void testReverb() {
	ADSR mChiffEnv(1, 0.01, 0.01, 0.0, 1.5);	// attack-chiff envelope
	WhiteNoise mChiff;							// attack-chiff noise source
	Butter mChFilter(mChiff, BW_BAND_PASS, 2000.f, 500.f);	// and filter
	mChiffEnv.setScale(10);						// scale chiff envelope
	mChFilter.setScale(mChiffEnv);				// apply chiff envelope
	Freeverb mReverb(mChFilter);				// stereo reverb
	mReverb.setRoomSize(0.95);					// longer reverb

	mChiffEnv.trigger();	
	logMsg("playing Reverb test\n");
	theIO->setRoot(mReverb);					// make some sound
	for (unsigned i = 0; i < 4; i++) {
		mChiffEnv.trigger();	
		sleepSec(4);
	}
	sleepSec(6);
	theIO->clearRoot();	
	logMsg("done.\n");
}

///  Play noise bursts into reverb

void testStereoverb() {
	ADSR mEnv(1, 0.005, 0.01, 0.0, 1.5);		// attack-chiff envelope
	WhiteNoise mChiff;							// attack-chiff noise source
	float ctrFrq = fRandB(2000, 500);			// filter center freq
	Butter mFilter(mChiff, BW_BAND_PASS, ctrFrq, 500.0f);	// BP filter
	mEnv.setScale(10);							// scale chiff envelope
	mFilter.setScale(mEnv);						// apply chiff envelope
	
	Panner mPanner(mFilter, 0.0);				// stereo panner
	Stereoverb mReverb(mPanner);				// stereo reverb
	mReverb.setRoomSize(0.988);					// long reverb time
	
	theIO->setRoot(mReverb);					// start sound output
	
	srand(getpid());							// set rand seed (UNIX-specific)
	float nap = 1.0f;
	logMsg("playing Stereoverb test\n");
	for (unsigned i = 0; i < 10; i++) {			// play a loop of notes
		mPanner.setPosition(fRand1());			// select a random stereo position
		mEnv.trigger();							// trigger the burst envelope
		sleepSec(nap);							// sleep a few seconds
		nap *= 1.15f;							// slow down
		mFilter.setFrequency(fRandB(2000, 1000));	// pick a new frequency, 2k +- 1k
	}	
	sleepSec(1);								// sleep at the end to let it die out
	theIO->clearRoot();	
	logMsg("done.\n");
}

///  Play noise bursts into multi-tap delay line

void testMultiTap() {
	ADSR mChiffEnv(1, 0.01, 0.01, 0.0, 1.5);	// attack-chiff envelope
	WhiteNoise mChiff;							// attack-chiff noise source
	Butter mChFilter(mChiff, BW_BAND_PASS, 2000.f, 500.f);	// and filter
	mChiffEnv.setScale(10);						// scale chiff envelope
	mChFilter.setScale(mChiffEnv);				// apply chiff envelope
	
	RingBuffer rbuf(mChFilter, 1, 22050);		// mono, 1.2 sec.
	rbuf.mTap.setOffset(2000);
	RingBufferTap tap1(& rbuf, 4000);
	RingBufferTap tap2(& rbuf, 8000);
	RingBufferTap tap3(& rbuf, 12000);
	RingBufferTap tap4(& rbuf, 16000);
	RingBufferTap tap5(& rbuf, 20000);
	
	Mixer mix(2);								// create a stereo mixer
	mix.addInput(rbuf);							// add the taps to the mixer
	mix.addInput(tap1);
	mix.addInput(tap2);
	mix.addInput(tap3);
	mix.addInput(tap4);
	mix.addInput(tap5);

	mChiffEnv.trigger();	
	logMsg("");
	logMsg("playing multi-tap delay test\n");
	theIO->setRoot(mix);						// make some sound
	for (unsigned i = 0; i < 4; i++) {
		mChiffEnv.trigger();	
		sleepSec(1);
	}
//	sleepSec(3);
	theIO->clearRoot();	
	logMsg("done.\n");
}

/// Test a block resizer by running a random gliss with a small block size

void testBlockUpsizer() {
	float dur = 6.0f;							// seconds to play each test for
	Osc vox;									// declare an oscillator
	AR a_env(6, 1, 1);							// dur, att, rel
	RandEnvelope f_env(3, 80, 200, 40);			// freq env = random walk
	vox.setFrequency(f_env);					// set the carrier's frequency
	vox.setScale(a_env);						// multiply index envelope by mod freq
//	f_env.trigger();							// reset the envelopes to time 0
	a_env.trigger();	
	BlockResizer blocker(vox, 300);				// small buffer, not a divisor of CSL's block size
	logMsg("playing random gliss in a block up-sizer...");
	runTest(blocker, dur);						// run test
	logMsg("done.");
}

/// Test a block resizer by running a random gliss with a huge block size

void testBlockDownsizer() {
	float dur = 6.0f;							// seconds to play each test for
	Osc vox;									// declare an oscillator
	AR a_env(6, 1, 1);							// dur, att, rel
	RandEnvelope f_env(3, 80, 200, 40);			// freq env = random walk
	vox.setFrequency(f_env);					// set the carrier's frequency
	vox.setScale(a_env);						// multiply index envelope by mod freq
//	f_env.trigger();							// reset the envelopes to time 0
	a_env.trigger();	
	BlockResizer blocker(vox, 1100);			// large buffer, not a multiple of CSL's block size
	logMsg("playing random gliss in a block down-sizer...");
	runTest(blocker, dur);						// run test
	logMsg("done.");
}


// Simple sample average filter class - an example of a custom UnitGenerator definition
// This implements nextBuffer() with its own DSP routine: a scaled past-sample averager (lo-pass filter)

class SAFliter : public Effect {				// It's an Effect/UnitGenerator
public:											// created with an input UGen &scale coeff
	SAFliter(UnitGenerator & in, float coeff = 0.5f) : Effect(in), mCoeff(coeff), mStore(0) { };
	~SAFliter() { };
													// nextBuffer() gets input and operates on it
	void nextBuffer(Buffer & outputBuffer, unsigned outBufNum) throw (CException) {
		unsigned numFrames = outputBuffer.mNumFrames;			// get buffer length
		SampleBuffer out = outputBuffer.monoBuffer(outBufNum);	// get ptr to output channel
		
		Effect::pullInput(numFrames);				// get some input
		SampleBuffer inPtr = mInputPtr;				// get a pointer to the input samples
		
		float val;
		for (unsigned i = 0; i < numFrames; i++) {	// here's the filter loop
			val = *inPtr++ * mCoeff;				// get next input sample, scale
			*out++ = val + mStore;					// put current output in the buffer and increment
			mStore = val;							// store val
		}
	}
protected:											// class' data members
	float mCoeff, mStore;							// scale coeff & past value
};													// end of class SAFliter

// test it

void testSAFilter() {
	SoundFile sfile(CGestalt::dataFolder(), "sns.aiff", true);	// open a speak'n'spell file
	
	SAFliter averager(sfile);							// s-a filter
	logMsg("playing filtered snd file...");
	runTest(averager, 5.0);
	logMsg("done.");
}

//////// RUN_TESTS Function ////////

#ifndef USE_JUCE

void runTests() {
//	testClipper();					// Simple clipper tests
//	testFIR();						// Noise filtering test
//	testFilters();
//	testDynamicFilters();
//	testReverb();
	testStereoverb();
//	testMultiTap();
}

#else

// test list for Juce GUI

testStruct effTestList[] = {
	"Clipper",				testClipper,
	"FIR filter",			testFIR,
	"All filters",			testFilters,
	"Dynamic filter",		testDynamicFilters,
	"Filtered snd file",	testDynamicVoice,
	"Many dynamic filters",	testManyDynamicFilters,
	"Reverb",				testReverb,
	"Stereo-verb",			testStereoverb,
	"Multi-tap",			testMultiTap,
	"Block up-sizer",		testBlockUpsizer,
	"Block down-sizer",		testBlockDownsizer,
	"Sample-avg filter",	testSAFilter,
	NULL,					NULL
};

#endif
