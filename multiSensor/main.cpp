#include <windows.h>
#include <Ole2.h>

#include <nuiapi.h>
#include <NuiImageCamera.h>
#include <CameraUIControl.h>
#include <iostream>
#include <fstream>

#define _CRTDBG_MAP_ALLOC  
#include <stdlib.h>  
#include <crtdbg.h>

#define DEPTHRESOLUTION NUI_IMAGE_RESOLUTION_320x240
//#define DEPTHRESOLUTION NUI_IMAGE_RESOLUTION_80x60

INuiSensor* InitSensor(int index = 0)
{
	INuiSensor* sensor;
	if (FAILED(NuiCreateSensorByIndex(index, &sensor)))
	{
		std::cout << "creating sensor by index failed: " << index << std::endl;
		std::cin.get();
		return nullptr;
	}

	if (FAILED(NuiInitialize(NUI_INITIALIZE_FLAG_USES_COLOR | NUI_INITIALIZE_FLAG_USES_DEPTH)))
	{
		std::cout << "initializing sensor failed" << std::endl;
		std::cin.get();
		return nullptr;
	}

	return sensor;
}

int main()
{

	_CrtSetDbgFlag(_CRTDBG_ALLOC_MEM_DF | _CRTDBG_LEAK_CHECK_DF);




#pragma region SENSOR
	//sensorcheck
	int sensorCount = 0;
	NuiGetSensorCount(&sensorCount);
	std::cout << "sensorcount: " << sensorCount << std::endl;

	//create and init Sensor
	INuiSensor* sensor = nullptr;
	sensor = InitSensor(0);
	if (!sensor)
		return 1;
	//sensor->NuiCameraElevationSetAngle(10);

	std::cout << "sensor initialized" << std::endl;
#pragma endregion 



	//create datastream
	HANDLE m_hNextVideoFrameEvent = CreateEvent(NULL, TRUE, FALSE, NULL);
	HANDLE streamhandle = NULL;
	if (!SUCCEEDED(NuiImageStreamOpen(NUI_IMAGE_TYPE_DEPTH, DEPTHRESOLUTION, 0, 2, m_hNextVideoFrameEvent, &streamhandle)))
	{
		std::cout << "Failed to get stream" << std::endl;
		std::cin.get();
		return 1;
	}


place:
	std::cout << "check" << std::endl;

	//wait till frame is received
	WaitForSingleObject(m_hNextVideoFrameEvent, INFINITE);
	std::cout << "frame taken" << std::endl;


	const NUI_IMAGE_FRAME * pImageFrame = NULL;
	if (!SUCCEEDED(NuiImageStreamGetNextFrame(streamhandle, 0, &pImageFrame)))
	{
		std::cout << "Failed to get imageframe" << std::endl;
		std::cin.get();
		return 1;
	}

	//define resolution
	unsigned long w = 0;
	unsigned long h = 0;
	NuiImageResolutionToSize(DEPTHRESOLUTION, w, h);
	UINT width = static_cast<int>(w);
	UINT height = static_cast<int>(h);

	//create filestream
	std::ofstream  objFile;
	objFile.open("depthIMG.obj");


	//load data to readable format
	NUI_LOCKED_RECT LockedRect;
	pImageFrame->pFrameTexture->LockRect(0, &LockedRect, NULL, 0);
	if (LockedRect.Pitch == 0)
	{
		std::cout << "texture has no correct size" << std::endl;
	}

	UINT * pBuffer = reinterpret_cast<UINT*>(LockedRect.pBits);

	pImageFrame->pFrameTexture->UnlockRect(0);

	//write depth info to file
	for (size_t y = 0; y < height / 2; y++)
	{
		for (size_t x = 0; x < width / 2; x++)
		{
			objFile << "v " << x << ", " << y << ", " << static_cast<float>(pBuffer[y*width + x]) / 10000000.0f << std::endl;
		}
	}

	//create mesh, do not include quads with no depth
	for (size_t y = 0; y < height / 2 - 1; y++)
	{
		for (size_t x = 0; x < width / 2 - 1; x++)
		{
			//int sumDepth = pBuffer[y*width + x].depth + pBuffer[y*width + x + 1].depth + pBuffer[y*width + x + width].depth + pBuffer[y*width + x + width + 1].depth;
			//int sumDepth = pBuffer[y*width + x] + pBuffer[y*width + x + 1] + pBuffer[y*width + x + width] + pBuffer[y*width + x + width + 1];
			//if (sumDepth > .5f)
			//{
				//make faces for a plane with depth info
			objFile << "f " << y*width + x + 1 << ", " << y*width + x + 2 << ", " << y*width + x + width + 1 << std::endl;
			objFile << "f " << y*width + x + width + 1 << ", " << y*width + x + 2 << ", " << y*width + x + width + 2 << std::endl;
			//}
		}
	}
	objFile.close();

	goto place;

	std::cout << "obj completed" << std::endl;

	std::cout << "Press Enter to close window" << std::endl;
	std::cin.get();

	return 0;
}
