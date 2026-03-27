#pragma once

/// @file EoOdaConverter.h
/// @brief Utility helpers for DWG↔DXF conversion via ODA File Converter.
///
/// ODA File Converter operates on *folders*, not individual files.  Every conversion
/// therefore follows the same pattern:
///   1. Create an isolated temporary folder.
///   2. Place the single input file in that folder (copy or export).
///   3. Run ODAFileConverter with source-folder = temp-folder, output-folder = temp-folder
///      (or a separate target folder for the final result).
///   4. Read the converted file.
///   5. Delete the temporary folder.
///
/// All file-system operations (folder creation, file copy, recursive delete) are
/// sub-millisecond for the single-file case.

#include <string>

namespace EoOdaConverter {

/// @brief Creates a uniquely named temporary folder under %TEMP%.
/// @param suffix  Optional suffix appended to the folder name (e.g. L"_dwg", L"_dxf")
///               for human-readable disambiguation when creating folder pairs.
/// @return Full path of the created folder (e.g. "C:\Users\…\Temp\AeSys_DWG_<qpc>_dwg").
///         Empty string on failure.
[[nodiscard]] std::wstring CreateTempFolder(const std::wstring& suffix = L"");

/// @brief Recursively deletes a folder and all of its contents.
/// @param folderPath Absolute path to the folder to delete.
/// @return true if the folder was successfully removed (or did not exist).
bool DeleteTempFolder(const std::wstring& folderPath);

/// @brief Converts a single .dwg file to .dxf using ODA File Converter.
///
/// The .dwg file is copied into @p tempFolder, ODAFileConverter runs with
/// source = @p tempFolder and output = @p tempFolder, producing a .dxf
/// alongside the .dwg copy.
///
/// @param dwgPath      Absolute path to the source .dwg file.
/// @param tempDwgFolder   Absolute path to the (empty) temporary dwg folder.
/// @param tempDxfFolder   Absolute path to the (empty) temporary dxf folder.
/// @param dxfVersion   ODA version string for the output DXF (e.g. "ACAD2018", "ACAD2013").
/// @return Absolute path to the resulting .dxf file, or empty string on failure.
[[nodiscard]] std::wstring ConvertDwgToDxf(const std::wstring& dwgPath, const std::wstring& tempDwgFolder,
    const std::wstring& tempDxfFolder, const std::wstring& dxfVersion = L"ACAD2018");

/// @brief Converts a single .dxf file to .dwg using ODA File Converter.
///
/// The .dxf file is expected to already exist in @p tempFolder.  ODAFileConverter
/// runs with source = @p tempFolder and output = @p dwgOutputFolder.
///
/// @param tempFolder       Absolute path to the folder containing the .dxf file.
/// @param dxfFileName      The .dxf file name (not full path) inside tempFolder.
/// @param dwgOutputFolder  Absolute path to the folder that receives the .dwg file.
/// @param dwgVersion       ODA version string for the output DWG (e.g. "ACAD2018", "ACAD2013").
/// @return Absolute path to the resulting .dwg file, or empty string on failure.
[[nodiscard]] std::wstring ConvertDxfToDwg(const std::wstring& tempFolder, const std::wstring& dxfFileName,
    const std::wstring& dwgOutputFolder, const std::wstring& dwgVersion = L"ACAD2018");

}  // namespace EoOdaConverter
