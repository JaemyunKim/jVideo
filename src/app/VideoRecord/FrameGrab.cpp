﻿#include "FrameGrab.h"


// Default constructor.
FrameGrab::FrameGrab() : mThread()
{
	//mThread = std::thread(FrameGrab::run, 1000);
	mSourceMode = -1;
	mDevId = -1;
	mIsGrab = false;
	mIsReady = false;
	mFrameCount = 0;
	mFps = 0;
}


// Destructor.
FrameGrab::~FrameGrab()
{
	stop();
}


// Open source.
void FrameGrab::openSource()
{
	try
	{
		// Load input video
		if (mSourceMode == 0)
		{
			mInputCap.open(mSrcName.string());
		}
		else
		{
			mInputCap.open(mDevId);
		}

		if (!mInputCap.isOpened())
		{
			std::string msg("Input video could not be opened.");
			throw std::runtime_error(msg);
		}	
	}
	catch(std::exception &e)
	{
		std::cerr << "!!! FrameGrab: " << e.what() << std::endl;
		std::string msg("Cannot connect the device.");
		throw std::runtime_error(msg);
	}
}


// Read frame.
void FrameGrab::readFrame()
{
	try
	{
		// read the frame..
		mFrameMutex.lock();
		if (!mInputCap.read(mFrame))
		{
			std::string msg("Cannot read frames from the device.");
			throw std::runtime_error(msg);
		}
		mFrameMutex.unlock();

		// get meta-information.
		mFps = mInputCap.get(CV_CAP_PROP_FPS);
		mResolution = cv::Size((int)mInputCap.get(CV_CAP_PROP_FRAME_WIDTH), (int)mInputCap.get(CV_CAP_PROP_FRAME_HEIGHT));
	}
	catch(std::exception &e)
	{
		std::cerr << "!!! FrameGrab: " << e.what() << std::endl;
		return;
	}
}


// Close source.
void FrameGrab::closeSource()
{
	if (mInputCap.isOpened())
		mInputCap.release();
}


// Start frame grab.
void FrameGrab::start(cv::Mat &frame)
{
	try
	{
		// connect to source device.
		openSource();

		// set the status.
		mIsGrab = true;

		// This will start the thread. Notice move semantics!
		mThread = std::thread(&FrameGrab::run, this, std::ref(frame));// , this);
	}
	catch (std::exception &e)
	{
		stop();
		std::cerr << "!!! FrameGrab: " << e.what() << std::endl;
		std::string msg("Stop.");
		throw std::runtime_error(msg);
	}
}


// Stop frame grab.
void FrameGrab::stop()
{
	// set the status.
	mIsGrab = false;

	// finish the thread for the frame grab.
	if (mThread.joinable())
		mThread.join();

	// disconnect to the source device.
	closeSource();
}

// Run frame grab.
void FrameGrab::run(cv::Mat &frame)
{
	// keep taking the frames.
	try
	{
		// check times
		std::chrono::duration<double> totalProcTime, procTime;
		std::chrono::system_clock::time_point startTimestamp, endTimestamp;

		// check the connection status.
		if (!mInputCap.isOpened())
		{
			std::string msg("Input video could not be opened.");
			throw std::runtime_error(msg);
		}

		// reset frame count.
		mFrameCount = 0;

		// check total processing time.
		totalProcTime = std::chrono::duration<float>(0);

		// Loop to read from input.
		while (mIsGrab == true)
		{
			// determine time at start.
			startTimestamp = std::chrono::system_clock::now();

			// read from on the device.
			readFrame();
			if (mFrame.empty())
				return;

			// copy the frame to shared frame buffer.
			mFrameMutex.lock();
			mFrame.copyTo(frame);
			mFrameMutex.unlock();

			// set the working status.
			mIsReady = true;

			// increase frame counter.
			mFrameCount++;

			// determine time at end.
			endTimestamp = std::chrono::system_clock::now();
			procTime = endTimestamp - startTimestamp;

			// print frame counter and processing time.
			std::cout << "  ** FrameGrab:\tframe: " << mFrameCount << "\tprocessing time: " << procTime.count() << " sec" << std::endl;

			// wait a milli seconds to reduce cpu usage.
			std::this_thread::sleep_for(std::chrono::milliseconds(1));

			// calculate total processing time.
			totalProcTime += procTime;
		}

		// set the working status.
		mIsReady = false;

		std::cout << "  ** Grabbing time: " << totalProcTime.count() << " sec" << std::endl;
		std::cout << "  ** Average grabbing time: " << totalProcTime.count() / mFrameCount << " sec" << std::endl;
	}
	catch(std::exception &e)
	{
		std::cerr << "!!! FrameGrab: " << e.what() << std::endl;
		return;
	}
}