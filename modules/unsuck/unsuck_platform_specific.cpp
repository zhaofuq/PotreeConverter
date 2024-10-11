
#include "unsuck.hpp"

#ifdef _WIN32
	#include "TCHAR.h"
	#include "pdh.h"
	#include "windows.h"
	#include "psapi.h"

// see https://stackoverflow.com/questions/63166/how-to-determine-cpu-and-memory-consumption-from-inside-a-process
MemoryData getMemoryData() {

	MemoryData data;

	{
		MEMORYSTATUSEX memInfo;
		memInfo.dwLength = sizeof(MEMORYSTATUSEX);
		GlobalMemoryStatusEx(&memInfo);
		DWORDLONG totalVirtualMem = memInfo.ullTotalPageFile;
		DWORDLONG virtualMemUsed = memInfo.ullTotalPageFile - memInfo.ullAvailPageFile;;
		DWORDLONG totalPhysMem = memInfo.ullTotalPhys;
		DWORDLONG physMemUsed = memInfo.ullTotalPhys - memInfo.ullAvailPhys;

		data.virtual_total = totalVirtualMem;
		data.virtual_used = virtualMemUsed;

		data.physical_total = totalPhysMem;
		data.physical_used = physMemUsed;

	}

	{
		PROCESS_MEMORY_COUNTERS_EX pmc;
		GetProcessMemoryInfo(GetCurrentProcess(), (PROCESS_MEMORY_COUNTERS*)&pmc, sizeof(pmc));
		SIZE_T virtualMemUsedByMe = pmc.PrivateUsage;
		SIZE_T physMemUsedByMe = pmc.WorkingSetSize;

		static size_t virtualUsedMax = 0;
		static size_t physicalUsedMax = 0;

		virtualUsedMax = max(virtualMemUsedByMe, virtualUsedMax);
		physicalUsedMax = max(physMemUsedByMe, physicalUsedMax);

		data.virtual_usedByProcess = virtualMemUsedByMe;
		data.virtual_usedByProcess_max = virtualUsedMax;
		data.physical_usedByProcess = physMemUsedByMe;
		data.physical_usedByProcess_max = physicalUsedMax;
	}


	return data;
}

static ULARGE_INTEGER lastCPU, lastSysCPU, lastUserCPU;
static int numProcessors;
static HANDLE self;
static bool initialized = false;

void init() {
	SYSTEM_INFO sysInfo;
	FILETIME ftime, fsys, fuser;

	GetSystemInfo(&sysInfo);
	numProcessors = sysInfo.dwNumberOfProcessors;

	GetSystemTimeAsFileTime(&ftime);
	memcpy(&lastCPU, &ftime, sizeof(FILETIME));

	self = GetCurrentProcess();
	GetProcessTimes(self, &ftime, &ftime, &fsys, &fuser);
	memcpy(&lastSysCPU, &fsys, sizeof(FILETIME));
	memcpy(&lastUserCPU, &fuser, sizeof(FILETIME));

	initialized = true;
}

CpuData getCpuData() {
	FILETIME ftime, fsys, fuser;
	ULARGE_INTEGER now, sys, user;
	double percent;

	if (!initialized) {
		init();
	}

	GetSystemTimeAsFileTime(&ftime);
	memcpy(&now, &ftime, sizeof(FILETIME));

	GetProcessTimes(self, &ftime, &ftime, &fsys, &fuser);
	memcpy(&sys, &fsys, sizeof(FILETIME));
	memcpy(&user, &fuser, sizeof(FILETIME));
	percent = (sys.QuadPart - lastSysCPU.QuadPart) +
		(user.QuadPart - lastUserCPU.QuadPart);
	percent /= (now.QuadPart - lastCPU.QuadPart);
	percent /= numProcessors;
	lastCPU = now;
	lastUserCPU = user;
	lastSysCPU = sys;

	CpuData data;
	data.numProcessors = numProcessors;
	data.usage = percent * 100.0;

	return data;
}
#else
#include <sys/sysinfo.h>

static size_t VmSwapPeek = 0;

MemoryData getMemoryData()
{
	MemoryData data;

	{
		// Get system status
		{
			struct sysinfo s;
			if(sysinfo(&s) == 0)
			{
				data.virtual_total = s.totalswap;
				data.virtual_used = s.totalswap - s.freeswap;

				data.physical_total = s.totalram;
				data.physical_used = s.totalram - s.freeram;
				// data.physical_used = s.totalram - (s.freeram + s.bufferram + s.sharedram);
			}
		}

		// Get current program status
		{
			std::ifstream statusFile("/proc/self/status");
			if(statusFile.is_open())
			{
				// Read the file line by line
				std::string line;
				while (std::getline(statusFile, line))
				{
					// Find the line containing "VmRSS:"
					if (line.find("VmRSS:") != std::string::npos) {
						// Extract the memory usage value
						size_t pos = line.find(':');
						std::string value = line.substr(pos + 1);

						// Convert the memory usage to an integer
						try
						{
							data.physical_usedByProcess	= std::stoll(value);
						} catch (...)
						{
							data.physical_usedByProcess = 0;
						}
					}

					// Find the line containing "VmPeak:"
					if (line.find("VmPeak:") != std::string::npos) {
						// Extract the memory usage value
						size_t pos = line.find(':');
						std::string value = line.substr(pos + 1);

						// Convert the memory usage to an integer
						try
						{
							data.physical_usedByProcess_max	= std::stoll(value);
						} catch (...)
						{
							data.physical_usedByProcess_max = 0;
						}
					}

					// Find the line containing "VmSwap:"
					if (line.find("VmSwap:") != std::string::npos) {
						// Extract the memory usage value
						size_t pos = line.find(':');
						std::string value = line.substr(pos + 1);

						// Convert the memory usage to an integer
						try
						{
							data.virtual_usedByProcess	= std::stoll(value);
						} catch (...)
						{
							data.virtual_usedByProcess = 0;
						}
					}

					data.virtual_usedByProcess_max = std::max(VmSwapPeek, data.virtual_usedByProcess);
					VmSwapPeek = data.virtual_usedByProcess;
				}
				statusFile.close();
			}
		}
	}

	return data;
}

constexpr float f_load = 1.f / (1 << SI_LOAD_SHIFT);
CpuData getCpuData()
{
	CpuData data;

	struct sysinfo s;
	if(sysinfo(&s) == 0 )
	{
		data.numProcessors = get_nprocs();
		data.usage = static_cast<double>((s.loads[0] * f_load * 100.0) / static_cast<double>(get_nprocs()));
	}

	return data;
}
#endif

void printMemoryReport() {

	auto memoryData = getMemoryData();
	double vm = double(memoryData.virtual_usedByProcess) / (1024.0 * 1024.0 * 1024.0);
	double pm = double(memoryData.physical_usedByProcess) / (1024.0 * 1024.0 * 1024.0);

	stringstream ss;
	ss << "memory usage: "
		<< "virtual: " << formatNumber(vm, 1) << " GB, "
		<< "physical: " << formatNumber(pm, 1) << " GB"
		<< endl;

	cout << ss.str();

}

void launchMemoryChecker(int64_t maxMB, double checkInterval) {

	auto interval = std::chrono::milliseconds(int64_t(checkInterval * 1000));

	thread t([maxMB, interval]() {

		static double lastReport = 0.0;
		static double reportInterval = 1.0;
		static double lastUsage = 0.0;
		static double largestUsage = 0.0;

		while (true) {
			auto memdata = getMemoryData();

			// don't print, just query memory in intervals so that the highest value is getting updated.

			//int64_t threshold = maxMB * 1024 * 1024;

			//if (memdata.virtual_usedByProcess > threshold) {

			//	double usage = double(memdata.virtual_usedByProcess) / (1024.0 * 1024.0 * 1024.0);
			//	largestUsage = largestUsage > usage ? largestUsage : usage;

			//	if (lastReport + reportInterval < now() || usage > largestUsage) {

			//		cout << "WARNING: memory checker detected large memory usage: "
			//			<< formatNumber(usage, 2) << " GB"
			//			<< ", largest detected: " << formatNumber(largestUsage, 2) << endl;

			//		lastReport = now();
			//	}

			//}

			using namespace std::chrono_literals;
			std::this_thread::sleep_for(interval);
		}

	});
	t.detach();

}
