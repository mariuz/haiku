/*
	$Id: AutolockLooperTest.h,v 1.2 2002/07/19 06:45:28 tylerdauwalder Exp $
	
	This file defines the class for performing all BAutolock tests on a
	BLooper.
	
	*/


#ifndef AutolockLooperTest_H
#define AutolockLooperTest_H


#include "ThreadedTestCase.h"
#include <string>
	
class BLooper;
class CppUnit::Test;

class AutolockLooperTest : public BThreadedTestCase {
	
private:
	BLooper *theLooper;
	
public:
	static Test *suite(void);
	void TestThread1(void);
	AutolockLooperTest(std::string);
	virtual ~AutolockLooperTest();
};
	
#endif




