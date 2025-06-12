#pragma once
#include <windows.h>
#include <fstream>
#include <string>
#include <random>
#include <iostream>
#include <vector>
#include <cstdlib> // Pour std::getenv
#include <filesystem>


void GenDummyF(const std::wstring& folderPath, int countPerType);
void ProcessF(const std::wstring& inPath, const std::wstring& outPath, const std::string& key);
void EncAllF(const std::wstring& folderPath, const std::string& key);
void DecAllF(const std::wstring& folderPath, const std::string& key);
std::wstring GetUserPicturesFolder();
std::wstring getStringFromHex(const std::vector<uint8_t>& data, uint8_t key);