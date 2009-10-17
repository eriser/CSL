//
//  CSL_Exceptions.h -- the CSL Exception classes (specifications + implementations)
//
//	See the copyright notice and acknowledgment of authors in the file COPYRIGHT
//

#ifndef CSL_EXCEPTIONS_H
#define CSL_EXCEPTIONS_H

#include <exception>		// Standard C++ exception library
#include <string>	

namespace csl {

using namespace std;

#ifndef CSL_ENUMS
	class exception { };	// needed for SWIG
#endif

///
/// Base class of CSL exceptions (written upper-case)
///

class CException : public std::exception {
public:
	string mMessage;
	CException(const string & msg) throw() : mMessage(msg) { };
	~CException() throw() { };
};

///
/// Malloc failure
///

class MemoryError : public CException {  
public:
	MemoryError(const string & msg) : CException(msg) { };
};

///
/// Wrong kind of operand
///

class ValueError : public CException {  
public:
	ValueError(const string & msg) : CException(msg) { };
};

///
/// Time-out
///

class TimingError : public CException {  
public:
	TimingError(const string & msg) : CException(msg) { };
};

///
/// Illegal operation at run time
///

class RunTimeError : public CException {  
public:
	RunTimeError(const string & msg) : CException(msg) { };
};

///
/// Impossible operation
///

class LogicError : public CException {  
public:
	LogicError(const string & msg) : CException(msg) { };
};

///
/// Numerical data of wrong type
///

class DomainError : public ValueError { 
public:
	DomainError(const string & msg) : ValueError(msg) { };
};

///
/// Data out of range
///

class OutOfRangeError : public ValueError {  
public:
	OutOfRangeError(const string & msg) : ValueError(msg) { };
};

///
/// IO Error
///

class IOError : public ValueError {  
public:
	IOError(const string & msg) : ValueError(msg) { };
};

}

#endif
