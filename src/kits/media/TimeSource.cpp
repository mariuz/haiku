/***********************************************************************
 * AUTHOR: Marcus Overhagen
 *   FILE: TimeSource.cpp
 *  DESCR: 
 ***********************************************************************/
#include <TimeSource.h>
#include "debug.h"
#include "DataExchange.h"
#include "ServerInterface.h"

namespace BPrivate { namespace media {

struct TimeSourceTransmit // sizeof() must be <= 4096
{
	#define INDEX_COUNT 128
	int32 readindex;
	int32 writeindex;
	int32 isrunning;
	bigtime_t realtime[INDEX_COUNT];
	bigtime_t perftime[INDEX_COUNT];
	float drift[INDEX_COUNT];
};

} }


/*************************************************************
 * protected BTimeSource
 *************************************************************/

BTimeSource::~BTimeSource()
{
	CALLED();
	if (fArea > 0)
		delete_area(fArea);
}

/*************************************************************
 * public BTimeSource
 *************************************************************/

status_t
BTimeSource::SnoozeUntil(bigtime_t performance_time,
						 bigtime_t with_latency,
						 bool retry_signals)
{
	CALLED();
	bigtime_t time;
	status_t err;
	do {
		time = RealTimeFor(performance_time, with_latency);
		err = snooze_until(time, B_SYSTEM_TIMEBASE);
	} while (err == B_INTERRUPTED && retry_signals);
	return err;
}


bigtime_t
BTimeSource::Now()
{
	CALLED();
	return PerformanceTimeFor(RealTime());
}


bigtime_t
BTimeSource::PerformanceTimeFor(bigtime_t real_time)
{
	CALLED();
	bigtime_t last_perf_time; 
	bigtime_t last_real_time; 
	float last_drift; 

	while (GetTime(&last_perf_time, &last_real_time, &last_drift) != B_OK)
		snooze(1);
		
	return (bigtime_t)(last_perf_time + (real_time - last_real_time) * last_drift);
}


bigtime_t
BTimeSource::RealTimeFor(bigtime_t performance_time,
						 bigtime_t with_latency)
{
	CALLED();

	if (fIsRealtime) {
		return performance_time - with_latency;
	}

	bigtime_t last_perf_time; 
	bigtime_t last_real_time; 
	float last_drift; 

	while (GetTime(&last_perf_time, &last_real_time, &last_drift) != B_OK)
		snooze(1);

	return (bigtime_t)(last_real_time + (performance_time - last_perf_time) / last_drift) - with_latency;
}


bool
BTimeSource::IsRunning()
{
	CALLED();
	bool isrunning;
	isrunning = fBuf ? atomic_add(&fBuf->isrunning, 0) : fStarted;
	
	printf("BTimeSource::IsRunning() node %ld, port %ld, %s\n", fNodeID, fControlPort, isrunning ? "yes" : "no");
	return isrunning;
}


status_t
BTimeSource::GetTime(bigtime_t *performance_time,
					 bigtime_t *real_time,
					 float *drift)
{
	CALLED();

	if (fIsRealtime) {
		*performance_time = *real_time = system_time();
		*drift = 1.0f;
		return B_OK;
	}
//	if (fBuf == 0) {
//		FATAL("BTimeSource::GetTime: fBuf == 0, name %s, id %ld\n",Name(),ID());
//		*performance_time = *real_time = system_time();
//		*drift = 1.0f;
//		return B_OK;
//	}

	int32 index;
	index = atomic_and(&fBuf->readindex, INDEX_COUNT - 1);
	*real_time = fBuf->realtime[index];
	*performance_time = fBuf->perftime[index];
	*drift = fBuf->drift[index];

//	if (*real_time == 0) {
//		*performance_time = *real_time = system_time();
//		*drift = 1.0f;
//		return B_OK;
//	}

	printf("BTimeSource::GetTime     timesource %ld, perf %16Ld, real %16Ld, drift %2.2f\n", ID(), *performance_time, *real_time, *drift);
	return B_OK;
}


bigtime_t
BTimeSource::RealTime()
{
	CALLED();
	return system_time();
}


status_t
BTimeSource::GetStartLatency(bigtime_t *out_latency)
{
	CALLED();
	*out_latency = 0;
	return B_OK;
}

/*************************************************************
 * protected BTimeSource
 *************************************************************/


BTimeSource::BTimeSource() : 
	BMediaNode("This one is never called"),
	fStarted(false),
	fArea(-1),
	fBuf(NULL),
	fIsRealtime(false)
{
	CALLED();
	AddNodeKind(B_TIME_SOURCE);
//	printf("##### BTimeSource::BTimeSource() name %s, id %ld\n", Name(), ID());

	// This constructor is only called by real time sources that inherit
	// BTimeSource. We create the communication area in FinishCreate(),
	// since we don't have a correct ID() until this node is registered.
}


status_t
BTimeSource::HandleMessage(int32 message,
						   const void *rawdata,
						   size_t size)
{
	INFO("BTimeSource::HandleMessage %#lx, node %ld\n", message, fNodeID);

	switch (message) {
		case TIMESOURCE_OP:
		{
			const time_source_op_info *data = static_cast<const time_source_op_info *>(rawdata);
			switch (data->op) {
				case B_TIMESOURCE_START:
					DirectStart(data->real_time);
					break;
				case B_TIMESOURCE_STOP:
					DirectStop(data->real_time, false);
					break;
				case B_TIMESOURCE_STOP_IMMEDIATELY:
					DirectStop(data->real_time, true);
					break;
				case B_TIMESOURCE_SEEK:
					DirectSeek(data->performance_time, data->real_time);
					break;
			}
			status_t result;
			result = TimeSourceOp(*data, NULL);
			if (result != B_OK) {
				FATAL("BTimeSource::HandleMessage: TimeSourceOp failed\n");
			}
			return B_OK;
		}
	}
	return B_ERROR;
}


void
BTimeSource::PublishTime(bigtime_t performance_time,
						 bigtime_t real_time,
						 float drift)
{
	printf("BTimeSource::PublishTime timesource %ld, perf %16Ld, real %16Ld, drift %2.2f\n", ID(), performance_time, real_time, drift);
	if (0 == fBuf) {
		FATAL("BTimeSource::PublishTime timesource %ld, fBuf = NULL\n", ID());
		fStarted = true;
		return;
	}

	int32 index;
	index = atomic_add(&fBuf->writeindex, 1) & (INDEX_COUNT - 1);
	fBuf->realtime[index] = real_time;
	fBuf->perftime[index] = performance_time;
	fBuf->drift[index] = drift;
	atomic_add(&fBuf->readindex, 1);
}


void
BTimeSource::BroadcastTimeWarp(bigtime_t at_real_time,
							   bigtime_t new_performance_time)
{
	UNIMPLEMENTED();
	// call BMediaNode::TimeWarp() of all slaved nodes
}


void
BTimeSource::SendRunMode(run_mode mode)
{
	UNIMPLEMENTED();
	// send the run mode change to all slaved nodes
}


void
BTimeSource::SetRunMode(run_mode mode)
{
	CALLED();
	BMediaNode::SetRunMode(mode);
	SendRunMode(mode);
}
/*************************************************************
 * private BTimeSource
 *************************************************************/

/*
//unimplemented
BTimeSource::BTimeSource(const BTimeSource &clone)
BTimeSource &BTimeSource::operator=(const BTimeSource &clone)
*/

status_t BTimeSource::_Reserved_TimeSource_0(void *) { return B_ERROR; }
status_t BTimeSource::_Reserved_TimeSource_1(void *) { return B_ERROR; }
status_t BTimeSource::_Reserved_TimeSource_2(void *) { return B_ERROR; }
status_t BTimeSource::_Reserved_TimeSource_3(void *) { return B_ERROR; }
status_t BTimeSource::_Reserved_TimeSource_4(void *) { return B_ERROR; }
status_t BTimeSource::_Reserved_TimeSource_5(void *) { return B_ERROR; }

/* explicit */
BTimeSource::BTimeSource(media_node_id id) :
	BMediaNode("This one is never called"),
	fStarted(false),
	fArea(-1),
	fBuf(NULL),
	fIsRealtime(false)
{
	CALLED();
	AddNodeKind(B_TIME_SOURCE);
	ASSERT(id > 0);
//	printf("###### explicit BTimeSource::BTimeSource() id %ld, name %s\n", id, Name());

	// This constructor is only called by the derived BPrivate::media::TimeSourceObject objects
	// We create a clone of the communication area
	char name[32];
	area_id area;
	sprintf(name, "__timesource_buf_%ld", id);
	area = find_area(name);
	if (area <= 0) {
		FATAL("BTimeSource::BTimeSource couldn't find area, node %ld\n", id);
		return;
	}
	sprintf(name, "__cloned_timesource_buf_%ld", id);
	fArea = clone_area(name, reinterpret_cast<void **>(const_cast<BPrivate::media::TimeSourceTransmit **>(&fBuf)), B_ANY_ADDRESS, B_READ_AREA | B_WRITE_AREA, area);
	if (fArea <= 0) {
		FATAL("BTimeSource::BTimeSource couldn't clone area, node %ld\n", id);
		return;
	}
}


void
BTimeSource::FinishCreate()
{
	CALLED();
	//printf("BTimeSource::FinishCreate(), id %ld\n", ID());
	
	char name[32];
	sprintf(name, "__timesource_buf_%ld", ID());
	fArea = create_area(name, reinterpret_cast<void **>(const_cast<BPrivate::media::TimeSourceTransmit **>(&fBuf)), B_ANY_ADDRESS, B_PAGE_SIZE, B_FULL_LOCK, B_READ_AREA | B_WRITE_AREA);
	if (fArea <= 0) {
		FATAL("BTimeSource::BTimeSource couldn't create area, node %ld\n", ID());
		fBuf = NULL;
		return;
	}
	fBuf->readindex = 0;
	fBuf->writeindex = 1;
	fBuf->realtime[0] = 0;
	fBuf->perftime[0] = 0;
	fBuf->drift[0] = 1.0f;
	fBuf->isrunning = fStarted;
}


status_t
BTimeSource::RemoveMe(BMediaNode *node)
{
	UNIMPLEMENTED();

	return B_ERROR;
}


status_t
BTimeSource::AddMe(BMediaNode *node)
{
	UNIMPLEMENTED();

	return B_ERROR;
}


void
BTimeSource::DirectStart(bigtime_t at)
{
	UNIMPLEMENTED();
	if (fBuf)
		atomic_or(&fBuf->isrunning, 1);
	else
		fStarted = true;
}


void
BTimeSource::DirectStop(bigtime_t at,
						bool immediate)
{
	UNIMPLEMENTED();
	if (fBuf)
		atomic_and(&fBuf->isrunning, 0);
	else
		fStarted = false;
}


void
BTimeSource::DirectSeek(bigtime_t to,
						bigtime_t at)
{
	UNIMPLEMENTED();
}


void
BTimeSource::DirectSetRunMode(run_mode mode)
{
	UNIMPLEMENTED();
}
