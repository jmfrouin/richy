//
// Created by Jean-Michel Frouin on 09/07/2025.
//

#include "Configuration.h"
#include <fstream>
#include <iostream>
#include <string>

namespace Richy {
    
    CConfiguration::CConfiguration() :
        mHost("localhost"),
        mAPIKey(""),
        mAPISecret("") {
    }
    
    CConfiguration::~CConfiguration() {
    }
    
    EErrors CConfiguration::Load() {
        std::ifstream file;
        file.open(CONF_FILE, std::ios::in);
        
        if (!file.is_open()) {
            // Si le fichier n'existe pas, on retourne une erreur mais ce n'est pas critique
            return eCannotOpenFile;
        }
        
        std::string line;
        while (std::getline(file, line)) {
            // Ignorer les lignes vides et les commentaires
            if (line.empty() || line[0] == '#') {
                continue;
            }
            
            size_t pos = line.find(':');
            if (pos == std::string::npos) {
                continue; // Ligne mal formée, on l'ignore
            }
            
            std::string name = line.substr(0, pos);
            std::string value = line.substr(pos + 1);
            
            // Nettoyer les espaces au début et à la fin
            // Pour simplifier, on ne fait que supprimer les espaces en début de valeur
            if (!value.empty() && value[0] == ' ') {
                value = value.substr(1);
            }
            
            // Charger les paramètres selon leur nom
            if (name == "host") {
                mHost = value;
            }
            else if (name == "api_key") {
                mAPIKey = value;
            }
            else if (name == "api_secret") {
                mAPISecret = value;
            }
        }
        
        file.close();
        return eNoError;
    }
    
    EErrors CConfiguration::Save() {
        std::ofstream file;
        file.open(CONF_FILE, std::ios::out);
        
        if (!file.is_open()) {
            return eCannotOpenFile;
        }
        
        // Écrire un en-tête commenté
        file << "# " << FULLNAME << " Configuration File" << std::endl;
        file << "# This file is automatically generated" << std::endl;
        file << std::endl;
        
        // Sauvegarder les paramètres
        file << "host:" << mHost << std::endl;
        file << "api_key:" << mAPIKey << std::endl;
        file << "api_secret:" << mAPISecret << std::endl;
        
        file.close();
        return eNoError;
    }
}