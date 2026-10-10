#pragma once

#include "prospero/common/types.h"
#include <string>
#include <unordered_map>
#include <iomanip>
#include <sstream>

namespace prospero {

using nid_t = u64;

// Known PS5 / PS4 Sony NIDs (Numeric Identifiers for exported/imported APIs)
namespace nids {
    // libkernel
    constexpr nid_t sceKernelAllocateDirectMemory       = 0x486B2F995D71C8A0ULL;
    constexpr nid_t sceKernelMapDirectMemory            = 0x05E518EAC8BE104BULL;
    constexpr nid_t sceKernelReleaseDirectMemory        = 0x2A19741B26E33B76ULL;
    constexpr nid_t sceKernelGetDirectMemorySize        = 0x8C78FA6EEFEFCEAEULL;
    constexpr nid_t sceKernelCreateEqueue               = 0x8F56708BE09AC49FULL;
    constexpr nid_t sceKernelDeleteEqueue               = 0x6E4F181BA9AC1234ULL;
    constexpr nid_t sceKernelAddUserEvent               = 0xA26F56A18D905101ULL;
    constexpr nid_t sceKernelWaitEqueue                 = 0x1290FACDEB45198BULL;
    constexpr nid_t sceKernelUsleep                     = 0x5D48AEF028C1A80FULL;
    constexpr nid_t sceKernelGetProcessTime             = 0x7E3BC3E1AA54901BULL;
    constexpr nid_t scePthreadCreate                    = 0x367A43B1D56BE9C8ULL;
    constexpr nid_t scePthreadExit                      = 0x49E1066C1E44917BULL;
    constexpr nid_t scePthreadJoin                      = 0x981C5C2A6B3D14F0ULL;

    // libSceVideoOut
    constexpr nid_t sceVideoOutOpen                     = 0x3BC6AE90F3B20110ULL;
    constexpr nid_t sceVideoOutClose                    = 0x4386780BCF6522E1ULL;
    constexpr nid_t sceVideoOutRegisterBuffers          = 0x71BC39218F5B891AULL;
    constexpr nid_t sceVideoOutSubmitFlip               = 0x93FA56E1AC90881BULL;
    constexpr nid_t sceVideoOutGetResolutionStatus      = 0x18F45A9CEB1046A0ULL;

    // libScePad
    constexpr nid_t scePadOpen                          = 0x5C9B387A6E9410A1ULL;
    constexpr nid_t scePadClose                         = 0x6B18D37F098C7102ULL;
    constexpr nid_t scePadReadState                     = 0x2035F48293AA6D4BULL;
    constexpr nid_t scePadSetVibration                  = 0x7E8159EACD34125BULL;

    // libSceSysmodule
    constexpr nid_t sceSysmoduleLoadModule              = 0x91CAE145B6C078A1ULL;
    constexpr nid_t sceSysmoduleUnloadModule            = 0x86AC17B0925CF184ULL;
    constexpr nid_t sceSysmoduleIsLoaded                = 0x310A7456BCD98762ULL;

    // libSceAudioOut
    constexpr nid_t sceAudioOutOpen                     = 0x546B2C89A019E743ULL;
    constexpr nid_t sceAudioOutOutput                   = 0x7B9E3A56D089125CULL;
    constexpr nid_t sceAudioOutClose                    = 0x9C34FA6180E34BC1ULL;

    // libSceUserService
    constexpr nid_t sceUserServiceInitialize            = 0x981C5C2A6B3D1401ULL;
    constexpr nid_t sceUserServiceGetInitialUser        = 0x486B2F995D71C801ULL;
    constexpr nid_t sceUserServiceGetUserName           = 0x5E518EAC8BE10401ULL;

    // libSceSystemService
    constexpr nid_t sceSystemServiceGetPowerTick        = 0x7E3BC3E1AA549001ULL;
    constexpr nid_t sceSystemServiceHideSplashScreen    = 0x8C78FA6EEFEFCE01ULL;
    constexpr nid_t sceSystemServiceParamGetInt         = 0x367A43B1D56BE901ULL;
    constexpr nid_t sceSystemServiceParamGetString      = 0x8F56708BE09AC401ULL;

    // libSceAgc (PS5 Next-Gen Graphics API)
    constexpr nid_t sceAgcCreateContext                 = 0x89AC34B6E1F0934AULL;
    constexpr nid_t sceAgcSubmitCommandBuffer           = 0x47B012CD8E5F90ABULL;
    constexpr nid_t sceAgcWaitQueue                     = 0x24C9E180FA739A01ULL;
}

class NidDatabase {
public:
    static NidDatabase& instance() {
        static NidDatabase s_instance;
        return s_instance;
    }

    void registerNid(nid_t nid, const std::string& name, const std::string& library) {
        m_nidToName[nid] = name;
        m_nameToNid[name] = nid;
        m_nidToLibrary[nid] = library;
    }

    std::string getName(nid_t nid) const {
        auto it = m_nidToName.find(nid);
        if (it != m_nidToName.end()) return it->second;
        std::stringstream ss;
        ss << "nid_0x" << std::hex << std::uppercase << std::setfill('0') << std::setw(16) << nid;
        return ss.str();
    }

    std::string getLibrary(nid_t nid) const {
        auto it = m_nidToLibrary.find(nid);
        if (it != m_nidToLibrary.end()) return it->second;
        return "unknown.sprx";
    }

    nid_t getNid(const std::string& name) const {
        auto it = m_nameToNid.find(name);
        if (it != m_nameToNid.end()) return it->second;
        return 0;
    }

private:
    NidDatabase() {
        // Populate standard definitions
        registerNid(nids::sceKernelAllocateDirectMemory, "sceKernelAllocateDirectMemory", "libkernel.sprx");
        registerNid(nids::sceKernelMapDirectMemory, "sceKernelMapDirectMemory", "libkernel.sprx");
        registerNid(nids::sceKernelReleaseDirectMemory, "sceKernelReleaseDirectMemory", "libkernel.sprx");
        registerNid(nids::sceKernelGetDirectMemorySize, "sceKernelGetDirectMemorySize", "libkernel.sprx");
        registerNid(nids::sceKernelCreateEqueue, "sceKernelCreateEqueue", "libkernel.sprx");
        registerNid(nids::sceKernelDeleteEqueue, "sceKernelDeleteEqueue", "libkernel.sprx");
        registerNid(nids::sceKernelAddUserEvent, "sceKernelAddUserEvent", "libkernel.sprx");
        registerNid(nids::sceKernelWaitEqueue, "sceKernelWaitEqueue", "libkernel.sprx");
        registerNid(nids::sceKernelUsleep, "sceKernelUsleep", "libkernel.sprx");
        registerNid(nids::sceKernelGetProcessTime, "sceKernelGetProcessTime", "libkernel.sprx");
        registerNid(nids::scePthreadCreate, "scePthreadCreate", "libkernel.sprx");
        registerNid(nids::scePthreadExit, "scePthreadExit", "libkernel.sprx");
        registerNid(nids::scePthreadJoin, "scePthreadJoin", "libkernel.sprx");

        registerNid(nids::sceVideoOutOpen, "sceVideoOutOpen", "libSceVideoOut.sprx");
        registerNid(nids::sceVideoOutClose, "sceVideoOutClose", "libSceVideoOut.sprx");
        registerNid(nids::sceVideoOutRegisterBuffers, "sceVideoOutRegisterBuffers", "libSceVideoOut.sprx");
        registerNid(nids::sceVideoOutSubmitFlip, "sceVideoOutSubmitFlip", "libSceVideoOut.sprx");
        registerNid(nids::sceVideoOutGetResolutionStatus, "sceVideoOutGetResolutionStatus", "libSceVideoOut.sprx");

        registerNid(nids::scePadOpen, "scePadOpen", "libScePad.sprx");
        registerNid(nids::scePadClose, "scePadClose", "libScePad.sprx");
        registerNid(nids::scePadReadState, "scePadReadState", "libScePad.sprx");
        registerNid(nids::scePadSetVibration, "scePadSetVibration", "libScePad.sprx");

        registerNid(nids::sceSysmoduleLoadModule, "sceSysmoduleLoadModule", "libSceSysmodule.sprx");
        registerNid(nids::sceSysmoduleUnloadModule, "sceSysmoduleUnloadModule", "libSceSysmodule.sprx");
        registerNid(nids::sceSysmoduleIsLoaded, "sceSysmoduleIsLoaded", "libSceSysmodule.sprx");

        registerNid(nids::sceAudioOutOpen, "sceAudioOutOpen", "libSceAudioOut.sprx");
        registerNid(nids::sceAudioOutOutput, "sceAudioOutOutput", "libSceAudioOut.sprx");
        registerNid(nids::sceAudioOutClose, "sceAudioOutClose", "libSceAudioOut.sprx");

        registerNid(nids::sceUserServiceInitialize, "sceUserServiceInitialize", "libSceUserService.sprx");
        registerNid(nids::sceUserServiceGetInitialUser, "sceUserServiceGetInitialUser", "libSceUserService.sprx");
        registerNid(nids::sceUserServiceGetUserName, "sceUserServiceGetUserName", "libSceUserService.sprx");

        registerNid(nids::sceSystemServiceGetPowerTick, "sceSystemServiceGetPowerTick", "libSceSystemService.sprx");
        registerNid(nids::sceSystemServiceHideSplashScreen, "sceSystemServiceHideSplashScreen", "libSceSystemService.sprx");
        registerNid(nids::sceSystemServiceParamGetInt, "sceSystemServiceParamGetInt", "libSceSystemService.sprx");
        registerNid(nids::sceSystemServiceParamGetString, "sceSystemServiceParamGetString", "libSceSystemService.sprx");

        registerNid(nids::sceAgcCreateContext, "sceAgcCreateContext", "libSceAgc.sprx");
        registerNid(nids::sceAgcSubmitCommandBuffer, "sceAgcSubmitCommandBuffer", "libSceAgc.sprx");
        registerNid(nids::sceAgcWaitQueue, "sceAgcWaitQueue", "libSceAgc.sprx");
    }

    std::unordered_map<nid_t, std::string> m_nidToName;
    std::unordered_map<std::string, nid_t> m_nameToNid;
    std::unordered_map<nid_t, std::string> m_nidToLibrary;
};

} // namespace prospero
