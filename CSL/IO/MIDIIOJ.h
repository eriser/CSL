///
///  MIDIIOJ.h -- MIDI IO using JUCE
///
///	See the copyright notice and acknowledgment of authors in the file COPYRIGHT
///

#ifndef CSL_MIDIIO_H
#define CSL_MIDIIO_H

#include "CSL_Types.h"
#include "CGestalt.h"
#include "Instrument.h"
#include "juce.h"

namespace csl {
	
///	CMIDIMessageType enum of midi msg categories

typedef enum {
	kNone			= 0,
	kNoteOff		= 8,
	kNoteOn			= 9,
	kPolyTouch		= 10,
	kControlChange	= 11,
	kProgramChange  = 12,
	kAftertouch		= 13,
	kPitchWheel		= 14,
	kSysEX			= 15
} CMIDIMessageType;

///	Message_ChannelToStatus -- converts from message and channel to status byte

#define MessageChannelToStatus(message, channel)  ((unsigned) message << 4) + channel)

///
///	CMIDIMessage class (mapped to juce::MidiMessage)
///

class CMIDIMessage {
public:
	CMIDIMessage();
	CMIDIMessage(CMIDIMessageType t, unsigned ch, unsigned d1, unsigned d2);
	
	bool isNoteOn();					///< bool flags for events
	bool isNoteOff();
	bool isPolyTouch();
	bool isControlChange();
	bool isProgramChange();
	bool isAftertouch();
	bool isPitchWheel();
	bool isSysEX();
	
	unsigned getCommand();				///< note accessors
	unsigned getNote();
	unsigned getVelocity();
	unsigned getPolyAftertouch();
	unsigned getControlFunction();
	unsigned getControlValue();
	unsigned getProgramNumber();
	unsigned getAftertouch();
	unsigned getPitchWheel();
	float getFrequency();
	float getVelocityFloat();			///< has range of [0.0 1.0] mapped to [0 127]
										/// Data fields
	CMIDIMessageType message;			///< event type
	unsigned command;
	unsigned channel;					///< 0-indexed, so from 0 to 15 
	unsigned data1;
	unsigned data2;
	float time;							///< timestamp in sec
};


///	MIDIIO class: superclass of in and out; has a message buffer and current messages
///		It's a model so you can observe it.
///		Uses mMsg.CMIDIMessageType as a status flag.
	
class MIDIIO: public Model {		///< It's a model & sends itself "changed"
public:	
	MIDIIO();
	virtual ~MIDIIO();
	
	static int countDevices();
	static void dumpDevices();		///< printing device info for all devices.

	void open();					///< open the abstract 
	virtual void open(int devID) = 0;	///< open a device
	bool isOpen();					///< true if MIDI stream is opened. 
	virtual void close();			///< closing MIDI stream
	virtual void start() { };		///< start MIDI stream
	virtual void stop() { };		///< stop MIDI stream
	virtual void clear();			///< clear MIDI stream
	void dumpBuffer();

	int mDeviceID;					///< device ID which will/is opened.
	CMIDIMessage mMsg;				///< current message (its flags determine the port state)
	CMIDIMessage mMsg2;	
	MidiBuffer mBuffer;				///< I/O buffer

protected:							///< static flags to keep track of driver state
	static bool mIsInitialized; 
	
	juce::MidiMessage * mJMsg;		///< JUCE-format message
				
	bool mIsOpen;					///< instance status indicators
	long mBufferSize;
	long mFilterFlag;
									///  error handler
	void handleError(CException * err);
	
									/// copy csl::CMIDIMessage <--> juce::MidiMessage
	void copyMessage(CMIDIMessage& source, CMIDIMessage& dest);
	void copyMessage(CMIDIMessage& source, MidiMessage* dest);
	void copyMessage(const MidiMessage& source, CMIDIMessage& dest);
};


///
///	MIDIIn class is-a MidiInputCallback too, and an "input-ready" flag
///

class MIDIIn : public MIDIIO, public MidiInputCallback {
public:
	MIDIIn();	
	
	unsigned bufferSize();
	void setBufferSize(unsigned bufferSize );
	virtual void open(int deviceID);	///< open a device
	bool poll();						///< poll returns a bool (really quickly)
	void nextEvent();					///< step to next event or reset flag
	void dumpMessage();					///< print current msg
	virtual void start();				///< start MIDI stream
	virtual void stop();				///< stop MIDI stream
	int evaluate(void * arg);			///< evaluate answers the message command

										/// implement inherited MidiInputCallback
	void handleIncomingMidiMessage(MidiInput * source, const MidiMessage & message);
	
	MidiInput * mDevice;				///< my device ptr
	double mStartTime;					///< the time I was started
};


///
///	MIDIOut class write msgs out to a device (or file)
///

class MIDIOut : public MIDIIO {
public:
	MIDIOut();
	~MIDIOut();

	MidiOutput * mOut;								///< the juce midi output is public
	
	virtual void open(int deviceID );
	void write(CMIDIMessage & msg);
	void writeNoteOn(unsigned channel, unsigned pitch, unsigned velocity );		///< MIDINote#, [0, 127]
	void writeNoteOn(unsigned channel, float frequency, float amplitude );		///< [Hz], [0.0 1.0];
	void writeNoteOff(unsigned channel, unsigned pitch, unsigned velocity );		///< MIDINote#, [0, 127]
	void writeNoteOff(unsigned channel, float frequency, float amplitude );		///< [Hz], [0.0 1.0];
	void writePolyTouch(unsigned channel, unsigned pitch, unsigned amount );
	void writeControlChange(unsigned channel, unsigned function, unsigned value );
	void writeProgramChange(unsigned channel, unsigned programNum );
	void writeAftertouch(unsigned channel, unsigned amount );						///< [0, 127]
	void writePitchWheel(unsigned channel, unsigned amount );						///< [0, 16384] 
	void writeSysEX(long when, unsigned char *msg );

protected:
	long		mBufferSize;
	long		mLatency;
	
};


///
/// MIDI stream/file player
///

class MIDIPlayer: public MIDIIO {
public:
	MIDIPlayer(string nam, InstrumentLibrary * lib);
	MIDIPlayer(string folder, string nam, InstrumentLibrary * lib);
	~MIDIPlayer() { }
	
	void open(int devID) { };		///< open a device (empty method)
	void start(int index);			///< play a track; merges tracks if index< 0
	void stop();					///< stop playing
	
	MidiFile mFile;					///< JUCE MIDI file
	int mNumTrax;					///< num tracks
	MidiMessageSequence * mTrak;	///< track ptr
	bool mIsOn;						///< Active flag
	
	InstrumentLibrary * mLibrary;	///< instrument library
	float mTempoScale;				///< tempo scale (secs/beat / ticks/beat)
	
protected:
	void init(String namS);
	MidiMessageSequence * mergeTrax();

};

}   // csl namespace

#endif
