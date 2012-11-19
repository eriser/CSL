//
//	Test_Panners.cpp -- C main functions for the basic CSL panner and mixer tests.
//	See the copyright notice and acknowledgment of authors in the file COPYRIGHT
//
// This program simply reads the run_tests() function (at the bottom of this file)
//  and executes a list of basic CSL tests
//

#ifdef USE_JUCE
	#include "Test_Support.h"
#else
	#define USE_TEST_MAIN			// use the main() function in test_support.h
	#include "Test_Support.cpp"		// include all of CSL core and the test support functions
#endif

//#define USE_CONVOLVER

#ifdef USE_CONVOLVER
#include "Convolver.h"				// FFT-based convolver
#endif

/////////////////////// Here are the actual unit tests ////////////////////

/// Pan a sine to stereo

void testPan() {
	Osc vox(440);							// src sine 
	Osc lfo(0.2, 1, 0, CSL_PI);				// position LFO
	vox.setScale(0.3);
	Panner pan(vox, lfo);					// panner
	logMsg("playing a panning sine...");
	runTest(pan, 8);
	logMsg("done.\n");
}

/// Pan a sine to "N" channels

void testN2MPan() {
	Osc vox(440);
	Osc lfoX(0.2, 1, 0, CSL_PI);
	Osc lfoY(0.2, 1, 0, 0);
	vox.setScale(0.3);
			//	NtoMPanner(UnitGenerator & i, UnitGenerator & pX, UnitGenerator & pY, float a, unsigned in_c, unsigned out_c, float spr);
	NtoMPanner pan(vox, lfoX, lfoY, 1, 1, 2, 2);					// a panner
	logMsg("playing N-channel panning sine...");
	runTest(pan, 5);
	logMsg("done.\n");
}

/// Test mixer -- really slow additive synthesis

void testSineMixer() {
	Osc vox1(431, 0.3);				// create 4 scaled sine waves
	Osc vox2(540, 0.1);
	Osc vox3(890, 0.03);
	Osc vox4(1280, 0.01);
//	AR vol(5, 1, 1);				// amplitude envelope
//	Mixer mix(2, vol);				// stereo mixer with envelope
	Mixer mix(2);					// create a stereo mixer
	mix.addInput(vox1);				// add the sines to the mixer
	mix.addInput(vox2);
	mix.addInput(vox3);
	mix.addInput(vox4);
	logMsg("playing mix of 4 sines...");
	runTest(mix);
	logMsg("mix done.\n");
}

/// Pan and mix 2 sines

void testPanMix() {
	Osc vox1, vox2;							// sources oscs
	RandEnvelope env1(0.5, 80, 180);		// random freq envelopes
	RandEnvelope env2(0.5, 80, 180);		// (frq, amp, offset, step)
	vox1.setFrequency(env1);				// set osc freq and scale
	vox1.setScale(0.3);
	vox2.setFrequency(env2);
	vox2.setScale(0.3);
	Osc lfo1(0.3), lfo2(0.5, 1, 0, CSL_PI);// position LFOs (out of phase)
	Panner pan1(vox1, lfo1);				// 2 panners
	Panner pan2(vox2, lfo2);
	Mixer mix(2);							// stereo mixer
	mix.addInput(pan1);
	mix.addInput(pan2);
	logMsg("playing mix of panning sins...");
#ifndef CSL_WINDOWS
	srand(getpid());						// seed the rand generator -- UNIX SPECIFIC CODE HERE
#endif
	runTest(mix, 15);
	logMsg("done.\n");
}

/// Pan and mix many sines

void testBigPanMix() {
	int num = 64;							// # of layers
	float scale = 3.0f / (float) num;		// ampl scale
	Mixer mix(2);							// stereo mixer
	for (int i = 0; i < num; i++) {			// loop to add a panning, LFO-controlled osc to the mix
		Osc * vox = new Osc();
		RandEnvelope * env = new RandEnvelope(0.5, 80, 180);
		vox->setFrequency(*env);
		vox->setScale(scale);
		Osc * lfo = new Osc(fRandM(0.5, 0.9), 1, 0, fRandM(0, CSL_PI));
		Panner * pan = new Panner(*vox, *lfo);
		mix.addInput(*pan);
	}
	logMsg("playing mix of %d panning sins...", num);
#ifndef CSL_WINDOWS
	srand(getpid());						// seed the rand generator -- UNIX SPECIFIC CODE HERE
#endif
	runTest(mix, 60);
	logMsg("done.\n");
	mix.deleteInputs();						// clean up
}

/// Make a bank or 50 sines with random walk panners and glissandi

void testOscBank() {
	Mixer mix(2);								// stereo mixer
	for (unsigned i = 0; i < 50; i++) {			// RandEnvelope(float frequency, float amplitude, float offset, float step);
		RandEnvelope * pos = new RandEnvelope;
		RandEnvelope * freq = new RandEnvelope(0.8, 200, 300, 20);
//		Sine * vox = new Sine(110.0, 0.01);		// use computed sine
		Osc * vox = new Osc(110.0, 0.01);		// use stored look-up table
		vox->setFrequency(* freq);
		Panner * pan = new Panner(* vox, * pos);
		mix.addInput(pan);
	}
	logMsg("playing mix of panning sines...");
#ifndef CSL_WINDOWS
	srand(getpid());							// seed the rand generator so we get different results -- UNIX SPECIFIC CODE HERE
#endif
	runTest(mix, 60);
	logMsg("done.\n");
	mix.deleteInputs();							// clean up
}

/// swap sound channels with a ChannelMappedBuffer InOut
/// Constructor: InOut(input, inChan, outChan,  [ch-1 ... ch-outChan]);

#include "InOut.h"			/// Copy in-out plug

void testCMapIO() {
	Osc osc(220);								// carrier to be enveloped
	AR a_env(2, 0.6, 1.0);						// AR constructor dur, att, rel
	osc.setScale(a_env);						// amplitude modulate the sine
	Panner pan(osc, 0.0);						// panner
	InOut chanMap(pan, 2, 2, kNoProc, 2, 1);	// swap stereo channels
	logMsg("playing swapped sin...");
	runTest(chanMap);
	logMsg("done.\n");
}

////////// Convolution

#ifdef USE_CONVOLVER

void testConvolver() {
	JSoundFile fi(CGestalt::dataFolder(), "rim3_L.aiff");
	try {
		fi.openForRead();
	} catch (CException) {
		logMsg(kLogError, "Cannot read sound file...");
		return;
	}
	logMsg("playing sound file...");
	fi.trigger();
	runTest(fi);
	logMsg("sound file player done.\n");
					// Create convolver; FFT size will be block size * 2
	Convolver cv(CGestalt::dataFolder(), "Quadraverb_large_L.aiff");	
	cv.setInput(fi);
	logMsg("playing convolver...");
	fi.trigger();
	runTest(cv);
	fi.trigger();
	runTest(cv);
	fi.close();
	logMsg("convolver done.\n");
}

#define IRLEN (44100 * 4)			// 4 sec. IR

void testConvolver2() {
	WhiteNoise nois;				// amplitude, offset
	AR env(0.05, 0.0001, 0.049);	// AR constructor dur, att, rel
	nois.setScale(env);				// amplitude modulate the sine
	
	logMsg("playing noise burst...");
	env.trigger();
	runTest(nois);
	logMsg("done.\n");
									// make a simple IR with a few echoes
	Buffer buf(1, IRLEN);
	buf.allocateBuffers();
	float * samp = buf.monoBuffer(0);
	for (unsigned i = 0; i < IRLEN; i += 5000)
		samp[i] = 1 / (1 + (sqrt(i) / 5000));
									// Create convolver; FFT size will be block size * 2
	Convolver cv(buf);
	cv.setInput(nois);
	logMsg("playing convolver...");
	env.trigger();
	runTest(cv);
	env.trigger();
	runTest(cv);
	logMsg("convolver done.\n");
}

void testConvolver3() {
	WhiteNoise nois;				// amplitude, offset
	AR env(0.05, 0.0001, 0.049);	// AR constructor dur, att, rel
	nois.setScale(env);				// amplitude modulate the sine
	
	logMsg("playing noise burst...");
	env.trigger();
	runTest(nois);
	logMsg("done.\n");
	Convolver cv(CGestalt::dataFolder(), "3.3s_LargeCathedral_mono.aiff");	
	cv.setInput(nois);
	logMsg("playing convolver...");
	env.trigger();
	runTest(cv);
	env.trigger();
	runTest(nois);
	env.trigger();
	runTest(cv);
	logMsg("convolver done.\n");
}

#endif

#ifndef CSL_WINDOWS

/// Spatializer with HRTF

#include "SpatialAudio.h"
#include "Binaural.h"

/// Repeat a short test file moving in circles around the horizontal plane

void test_Binaural_horiz() {
				// Open a mono soundfile
	SoundFile sndfile(CGestalt::dataFolder() + "splash_mono.aiff");
	sndfile.dump();

	char folder[CSL_NAME_LEN];						// create HRTF data location
	strcpy(folder, CGestalt::dataFolder().c_str());	// CSL data folder location
	strcat(folder, "IRCAM_HRTF/512_DB/HRTF_1047.dat");	// HRTF data location
	HRTFDatabase::Reload(folder);					// Load the HRTF data
	HRTFDatabase::Database()->dump();

				// make the sound "Positionable"
	SpatialSource source(sndfile);
				// Create a spatializer.
	Spatializer panner(kBinaural);
				// Add the sound source to it
	panner.addSource(source);
				// loop to play transpositions
	logMsg("playing HRTF-spatialized rotating sound source (horizontal plane)...");
	theIO->setRoot(panner);							// make some sound

	for (int i = 0; i < 30; i++) {
		source.setPosition('s', (float) (i * 24), 0.0f, 2.0f);	// rotate in small steps
		source.dump();
		sndfile.trigger();
		sleepSec(0.8);
	}
	theIO->clearRoot();
	logMsg("done.");
}

/// Repeat a short test file moving in circles around the vertical plane at AZ = CSL_PIHALF 
/// (axial plane in line with your ears, easy to localize)

void test_Binaural_vertAxial() {
				// Open a mono soundfile
	SoundFile sndfile(CGestalt::dataFolder() + "guanno_mono.aiff");
	sndfile.dump();
				// make the sound "Positionable"
	SpatialSource source(sndfile);
				// Create a spatializer.
	Spatializer panner(kBinaural);
				// Add the sound source to it
	panner.addSource(source);
				// loop to play transpositions
	logMsg("playing HRTF-spatialized rotating sound source (vertical plane)...");
	theIO->setRoot(panner);							// make some sound
	for (int i = 30; i > 6; i--) {
		source.setPosition('s', CSL_PIHALF, (float) (i * 15), 2.0f);	// rotate in small steps
		source.dump();
		sndfile.trigger();
		sleepSec(0.8);
	}
	theIO->clearRoot();
	logMsg("done.");
}

/// Repeat a short test file moving in circles around the vertical plane at AZ = 0 
/// (median plane between your ears, hard to localize)

void test_Binaural_vertMedian() {
				// Open a mono soundfile
	SoundFile sndfile(CGestalt::dataFolder() + "triangle_mono.aiff");
	sndfile.openForRead();
	sndfile.dump();
	sndfile.setToEnd();
				// make the sound "Positionable"
	SpatialSource source(sndfile);
				// Create a spatializer.
	Spatializer panner(kBinaural);
				// Add the sound source to it
	panner.addSource(source);
				// loop to play transpositions
	logMsg("playing HRTF-spatialized rotating sound source (medial plane)...");
	theIO->setRoot(panner);							// make some sound
	for (int i = 30; i > 6; i--) {
		source.setPosition('s', 0.0f, (float) (i * 15), 2.0f);	// rotate in small steps
		source.dump();
		sndfile.trigger();
		sleepSec(0.8);
	}
	theIO->clearRoot();
	logMsg("done.");
}

/// Spatializer with Ambisonics

void test_Ambi_horiz() {
				// Open a mono soundfile
	SoundFile sndfile(CGestalt::dataFolder() + "triangle_mono.aiff");
	sndfile.dump();
					// make the sound "Positionable"
	SpatialSource source(sndfile);
				// Create a spatializer.
	Spatializer panner(kAmbisonic);
				// Add the sound source to it
	panner.addSource(source);
				// loop to play transpositions
	logMsg("playing Ambisonic-spatialized rotating sound source (horizontal plane)...");
	theIO->setRoot(panner);					// make some sound
	for (int i = 0; i < 30; i++) {
		source.setPosition('s', (float) (i * 24), 0.0f, 2.0f);	// rotate in small steps
		source.dump();
		sndfile.trigger();
		sleepSec(0.8);
	}
	theIO->clearRoot();
	logMsg("done.");
}

/// Spatializer with simple panners & filters and/or reverb

void test_SimpleP() {
				// Open a mono soundfile
	SoundFile sndfile(CGestalt::dataFolder() + "Piano_A5_mf_mono.aiff");
	sndfile.dump();
				// make the sound "Positionable"
	SpatialSource source(sndfile);
				// Create a "simple" spatializer.
	Spatializer panner(kSimple);
				// Add the sound source to it
	panner.addSource(source);
				// loop to play transpositions
	logMsg("playing simply spatialized rotating sound source...");
	Mixer mix(2);
	mix.addInput(panner);
	theIO->setRoot(mix);					// make some sound
	for (int i = 0; i < 30; i++) {
				// rotate in small steps, getting farther away
		source.setPosition('s', (float) (i * 24.0f), 0.0f, (2.0f + (i * 0.15f)));
		source.dump();
		sndfile.trigger();
		sleepSec(0.9);
	}
	theIO->clearRoot();
	logMsg("done.");
}

/// Spatializer with VBAP

void test_VBAP_horiz() {
				// Open a mono soundfile
	SoundFile sndfile(CGestalt::dataFolder() + "triangle_mono.aiff");
	sndfile.openForRead();
	sndfile.dump();
	sndfile.setToEnd();
				// make the sound "Positionable"
	SpatialSource source(sndfile);
				// Create a spatializer.
	Spatializer panner(kVBAP);
				// Add the sound source to it
	panner.addSource(source);
				// loop to play transpositions
	logMsg("playing VBAP-spatialized rotating sound source (horizontal plane)...");
	theIO->setRoot(panner);							// make some sound
	for (int i = 0; i < 30; i++) {
		source.setPosition('s', (float) (i * 24), 0.0f, 2.0f);	// rotate in small steps
		source.dump();
		sndfile.trigger();
		sleepSec(0.8);
	}
	theIO->clearRoot();
	logMsg("done.");
}

#endif

//////// RUN_TESTS Function ////////

#ifndef USE_JUCE

void runTests() {
//	testPan();
//	testN2MPan();
//	testSineMixer();
//	testPanMix();
//	testConvolver();
//	testConvolver2();
//	testConvolver3();
//	testOscBank();
//	testCMapIO();
	test_Binaural_horiz();
//	test_Binaural_vertAxial();
//	test_Binaural_vertMedian();
}

#else

// test list for Juce GUI

testStruct panTestList[] = {
	"Stereo panner",		testPan,				"Demonstrate the stero panner",
//	"N2M panner",			testN2MPan,	
	"Mixer",				testSineMixer,			"Mixer with 4 sine inputs (slow sum-of-sines)",
	"Panning mixer",		testPanMix,				"Play a panning stereo mixer",
	"Bigger panning mixer",	testBigPanMix,			"Test a mixer with many inputs",
#ifdef USE_CONVOLVER
	"Test convolver",		testConvolver,			"Test a convolver",
	"Test convolver 2",		testConvolver2,			"Test a convolver",
	"Test convolver 3",		testConvolver3,			"Test a convolver",
#endif
	"Osc bank",				testOscBank,			"Mix a bank of oscillators",
	"Channel-mapped IO",	testCMapIO,				"Demonstrate channel-mapped IO",
#ifndef CSL_WINDOWS
	"HRTF horiz circles",	test_Binaural_horiz,	"Test the HRTF-based binaural panner",
	"HRTF axial circles",	test_Binaural_vertAxial,"Play a HRTF-panner with axial circles",
	"HRTF median circles",	test_Binaural_vertMedian,"Play a HRTF-panner with median circles",
	"Ambisonics",			test_Ambi_horiz,		"Test the Ambisonic-based spatial panner",
	"Simple",				test_SimpleP,			"Test the simple spatial panner",
	"VBAP",					test_VBAP_horiz,		"Test the VBAP-based spatial panner",
#endif
NULL,						NULL,			NULL
};

#endif
