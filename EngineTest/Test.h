#pragma once

class test //abstract class from which all tests will inherit
{
public:
	virtual bool initialize() = 0;
	virtual void run() = 0;
	virtual void shutdown() = 0;
};