//----------------------------------------------------------------------
//  This software is part of the OpenBeOS distribution and is covered 
//  by the OpenBeOS license.
//---------------------------------------------------------------------

#include <stdio.h>
#include <string.h>

#include <OS.h>

#include <KDiskDevice.h>
#include <KDiskDeviceManager.h>

const char *kTestFileDevice = "/boot/home/tmp/test-file-device";

// main
int
main()
{
	KDiskDeviceManager::CreateDefault();
	KDiskDeviceManager *manager = KDiskDeviceManager::Default();
	manager->InitialDeviceScan();
	status_t error = manager->CreateFileDevice(kTestFileDevice);
	if (error != B_OK)
		printf("creating the file device failed: %s\n", strerror(error));
	if (manager->Lock()) {
		for (int32 i = 0; KDiskDevice *device = manager->DeviceAt(i); i++) {
			device->Dump();
			printf("\n");
		}
		manager->Unlock();
	}

//	error = manager->DeleteFileDevice(kTestFileDevice);
//	if (error != B_OK)
//		printf("deleting the file device failed: %s\n", strerror(error));


	KDiskDeviceManager::DeleteDefault();
	return 0;
}

