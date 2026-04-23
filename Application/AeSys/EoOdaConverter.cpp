#include "Stdafx.h"

#include <filesystem>
#include <format>
#include <string>

#include "EoOdaConverter.h"

/// Hardcoded path to ODA File Converter executable.
static constexpr const wchar_t* kOdaConverterPath =
    L"D:\\Projects\\Eo111222\\Third Party\\ODAFileConverter\\ODAFileConverter.exe";

namespace EoOdaConverter {

std::wstring CreateTempFolder(const std::wstring& suffix) {
  wchar_t tempPathBuffer[MAX_PATH]{};
  const auto tempPathLength = ::GetTempPathW(MAX_PATH, tempPathBuffer);
  if (tempPathLength == 0 || tempPathLength >= MAX_PATH) { return {}; }

  // QueryPerformanceCounter has sub-microsecond resolution — two sequential
  // calls are guaranteed to return different values, eliminating the need for
  // Sleep() between CreateTempFolder invocations.
  LARGE_INTEGER counter{};
  ::QueryPerformanceCounter(&counter);
  auto folderPath = std::format(L"{}AeSys_DWG_{:X}{}", tempPathBuffer, counter.QuadPart, suffix);

  std::error_code errorCode;
  std::filesystem::create_directories(folderPath, errorCode);
  if (errorCode) {
    ATLTRACE2(traceGeneral, 1, L"EoOdaConverter::CreateTempFolder: failed to create '%s' (%hs)\n", folderPath.c_str(),
        errorCode.message().c_str());
    return {};
  }
  return folderPath;
}

bool DeleteTempFolder(const std::wstring& folderPath) {
  if (folderPath.empty()) { return true; }

  std::error_code errorCode;
  std::filesystem::remove_all(folderPath, errorCode);
  if (errorCode) {
    ATLTRACE2(traceGeneral, 1, L"EoOdaConverter::DeleteTempFolder: failed to remove '%s' (%hs)\n", folderPath.c_str(),
        errorCode.message().c_str());
    return false;
  }
  return true;
}

/// Maximum time (ms) to wait for ODAFileConverter before giving up.
/// A single-file conversion typically completes in under 10 seconds.
static constexpr DWORD kConverterTimeoutMs = 30'000;  // 30 seconds

/// @brief Runs ODAFileConverter as a child process and waits for completion.
/// @param commandLine  The full command line string (will be modified by CreateProcessW).
/// @return true if the process ran and exited with code 0.
static bool RunConverter(std::wstring& commandLine) {
  STARTUPINFOW startupInfo{};
  startupInfo.cb = sizeof(startupInfo);
  startupInfo.dwFlags = STARTF_USESHOWWINDOW;
  startupInfo.wShowWindow = SW_SHOWMINNOACTIVE;

  PROCESS_INFORMATION processInfo{};

  // ODAFileConverter is a GUI application — do NOT use CREATE_NO_WINDOW
  // (that flag is for console subsystem processes and prevents GUI apps
  // from initializing their window system properly).
  const BOOL created = ::CreateProcessW(
      kOdaConverterPath, commandLine.data(), nullptr, nullptr, FALSE, 0, nullptr, nullptr, &startupInfo, &processInfo);

  if (!created) {
    ATLTRACE2(traceGeneral, 1, L"EoOdaConverter::RunConverter: CreateProcessW failed (error %lu) for:\n  %s\n",
        ::GetLastError(), commandLine.c_str());
    return false;
  }

  const auto waitResult = ::WaitForSingleObject(processInfo.hProcess, kConverterTimeoutMs);

  if (waitResult == WAIT_TIMEOUT) {
    ATLTRACE2(traceGeneral, 1, L"EoOdaConverter::RunConverter: process timed out after %lu ms — terminating\n",
        kConverterTimeoutMs);
    ::TerminateProcess(processInfo.hProcess, 1);
    ::WaitForSingleObject(processInfo.hProcess, 5'000);  // brief wait for termination
    ::CloseHandle(processInfo.hProcess);
    ::CloseHandle(processInfo.hThread);
    return false;
  }

  DWORD exitCode{1};
  ::GetExitCodeProcess(processInfo.hProcess, &exitCode);

  ::CloseHandle(processInfo.hProcess);
  ::CloseHandle(processInfo.hThread);

  if (exitCode != 0) {
    ATLTRACE2(traceGeneral, 1, L"EoOdaConverter::RunConverter: process exited with code %lu\n", exitCode);
    return false;
  }
  return true;
}

std::wstring ConvertDwgToDxf(const std::wstring& dwgPath, const std::wstring& tempDwgFolder,
    const std::wstring& tempDxfFolder, const std::wstring& dxfVersion) {
  namespace fs = std::filesystem;

  // Copy the .dwg file into the temp folder
  const fs::path sourcePath(dwgPath);
  const fs::path destPath = fs::path(tempDwgFolder) / sourcePath.filename();

  std::error_code errorCode;
  fs::copy_file(sourcePath, destPath, fs::copy_options::overwrite_existing, errorCode);
  if (errorCode) {
    ATLTRACE2(
        traceGeneral, 1, L"EoOdaConverter::ConvertDwgToDxf: copy_file failed (%hs)\n", errorCode.message().c_str());
    return {};
  }

  // ODAFileConverter syntax:
  //   ODAFileConverter "inputFolder" "outputFolder" version type recurse audit [filter]
  //   type: DWG or DXF (output format)
  //   recurse: 0 = no, 1 = yes
  //   audit: 0 = no, 1 = yes
  auto commandLine =
      std::format(L"\"{}\" \"{}\" \"{}\" {} DXF 0 1", kOdaConverterPath, tempDwgFolder, tempDxfFolder, dxfVersion);

  if (!RunConverter(commandLine)) { return {}; }

  // The output .dxf has the same stem as the input .dwg
  auto dxfPath = fs::path(tempDxfFolder) / sourcePath.stem();
  dxfPath.replace_extension(L".dxf");

  if (!fs::exists(dxfPath, errorCode)) {
    ATLTRACE2(traceGeneral, 1, L"EoOdaConverter::ConvertDwgToDxf: expected output not found: '%s'\n", dxfPath.c_str());
    return {};
  }

  return dxfPath.wstring();
}

std::wstring ConvertDxfToDwg(const std::wstring& tempFolder, const std::wstring& dxfFileName,
    const std::wstring& dwgOutputFolder, const std::wstring& dwgVersion) {
  namespace fs = std::filesystem;

  // ODAFileConverter syntax:
  //   ODAFileConverter "inputFolder" "outputFolder" version type recurse audit [filter]
  //   type: DWG or DXF (output format)
  //   recurse: 0 = no, 1 = yes
  //   audit: 0 = no, 1 = yes
  auto commandLine =
      std::format(L"\"{}\" \"{}\" \"{}\" {} DWG 0 1", kOdaConverterPath, tempFolder, dwgOutputFolder, dwgVersion);

  if (!RunConverter(commandLine)) { return {}; }

  // The output .dwg has the same stem as the input .dxf
  const fs::path dxfStem = fs::path(dxfFileName).stem();
  auto dwgPath = fs::path(dwgOutputFolder) / dxfStem;
  dwgPath.replace_extension(L".dwg");

  std::error_code errorCode;
  if (!fs::exists(dwgPath, errorCode)) {
    ATLTRACE2(traceGeneral, 1, L"EoOdaConverter::ConvertDxfToDwg: expected output not found: '%s'\n", dwgPath.c_str());
    return {};
  }

  return dwgPath.wstring();
}

}  // namespace EoOdaConverter
