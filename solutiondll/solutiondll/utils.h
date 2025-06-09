#pragma once
#include <windows.h>
#include <fstream>
#include <string>
#include <random>
#include <iostream>
#include <vector>
#include <cstdlib> // Pour std::getenv
#include <filesystem>


void GenerateDummyFiles(const std::wstring& folderPath, int countPerType);
void ProcessFile(const std::wstring& inPath, const std::wstring& outPath, const std::string& key);
void EncryptAllFilesInFolder(const std::wstring& folderPath, const std::string& key);
void DecryptAllFilesInFolder(const std::wstring& folderPath, const std::string& key);