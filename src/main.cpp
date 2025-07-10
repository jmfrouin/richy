#include <iostream>
#include "core/def.h"
#include "core/Configuration.h"

int main() {
    std::cout << FULLNAME << std::endl;
    
    // Création de l'instance de configuration
    Richy::CConfiguration config;
    
    // Chargement de la configuration
    std::cout << "Loading configuration..." << std::endl;
    Richy::EErrors loadResult = config.Load();
    
    if (loadResult == Richy::eNoError) {
        std::cout << GREEN "Configuration loaded successfully!" STOP << std::endl;
        std::cout << "Host: " << config.mHost << std::endl;
        std::cout << "API Key: " << (config.mAPIKey.empty() ? "Not set" : "***") << std::endl;
        std::cout << "API Secret: " << (config.mAPISecret.empty() ? "Not set" : "***") << std::endl;
    } else if (loadResult == Richy::eCannotOpenFile) {
        std::cout << YELLOW "Configuration file not found, using defaults" STOP << std::endl;
        std::cout << "Host: " << config.mHost << std::endl;
        std::cout << "API Key: " << (config.mAPIKey.empty() ? "Not set" : "***") << std::endl;
        std::cout << "API Secret: " << (config.mAPISecret.empty() ? "Not set" : "***") << std::endl;
    } else {
        std::cout << RED "Error loading configuration!" STOP << std::endl;
        return 1;
    }
    
    // Exemple d'utilisation : modification des paramètres si nécessaire
    if (config.mHost == "localhost") {
        std::cout << CYAN "Setting default configuration values..." STOP << std::endl;
        config.mHost = "api.example.com";
        config.mAPIKey = "your_api_key_here";
        config.mAPISecret = "your_api_secret_here";
        
        // Sauvegarde de la configuration
        std::cout << "Saving configuration..." << std::endl;
        Richy::EErrors saveResult = config.Save();
        
        if (saveResult == Richy::eNoError) {
            std::cout << GREEN "Configuration saved successfully!" STOP << std::endl;
        } else {
            std::cout << RED "Error saving configuration!" STOP << std::endl;
            return 1;
        }
    }
    
    std::cout << BLUE "Application initialized successfully!" STOP << std::endl;
    
    return 0;
}