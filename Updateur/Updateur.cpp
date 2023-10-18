#include <iostream>
#include <curl/curl.h>
#include <string>
#include <fstream>
#include <json/json.h>
#include <direct.h>
#pragma comment(lib, "Version.lib")

std::string filePath = "C:\\GestionParc\\";
std::string fileName = "GestionParc.exe";

std::string getVersion() {
    std::wstring name = L"C:\\GestionParc\\GestionParc.exe";
    std::string version;
    LPCWSTR lpFileName = name.c_str();

    DWORD handle;
    DWORD size = GetFileVersionInfoSize(lpFileName, &handle);

    if (size)
    {
        BYTE* buffer = new BYTE[size];
        if (GetFileVersionInfo(lpFileName, handle, size, buffer))
        {
            VS_FIXEDFILEINFO* fileInfo;
            UINT fileInfoSize;
            if (VerQueryValueA(buffer, "\\", (LPVOID*)&fileInfo, &fileInfoSize))
            {
                std::cout << "Version de l'exe : " << HIWORD(fileInfo->dwFileVersionMS) << "." << LOWORD(fileInfo->dwFileVersionMS) << "." << HIWORD(fileInfo->dwFileVersionLS) << "." << LOWORD(fileInfo->dwFileVersionLS) << std::endl;
                version = std::to_string(HIWORD(fileInfo->dwFileVersionMS)) + "." + std::to_string(LOWORD(fileInfo->dwFileVersionMS)) + "." + std::to_string(HIWORD(fileInfo->dwFileVersionLS)) + "." + std::to_string(LOWORD(fileInfo->dwFileVersionLS));
            }
            else
            {
                std::cerr << "Erreur lors de la recupération de la version de l'executable" << std::endl;
            }
        }
        else
        {
            std::cerr << "Erreur lors de l'ouverture du fichier executable" << std::endl;
        }

        delete[] buffer;
    }
    else
    {
        std::cerr << "Erreur lors de la recuperation de la taille du fichier executable" << std::endl;
    }

    return version;
    
}

static size_t WriteCallback(void* contents, size_t size, size_t nmemb, void* userp)
{
    ((std::string*)userp)->append((char*)contents, size * nmemb);
    return size * nmemb;
}

size_t WriteCallbackOf(void* contents, size_t size, size_t nmemb, void* userp)
{
    size_t realsize = size * nmemb;
    std::ofstream* file = static_cast<std::ofstream*>(userp);
    file->write(static_cast<char*>(contents), realsize);
    return realsize;
}

int main() {
    curl_global_init(CURL_GLOBAL_ALL);
    CURL* curl = curl_easy_init();
    if (curl) {
        std::string version;

        curl_easy_setopt(curl, CURLOPT_URL, "https://raw.githubusercontent.com/Mikawa-Sama/gestionParc/main/Version.txt");
        curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, WriteCallback);
        curl_easy_setopt(curl, CURLOPT_WRITEDATA, &version);
        CURLcode res = curl_easy_perform(curl);

        std::cout << "version git : " << version << std::endl;

        if (res != CURLE_OK) {
            std::cerr << "Erreur lors de l'envoi de la requete HTTP: " << curl_easy_strerror(res) << std::endl;
        }
        curl_easy_cleanup(curl);
        std::string gitVersion = getVersion();
        version.erase(std::remove_if(version.begin(), version.end(), [](char c) {return c == '\n'; }), version.end());
        gitVersion.erase(std::remove_if(gitVersion.begin(), gitVersion.end(), [](char c) {return c == '\n'; }), gitVersion.end());
        if (version != gitVersion) {
            std::cout << version << " - " << gitVersion << std::endl;
            std::cout << "Nouvelle mise a jour disponible !" << std::endl;
            std::cout << "Le telechargement va demarrer, veuillez ne pas fermer la fenetre !" << std::endl;

            CURLcode res;
            curl_global_init(CURL_GLOBAL_DEFAULT);
            curl = curl_easy_init();
            if (curl) {
                std::string url = "https://api.github.com/repos/Mikawa-Sama/gestionParc/contents/";
                curl_easy_setopt(curl, CURLOPT_URL, url.c_str());
                curl_easy_setopt(curl, CURLOPT_USERAGENT, "Awesome - Octocat - App");
                curl_easy_setopt(curl, CURLOPT_FOLLOWLOCATION, 1L);
                curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, WriteCallback);
                std::string response;
                curl_easy_setopt(curl, CURLOPT_WRITEDATA, &response);
                res = curl_easy_perform(curl);

                if (res != CURLE_OK) {
                    std::cerr << "Erreur lors de l'envoi de la requete HTTP: " << curl_easy_strerror(res) << std::endl;
                    return 1;
                }

                Json::Value root;
                //std::cout << response << std::endl;
                Json::CharReaderBuilder builder;
                Json::CharReader* reader = builder.newCharReader();
                std::string errors;
                bool parsingSuccessful = reader->parse(response.c_str(), response.c_str() + response.size(), &root, &errors);
                delete reader;
                //std::cout << root << std::endl;
                if (!parsingSuccessful) {
                    std::cerr << "Erreur JSON: " << errors << std::endl;
                    return 1;
                }

                for (const auto& file : root) {
                    if (file["name"] != ".gitignore") {
                        std::string file_url = file["download_url"].asString();
                        std::string file_name = file["name"].asString();
                        std::string output_file = filePath + file_name;
                        std::cout << "Telechargement de " << file_name << "..." << std::endl;

                        curl_easy_setopt(curl, CURLOPT_URL, file_url.c_str());
                        std::cout << file_url.c_str() << std::endl;
                        curl_easy_setopt(curl, CURLOPT_FOLLOWLOCATION, 1L);
                        curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, WriteCallbackOf);
                        std::ofstream outfile(output_file, std::ofstream::binary);
                        curl_easy_setopt(curl, CURLOPT_WRITEDATA, &outfile);
                        CURLcode res2 = curl_easy_perform(curl);

                        if (res2 != CURLE_OK) {
                            std::cerr << "Erreur requete HTTP: " << curl_easy_strerror(res) << std::endl;
                            outfile.close();
                            return 1;
                        }
                        outfile.close();
                    }
                }
                curl_easy_cleanup(curl);
            }
            curl_global_cleanup();

            std::cout << "Mise a jour terminer ! Lancement du logiciel..." << std::endl;
            int result = _chdir(filePath.c_str());
            if (result == 0) {
                system(fileName.c_str());
            }
        }
        else {
            std::cout << "Aucune nouvelle mise a jour de disponible, lancement du logiciel..." << std::endl;

            int result = _chdir(filePath.c_str());
            if (result == 0) {
                system(fileName.c_str());
            }
        }
        
    }
    return 0;
}