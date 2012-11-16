//
//  STP_Widgets.h - JUCE widgets for audio by stp@heaveneverywhere.com
//	What's here:
//		AudioWaveformDisplay - simple oscilloscope based on the audio demo code
//		AudioSpectrumDisplay - simple spectrogram
//		RangeSlider - interprets its two markers as base value and range, rather than min/max
//			i.e., the upper marker is the base value, and the lower one is the random range
//			(This is removed for now since it requires a change to Slider, making some of the private members protected.)
//		VUMeter (based on killerbobjr's MeterComponent)
//			see http://www.rawmaterialsoftware.com/juceforum/viewtopic.php?t=608
//			and	 http://mysite.verizon.net/mikews/vumeters.zip
//

// in CSL_Types.h

//// struct used for the JUCE pop-up menu of tests (see the test files)

//	typedef struct {
//		char * name;			// test name (menu item)
//		VoidFcnPtrN * fcn;		// test void fcn ptr
//		char * comment;			// test comment
//	} testStruct;

// in the test files

// test list for Juce GUI

//testStruct oscTestList[] = {
//	"Sweep test",				testSweep,			"Test a sine with swept freq and volume swell",
//	"Simple sines",				testSimpleSines,	"Test some simple sine oscilators",
//	"Standard waveforms",		testBasicWaves,		"Demonstrate the standard wave forms",
//	"Scaled sine",				testScaledSin,		"Play a scaled-quiet sine wave",
//	"Wavetable interpolation",	testWavetableInterpolation,	"Show truncated/interpolated wave tables",
//	"AM/FM sines",				testAMFMSin,				"Play an AM and FM sine wave",
//	"Dump AM/FM sines",			dumpAMFMSin,				"Dump the graph of the AM/FM sine",
//	"SumOfSines cached",		testSumOfSinesCached,		"Play a sum-of-sines additive oscillator",
//	"SumOfSines non-cached",	testSumOfSinesNonCached,	"Play an uncached inharmonic sum-of-sines", 
//	"SumOfSines build",			testSumOfSinesSteps,		"Build up a harmonic series on a sum-of-sines",
//	"SumOfSines 1/f",			testSumOfSines1F,			"Play a 1/f spectrum sum-of-sines",
//	"Wavetable from file",		testWaveTableFromFile,		"Play a wave table from a sound file",
//	"SHARC SOS",				test_SHARC,					"Load/print the SHARC timbre database, play example",
//	"Vector SHARC",				test_SHARC2,				"Show vector cross-fade of SHARC spectra",
//	NULL,						NULL,				NULL
//};


#include "../JuceLibraryCode/JuceHeader.h"
#include "CSL_Types.h"

// Note that defining RANGE_SLIDER requires a change to the privacy of class Slider's members;
// comment out line 705 of ~/Code/juce/src/juce_appframework/gui/components/controls/juce_Slider.h
// (This line says "private:")

//#define RANGE_SLIDER

//////////////////////////////////////////////////////////////////////////
//
// AudioWaveformDisplay = your basic oscilloscope component
// This can be zoomed in X and Y, and repaints itself at the selected rate (see initialise)
//

class AudioWaveformDisplay : public Component,
							public Timer,
							public AudioIODeviceCallback {
public:
						// Constructor
	AudioWaveformDisplay();
    ~AudioWaveformDisplay();
						// set up oscilloscope state (since constructor takes no args)
	void initialise(int channel, unsigned rate, unsigned window, bool zeroX);
						// start/stop display
	void start();
	void stop();
						// callback accumulates samples
    virtual void audioDeviceIOCallback (const float** inputChannelData, int totalNumInputChannels,
                                float** outputChannelData, int totalNumOutputChannels, int numSamples);
    void audioDeviceAboutToStart (double sampleRate, int numSamplesPerBlock);
    void audioDeviceAboutToStart (AudioIODevice* device) { };
    void audioDeviceStopped();
    void setIndicator(unsigned value) { indicatorValue = value; }

    void timerCallback();					// timer redraws
	virtual void paint (Graphics& g);		// main scope draw method

	void setSamplesToAverage(unsigned val) { samplesToAverage = val; }
	
protected:
    float * circularBuffer;			// This is the sample bufefr for the waveform display 
									// and the pixel buffer for the spectrum display
    float currentOutputLevel;
    int volatile bufferPos, bufferSize, numSamplesIn;
	unsigned delayInMsec;
	unsigned samplesToAverage;
	unsigned indicatorValue;
	int whichChannel;
	bool zeroCross;
	
    void addSample (const float sample);		// adding a sample accumulates values
};


//////////////////////////////////////////////////////////////////////////
//
// AudioSpectrumDisplay = your basic spectrogram component
// This can be zoomed in X and Y, and repaints itself at the selected rate (see initialise)
//

class AudioSpectrumDisplay : public AudioWaveformDisplay {
public:							// Constructor
	AudioSpectrumDisplay();
    ~AudioSpectrumDisplay();
	
    virtual void audioDeviceIOCallback (const float** inputChannelData, int totalNumInputChannels,
							float** outputChannelData, int totalNumOutputChannels, int numSamples);
	virtual void paint (Graphics& g);		// main scope draw method

	bool mLogDisplay;			// if true, use log display, else linear
	bool mSpectroDisplay;		// if true, show full spectrogram, else single spectral slice
	unsigned numWindows;

protected:
    float ** spectrumBuffer;	// 2D spectral frames
};

#ifdef RANGE_SLIDER

//////////////////////////////////////////////////////////////////////////
//
// A RangeSlider interprets its two markers as base value and range, rather than min/max
//

class RangeSlider : public Slider {
public:
    RangeSlider (const String& componentName) : Slider(componentName) { };
    ~RangeSlider() { };
				// set/get new values
	double getBaseValue();
    double getRangeValue();
	void setBaseValue (double newValue, const bool sendUpdateMessage = true, const bool sendMessageSynchronously = true);
	void setRangeValue (double newValue, const bool sendUpdateMessage = true, const bool sendMessageSynchronously = true);
	
protected:		// work methods
	void mouseDown (const MouseEvent& e);
	void mouseDrag (const MouseEvent& e);
				// data members
	double baseValue, rangeValue;
};

#endif

//////////////////////////////////////////////////////////////////////////
//
// VU meters - horizontal bar, vertical bar, and analog meter
//

class VUMeter : public Component, 
				public Timer,
				public AudioIODeviceCallback {
public:
	
	enum {    /** The type of meters to use. */
		Vertical = 1,
		Horizontal,
		Analog
	};
	
	enum {    /** Needle drop shadow type */
		None = 1,
		Left,
		Right,
		Above,
		Below
	};
	
	VUMeter();						// default constructor
	
	VUMeter(int channel,			// digital c'tor
			int type,
			int segments=0, 
			int markerWidth=2,
			const Colour& minColour=Colours::green, 
			const Colour& thresholdColour=Colours::yellow, 
			const Colour& maxColour=Colours::red, 
			const Colour& back=Colours::black, 
			float threshold=0.707f);
			
	VUMeter(int channel,			// analog c'tor
			Image* background,
			Image* overlay, 
			float minPosition, 
			float maxPosition, 
			Point<int>& needleCenter,
			int needleLength,
			int needleWidth=2,
			int arrowLength=4,
			int arrowWidth=4,
			const Colour& needleColour=Colours::black,
			int needleDropShadow=None,
			int dropDistance=0);
			
	~VUMeter();
	
	void setBounds (int x, int y, int width, int height);
	void setFrame(int inset, bool raised=false);
	void setColours(Colour& min, Colour& threshold, Colour& max, Colour& back);
	void setDecay(int decay, int hold, float percent=0.707f);
	void setChannel(unsigned channel) { m_channel = channel; }
	void setValue(float v);
	float getValue() { return m_value; }
	void setSkewFactor(float skew) { m_skew = skew; }
	
	void audioDeviceIOCallback (const float** inputChannelData, int totalNumInputChannels,
                                float** outputChannelData, int totalNumOutputChannels, int numSamples);
    void audioDeviceAboutToStart (double sampleRate, int numSamplesPerBlock) { };
    void audioDeviceStopped() { };
    void audioDeviceAboutToStart (AudioIODevice* device) { };

	juce_UseDebuggingNewOperator
	
protected:
	void paint (Graphics& g);
	
private:
	Image* m_img;
	float m_value;
	float m_skew;
	float m_threshold;
	int m_meterType;
	int m_segments;
	int m_markerWidth;
	int m_inset;
	unsigned m_channel;
	bool m_raised;
	Colour m_minColour;
	Colour m_thresholdColour;
	Colour m_maxColour;
	Colour m_backgroundColour;
	int m_decayTime;
	float m_decayPercent;
	float m_decayToValue;
	int m_hold;
	int m_monostable;
	Image* m_background;
	Image* m_overlay;
	float m_minPosition;
	float m_maxPosition;
	Point<int> m_needleCenter;
	int m_needleLength;
	int m_needleWidth;
	int m_arrowLength;
	int m_arrowWidth;
	Colour m_needleColour;
	int m_needleDropShadow;
	int m_dropDistance;
	
	void buildImage(void);
	virtual void timerCallback();
};

// Bitmaps for fancy analog meters

namespace bmps {
    extern const char*  vuback_png;
    const int           vuback_pngSize = 73824;

    extern const char*  vufront_png;
    const int           vufront_pngSize = 58837;
};
