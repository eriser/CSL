//
//  InOut.h -- copies the input buffer to the output buffer, possibly with channel remap and scaling
// Constructor: InOut(UnitGenerator &, unsigned inChan, [unsigned outChan, ch-1 ... ch-outChan]);
//
//	See the copyright notice and acknowledgment of authors in the file COPYRIGHT
//

#ifndef CSL_InOut_H
#define CSL_InOut_H

#include "CSL_Core.h"
#include <stdarg.h>		// for varargs

namespace csl {

#ifdef CSL_ENUMS
typedef enum {
	kNoProc,		// tries to keep it mono or stereo if in_chans == out_chans
	kLR2M,			// mixes the L and R inputs down to a mono signal
	kL2M,			// Copies left input to make mono
	kR2M,			// Copies right input to make mono
	kN2M,			// Copies N-channel input to M-channel output using a map
} InOutFlags;
#else
	#define kNoProc 0
	#define kLR2M 1
	#define kL2M 2
	#define kR2M 3
	#define kN2M 4
	typedef int InOutFlags;
#endif

///
/// InOut class copies the IO port's input buffer to the output buffer, possibly with channel remap and scaling
///

class InOut : public Effect {
public:
						/// Constuctor with IO, number of channels in & out, and processing
	InOut(IO * anIO, unsigned inChan, unsigned outChan, InOutFlags f );
//	InOut(IO * anIO, unsigned inChan, unsigned outChan, InOutFlags f, ...);
	InOut(UnitGenerator & myInput, unsigned inChan, unsigned outChan, InOutFlags f);
//	InOut(UnitGenerator & myInput, unsigned inChan, unsigned outChan, InOutFlags, ...);
	~InOut();

	void setInChan(unsigned chan) { mInChan= chan; };			///< set # in/out chans
	void setOutChan(unsigned chan) { mOutChan= chan; };
	unsigned getInChan(void) { return mInChan; };					///< get # in/out chans
	unsigned getOutChan(void) { return mOutChan; };	

	void setChanMap(unsigned * chans);			///< set channel map
	void setChanGains(float * values);				///< set gain array
	void setGain(unsigned index, float value);		///< set gain value at index

	virtual void nextBuffer(Buffer & outputBuffer) throw (CException);

private:
	IO * mIO;				///< The (Singleton) IO pointer (or NULL, to act as an effect)
	BufferCMap mMap;		///< the mapped buffer pointers for the output channels
	unsigned mInChan;		///< # in chans
	unsigned mOutChan;		///< # out chans
	InOutFlags mFlags;		//< copy/process flag
	float *mGains;			///< amplitude scales
};

}

#endif
