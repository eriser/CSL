//
//	Test_Audio.cpp -- Streaming audio IO tests
//
//	See the copyright notice and acknowledgment of authors in the file COPYRIGHT
//

#ifdef USE_JUCE
	#include "Test_Support.h"
#else
	#define USE_TEST_MAIN			// use the main() function in test_support.h
	#include "Test_Support.cpp"		// include all of CSL core and the test support functions
#endif

using namespace csl;

extern AudioDeviceManager * gAudioDeviceManager;	// global JUCE audio device mgr

// dump io names

void audio_dump() {
}

// 

void mic_test() {
}

// 

void echo_test() {
}

//  

void panner_test() {

}

//////////////////////////////////////////////////////////////////////////////

// make an Audio listener class -- implement update() and store in-coming audio

//class AudioListener : public Observer {
//public:
//								// update message prints to the log
//	void update(void * arg) {
//	}
//								// constructor
//	AudioListener(MIDIIn * in) : mIn(in) { };
//								// data member for the input
//	AudioListener * mIn;
//
//};
//
///// Create a MIDI in and attach a filtering listener to it
//
void listener_test() {
//	MIDIIn in;
//	in.open(DEFAULT_MIDI_IN);
//	MIDIListener lst(&in);			// create a listener
////	lst.mPeriod = 0.25;
//	lst.mKey = kNoteOn;				// filter noteOn events
//	in.attachObserver(&lst);		// attach observer to input
//	logMsg("Start MIDI listener");
//	in.start();
//	
//	sleepSec(10);
//	
//	logMsg("done.");
//	in.stop();
//	in.close();
}

// test list for Juce GUI

testStruct audioTestList[] = {
	"Dump audio ports",		audio_dump,		"Dump list of audio ports to stdout",
	"Echo audio in",		mic_test,		"Play the microphone in",
	"Audio echo",			echo_test,		"Add echo to the live input",
	"Input panner",			panner_test,	"Stereo panner on the live input",
	"Input listener",		listener_test,	"Demonstrate recording input listener",
	NULL,					NULL,			NULL
};
