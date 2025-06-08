#pragma once
#include <windows.h>
#include <fstream>
#include <string>
#include <random>
#include <iostream>
#include <vector>
#include <cstdlib> // Pour std::getenv



void GenerateDummyFiles(const std::wstring& folderPath, int countPerType);