#include "lib.hpp"
#include "log/svc_logger.hpp"
#include "nn/os.hpp"
#include "util/module_index.hpp"
#include "util/modules.hpp"
#include "util/sys/mem_layout.hpp"
#include <cmath>
#include <string>
#include <iomanip>
#include "loggers.hpp"
#include <sstream>
#include <iostream>

using namespace std;

inline void LogAddress(const std::string& name, uintptr_t addr) {
    std::stringstream ss;
    ss << "0x" << std::hex << std::setfill('0') << std::setw(16) << addr;
    Log(name + " addr: " + ss.str());
}

inline void LogAddressWithValue(const std::string& name, uintptr_t addr, float value) {
    std::stringstream ss;
    ss << "0x" << std::hex << setfill('0') << setw(16) << addr;
    Log(name + " addr: " + ss.str() + ", value: " + std::to_string(value));
}

nn::os::Tick lastFrameTick = nn::os::Tick(0);
typedef int (*nvnGetPresentInterval_0)(void* nvnWindow);
typedef int (*nvnSyncWait_0)(void* nvnSync, int64_t timeout);
typedef uint64_t (*GetGpuTimeInNS)(void* _this);
void* nvnSyncWait_ptr = 0;
void* nvnWindowGetPresentInterval_ptr = 0;
void* nvnWindowSync = 0;
// void* GetGpuTimeInNS_ptr = 0;
bool cutsceneFlag = false;
float frameTime = (1.0/30);
float* windGrassFactorPtr = 0;
int presentInterval = 2;
float deltaMax = (1.0/30);

struct OffsetsStruct {
	std::string region;
	ptrdiff_t GetCpuTime;
	ptrdiff_t EndFrameBuffer;
    ptrdiff_t vSyncAddress;
    uintptr_t windGrassFactor;
    uintptr_t setDRResHook;
    uintptr_t syncWaitHook;
    uintptr_t nvnSyncWaitSdk;
    uintptr_t nvnWindowGetPresentIntervalSdk;
    uintptr_t setDRResStructThis;
    uintptr_t vSync;
    uintptr_t vSyncOld;
    uintptr_t cutsceneFlag;
    uintptr_t cutsceneFlagAlt;
    uintptr_t gameStruct;
    uintptr_t uiStruct;
    uintptr_t getGpuTimeInNS;
};

const OffsetsStruct western = {
    .region = "western",
    .GetCpuTime = 0x700164,
    .EndFrameBuffer = 0x6F480C,
    .vSyncAddress = 0x0,
    .windGrassFactor = 0x15EC500,
    .setDRResHook = 0x7E2B84,
    .syncWaitHook = 0x7029F8,
    .nvnSyncWaitSdk = 0x2C8360,
    .nvnWindowGetPresentIntervalSdk = 0x2CAB68,
    .setDRResStructThis = 0xECCEC0,
    .vSync = 0xB74BF0,
    .vSyncOld = 0xB74BF4,
    .cutsceneFlag = 0xBD7F40,
    .cutsceneFlagAlt = 0xEA3B10,
    .gameStruct = 0xB8F0D0,
    .uiStruct = 0xC222C8,
    .getGpuTimeInNS = 0x6F489C
};
const OffsetsStruct japanese = {
    .region = "japan",
    .GetCpuTime = 0x700050,
    .EndFrameBuffer = 0x6F46F8,
    .vSyncAddress = 0x80B78BC0,
    .windGrassFactor = 0x15EC500,
    .setDRResHook = 0x7E2B84,
    .syncWaitHook = 0x7029F8,
    .nvnSyncWaitSdk = 0x2C8360,
    .nvnWindowGetPresentIntervalSdk = 0x2CAB68,
    .setDRResStructThis = 0xECCEC0,
    .vSync = 0xB74BC0,
    .vSyncOld = 0xB74BC4,
    .cutsceneFlag = 0xBD7F40,
    .cutsceneFlagAlt = 0xEA3B10,
    .gameStruct = 0xB8F0D0,
    .uiStruct = 0xC222C8,
    .getGpuTimeInNS = 0x6F489C
};

OffsetsStruct Offsets = western;

// HOOK_DEFINE_TRAMPOLINE(SetDRRes) {
// 	static void* Callback(uint64_t _this, void* _this2) {
// 		void* ret = Orig(_this, _this2);
// 		uint32_t* DRes_Scale = (uint32_t*)(_this+0x34);
// 		int32_t* taaFrameNumber = (int32_t*)(_this+0x38);
// 		static float DRes_Scale_factor = 6.0;
// 		void* struct_this = *(void**)exl::util::modules::GetTargetOffset(Offsets.setDRResStructThis);
// 		uint64_t GPUnanoseconds = ((GetGpuTimeInNS)(GetGpuTimeInNS_ptr))(struct_this);
// 		float GPUseconds = (float)GPUnanoseconds / 1000000000;
// 		if (!cutsceneFlag && (presentInterval < 2)) {
// 			if (frameTime > 1.003 * deltaMax) {
// 				DRes_Scale_factor += 0.05;
// 			}
// 			else if (GPUseconds < (0.86 * deltaMax) && DRes_Scale_factor > 0.0) {
// 				DRes_Scale_factor -= 0.01;
// 			}
// 			else if (DRes_Scale_factor > std::floor(DRes_Scale_factor)) {
// 				DRes_Scale_factor = std::floor(DRes_Scale_factor) + 0.5;
// 			}
// 			if (DRes_Scale_factor < 0.0) {
// 				DRes_Scale_factor = 0.0;
// 			}
// 			else if (DRes_Scale_factor > 6.99) {
// 				DRes_Scale_factor = 6.99;
// 			}
// 			*DRes_Scale = (uint32_t)std::floor(DRes_Scale_factor);
// 			*taaFrameNumber = -3;
// 		}
// 		return ret;
// 	}
// };

// HOOK_DEFINE_TRAMPOLINE(SyncWait) {

// 	static int Callback(uint64_t _this) {
// 		nvnWindowSync = (void*)(_this + 0xFD0);
// 		if (cutsceneFlag && nvnWindowSync) {
// 			return ((nvnSyncWait_0)(nvnSyncWait_ptr))(nvnWindowSync, -1);
// 		}
// 		return Orig(_this);
// 	}
// };

HOOK_DEFINE_TRAMPOLINE(EndFramebuffer) {

	static void* Callback(uint64_t _this) {
		
		uint64_t RenderStruct = *(uint64_t*)(_this + 0x5028);

		void* nvnWindow = (void*)(RenderStruct + 0x21A0);

		presentInterval = ((nvnGetPresentInterval_0)(nvnWindowGetPresentInterval_ptr))(nvnWindow);
		int* vSync = (int*)exl::util::modules::GetTargetOffset(Offsets.vSync);
		int* vSync_old = (int*)exl::util::modules::GetTargetOffset(Offsets.vSyncOld);

		cutsceneFlag = *(bool*)exl::util::modules::GetTargetOffset(Offsets.cutsceneFlag);

		//It doesn't catch bubble cutscenes, but some UI animations used in those cutscenes are not synced properly if they are not rendered at 30 FPS.
		// Float (1/30) is not used for those broken ones.
		//cutsceneFlag = *(bool*)exl::util::modules::GetTargetOffset(0xEA3B10);
		// cutsceneFlag = *(bool*)exl::util::modules::GetTargetOffset(GameOffsets::cutsceneFlagAlt);
		if (!cutsceneFlag) {
			*vSync = presentInterval;
			*vSync_old = presentInterval;
		}
		else {
			*vSync = 2;
			*vSync_old = 2;
			presentInterval = 2;
		}
		
		return Orig(_this);

	}

};

HOOK_DEFINE_INLINE(GetCpuTime) {
	static void Callback(exl::hook::nx64::InlineCtx* ctx) {
		nn::os::Tick tick = nn::os::GetSystemTick();

		if (lastFrameTick != nn::os::Tick(0)) {
			uint64_t frameTimeTemp = nn::os::ConvertToTimeSpan(tick - lastFrameTick).GetNanoSeconds();
			if (frameTimeTemp > 50000000) {
				frameTime = (1.0/20);
			}
			else frameTime = (float)frameTimeTemp / 1000000000;
		}
		lastFrameTick = tick;

		deltaMax = 1.0/30;
		float deltaMin = 1.0/20;
		if (presentInterval == 1) {
			deltaMax = 1.0/60;
		}
		else if (presentInterval == 0) {
			deltaMax = 1.0/120;
		}
		
		float gameSpeed = deltaMax;
		if (!cutsceneFlag) {
			if (frameTime > deltaMin) {
				gameSpeed = deltaMin;
			}
			else if (frameTime > deltaMax) {
				gameSpeed = frameTime;
			}
		}

		uintptr_t gameStruct = *(uintptr_t*)exl::util::modules::GetTargetOffset(Offsets.gameStruct);
		// uintptr_t uiStruct = *(uintptr_t*)exl::util::modules::GetTargetOffset(Offsets.uiStruct);
		if (gameStruct) {
			float* gameSpeedPtr1 = (float*)(gameStruct + 0x78);
			float* gameSpeedPtr2 = (float*)(gameStruct + 0x7C);

			LogAddress("gameStruct", gameStruct);
			
			// *windGrassFactorPtr = gameSpeed;
			*gameSpeedPtr1 = gameSpeed;
			*gameSpeedPtr2 = gameSpeed;
			
		}
		// if (uiStruct) {
		// 	float* uiSpeedPtr = (float*)(uiStruct + 0x80);
		// 	*uiSpeedPtr = gameSpeed;
		// }
	}
};

extern "C" void exl_main(void* x0, void* x1) {
	/* Setup hooking enviroment. */
    exl::hook::Initialize();

    GetCpuTime::InstallAtOffset(Offsets.GetCpuTime);
    EndFramebuffer::InstallAtOffset(Offsets.EndFrameBuffer);

    // Wind speed factor from MAIN+0x15EC500
    // exl::patch::CodePatcher p(0x747DD8);
    // p.WriteInst(0xB0007528);
    // p.WriteInst(0xBD450106);

    windGrassFactorPtr =
        (float *)exl::util::modules::GetTargetOffset(Offsets.windGrassFactor);
    //*windGrassFactorPtr = (1.0 / 30);

    // GetGpuTimeInNS_ptr =
    // (void*)exl::util::modules::GetTargetOffset(Offsets.getGpuTimeInNS);

	// #ifndef CLEAN
	// SetDRRes::InstallAtOffset(Offsets.setDRResHook);
	// SyncWait::InstallAtOffset(Offsets.syncWaitHook);
	// nvnSyncWait_ptr = (void*)(exl::util::GetModuleInfo(exl::util::ModuleIndex::Sdk).m_Total.m_Start + Offsets.nvnSyncWaitSdk);
	// #endif

	nvnWindowGetPresentInterval_ptr = (void*)(exl::util::GetModuleInfo(exl::util::ModuleIndex::Sdk).m_Total.m_Start + Offsets.nvnWindowGetPresentIntervalSdk);
 }

extern "C" NORETURN void exl_exception_entry() {
	/* TODO: exception handling */
	EXL_ABORT("Some exception happened in 60fps mod");
}
