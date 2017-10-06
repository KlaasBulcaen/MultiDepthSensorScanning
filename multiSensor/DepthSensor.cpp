
#include <windows.h>
#include <iostream>

#include "DepthSensor.h"

#include <nuiapi.h>
#include <NuiImageCamera.h>
#include <CameraUIControl.h>
#include <fstream>


#define STB_IMAGE_WRITE_IMPLEMENTATION
#include "stb_image_write.h"

DepthSensor::DepthSensor(INuiSensor* sensor, NUI_IMAGE_RESOLUTION DepthimgRes, NUI_IMAGE_RESOLUTION ColorimgRes)
{
	//create datastream
	m_hNextDepthFrameEvent = CreateEvent(NULL, TRUE, FALSE, NULL);
	m_hDepthStream = NULL;


	m_hNextColorFrameEvent = CreateEvent(NULL, TRUE, FALSE, NULL);
	m_hColorStream = NULL;

	m_pSensor = sensor;
	//streams
	if (!SUCCEEDED(m_pSensor->NuiImageStreamOpen(NUI_IMAGE_TYPE_DEPTH, DepthimgRes, 0, 2, m_hNextDepthFrameEvent, &m_hDepthStream)))
	{
		std::cout << "Failed to get depth stream" << std::endl;
		std::cin.get();
	}

	if (!SUCCEEDED(m_pSensor->NuiImageStreamOpen(NUI_IMAGE_TYPE_COLOR, ColorimgRes, 0, 2, m_hNextColorFrameEvent,& m_hColorStream)))
	{
		std::cout << "Failed to get color stream" << std::endl;
		std::cin.get();
	}
	m_depthRes = DepthimgRes;
	m_colorRes = ColorimgRes;

}



DepthSensor::~DepthSensor()
{
}

void DepthSensor::LoadDepthFrame()
{
	//wait till frames are received
	WaitForSingleObject(m_hDepthStream, INFINITE);
	std::cout << "depth frame received" << std::endl;
}

void DepthSensor::LoadColorFrame()
{
	//wait till frames are received
	WaitForSingleObject(m_hColorStream, INFINITE);
	std::cout << "color frame received" << std::endl;
}


void DepthSensor::CreateObjFromDepthStream(const char* filePath)
{
	LoadDepthFrame();

	 NUI_IMAGE_FRAME * pDepthImageFrame = NULL;	
	if (!SUCCEEDED(m_pSensor->NuiImageStreamGetNextFrame(m_hDepthStream, 0, pDepthImageFrame)))
	{
		std::cout << "Failed to get depth imageframe" << std::endl;
		std::cin.get();
	}


	//create filestream
	std::ofstream  objFile;
	objFile.open(filePath);
	

	//load data to readable format
	NUI_LOCKED_RECT DepthLockedRect;
	pDepthImageFrame->pFrameTexture->LockRect(0, &DepthLockedRect, NULL, 0);
	if (DepthLockedRect.Pitch == 0)
	{
		std::cout << "texture has no correct size" << std::endl;
	}


	ULONG * pBuffer = reinterpret_cast<ULONG*>(DepthLockedRect.pBits);


	pDepthImageFrame->pFrameTexture->UnlockRect(0);

	unsigned long w = 0;
	unsigned long h = 0;
	NuiImageResolutionToSize(m_depthRes, w, h);
	int width = static_cast<int>(w);
	int height = static_cast<int>(h);

	//write depth info to file
	for (int y = 0; y < height; y++)
	{
		for (int x = 0; x < width; x++)
		{
			objFile << "v " << x << " " << y << " " << static_cast<float>(pBuffer[y*width + x]) / 100000000.0f << std::endl;
			objFile << "vt " << static_cast<float>(x) / static_cast<float>(width) << " " << static_cast<float>(y) / static_cast<float>(height) << std::endl;
		}
	}

	//create mesh, do not include quads with no depth
	for (int y = 0; y < height - 1; y++)
	{
		for (int x = 0; x < width - 1; x++)
		{
			//make faces for a plane with depth info
			objFile << "f " << y*width + x + 1 << "/" << y*width + x + 1 << " " << y*width + x + 2 << "/" << y*width + x + 2 << " " << y*width + x + width + 1 << "/" << y*width + x + width + 1 << std::endl;
			objFile << "f " << y*width + x + width + 1 << "/" << y*width + x + width + 1 << " " << y*width + x + 2 << "/" << y*width + x + 2 << " " << y*width + x + width + 2 << "/" << y*width + x + width + 2 << std::endl;
		}
	}
	objFile.close();

	pDepthImageFrame->pFrameTexture->UnlockRect(0);

	std::cout << "depth file completed" << std::endl;
}


void DepthSensor::CreatepngFromColorStream(const char* filePath)
{
	LoadColorFrame();

	//wait for frame
	NUI_IMAGE_FRAME * pColorImageFrame = NULL;
	if (!SUCCEEDED(m_pSensor->NuiImageStreamGetNextFrame(m_hColorStream, 0, pColorImageFrame)))
	{
		std::cout << "Failed to get color imageframe" << std::endl;
		std::cin.get();
	}

	//lock rect
	NUI_LOCKED_RECT ColorLockedRect;
	pColorImageFrame->pFrameTexture->LockRect(0, &ColorLockedRect, NULL, 0);
	if (ColorLockedRect.Pitch == 0)
	{
		std::cout << "texture has no correct size" << std::endl;
	}

	//get img resolution
	unsigned long w = 0;
	unsigned long h = 0;
	NuiImageResolutionToSize(m_colorRes, w, h);
	UINT CWidht = static_cast<int>(w);
	UINT CHeight = static_cast<int>(h);

	ULONG * pColorBuffer = reinterpret_cast<ULONG*>(ColorLockedRect.pBits);

	//create png image file
	stbi_write_png("colorinfo.png", CWidht, CHeight, 3, pColorBuffer, CWidht * 4);
	std::cout << "color completed" << std::endl;

	//lock rect again
	pColorImageFrame->pFrameTexture->UnlockRect(0);
}
