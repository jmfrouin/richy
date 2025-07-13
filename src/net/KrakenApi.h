//
// Created by Jean-Michel Frouin on 13/07/2025.
//

#ifndef KRAKENAPI_H
#define KRAKENAPI_H

#include <string>
#include <vector>
#include <map>
#include <memory>
#include "../core/def.h"

namespace API {

    // Structures pour les données de marché
    struct TickerData {
        std::string pair;
        double ask;
        double bid;
        double last;
        double volume;
        double high;
        double low;
        double open;
        long timestamp;
    };

    struct OrderBookEntry {
        double price;
        double volume;
        long timestamp;
    };

    struct OrderBook {
        std::vector<OrderBookEntry> asks;
        std::vector<OrderBookEntry> bids;
    };

    struct Trade {
        double price;
        double volume;
        long timestamp;
        std::string type; // "buy" or "sell"
    };

    struct Balance {
        std::string currency;
        double available;
        double locked;
        double total;
    };

    struct Order {
        std::string orderId;
        std::string pair;
        std::string type; // "buy" or "sell"
        std::string orderType; // "market", "limit", "stop-loss", etc.
        double volume;
        double price;
        double filled;
        std::string status; // "open", "closed", "canceled"
        long timestamp;
    };

    struct Position {
        std::string pair;
        std::string type; // "long" or "short"
        double volume;
        double avgPrice;
        double unrealizedPnL;
        double realizedPnL;
        long timestamp;
    };

    class KrakenApi {
        public:
            KrakenApi();
            ~KrakenApi();
            
            // Configuration
            void SetCredentials(const std::string& apiKey, const std::string& apiSecret);
            void SetSandboxMode(bool enabled);
            
            // ===== MÉTHODES PUBLIQUES (sans authentification) =====
            
            // Informations sur les paires de trading
            std::vector<std::string> GetTradingPairs();
            std::map<std::string, std::string> GetAssetInfo();
            
            // Données de marché en temps réel
            TickerData GetTicker(const std::string& pair);
            std::vector<TickerData> GetMultipleTickers(const std::vector<std::string>& pairs);
            
            // Carnet d'ordres
            OrderBook GetOrderBook(const std::string& pair, int depth = 100);
            
            // Historique des trades
            std::vector<Trade> GetRecentTrades(const std::string& pair, int count = 100);
            
            // Données OHLC (chandelier)
            std::vector<std::vector<double>> GetOHLC(const std::string& pair, int interval = 1);
            
            // Statut du serveur
            std::string GetServerTime();
            std::string GetSystemStatus();
            
            // ===== MÉTHODES PRIVÉES (avec authentification) =====
            
            // Gestion du compte
            std::vector<Balance> GetAccountBalance();
            std::map<std::string, double> GetTradingBalance();
            
            // Gestion des ordres
            std::string PlaceOrder(const std::string& pair, const std::string& type, 
                                 const std::string& orderType, double volume, 
                                 double price = 0.0, const std::map<std::string, std::string>& options = {});
            
            std::string PlaceMarketOrder(const std::string& pair, const std::string& type, double volume);
            std::string PlaceLimitOrder(const std::string& pair, const std::string& type, 
                                      double volume, double price);
            std::string PlaceStopLossOrder(const std::string& pair, const std::string& type, 
                                         double volume, double stopPrice);
            
            bool CancelOrder(const std::string& orderId);
            bool CancelAllOrders(const std::string& pair = "");
            
            // Consultation des ordres
            std::vector<Order> GetOpenOrders(const std::string& pair = "");
            std::vector<Order> GetClosedOrders(const std::string& pair = "", int count = 50);
            Order GetOrderInfo(const std::string& orderId);
            
            // Historique des trades personnels
            std::vector<Trade> GetTradeHistory(const std::string& pair = "", int count = 50);
            
            // Positions (pour le trading sur marge)
            std::vector<Position> GetOpenPositions();
            
            // Dépôts et retraits
            std::vector<std::string> GetDepositMethods(const std::string& asset);
            std::string GetDepositAddress(const std::string& asset, const std::string& method);
            std::string RequestWithdrawal(const std::string& asset, const std::string& key, 
                                        double amount);
            
            // ===== MÉTHODES UTILITAIRES =====
            
            // Validation
            bool ValidatePair(const std::string& pair);
            double GetMinOrderSize(const std::string& pair);
            double GetTickSize(const std::string& pair);
            
            // Calculs
            double CalculateOrderValue(const std::string& pair, double volume, double price);
            double CalculateFees(const std::string& pair, double volume, const std::string& type);
            
            // Gestion des erreurs
            std::string GetLastError() const;
            bool HasError() const;
            
            // WebSocket (pour les données en temps réel)
            bool ConnectWebSocket();
            void DisconnectWebSocket();
            bool SubscribeToTicker(const std::string& pair);
            bool SubscribeToOrderBook(const std::string& pair);
            bool SubscribeToTrades(const std::string& pair);
            bool SubscribeToOwnTrades();
            
            // Callback pour les données WebSocket
            void SetTickerCallback(std::function<void(const TickerData&)> callback);
            void SetOrderBookCallback(std::function<void(const OrderBook&)> callback);
            void SetTradeCallback(std::function<void(const Trade&)> callback);
            
            // Test de connectivité
            bool TestConnection();
            bool TestAuthentication();
            
        private:
            // Méthodes internes
            std::string MakeRequest(const std::string& endpoint, const std::string& method = "GET", 
                                  const std::map<std::string, std::string>& params = {}, 
                                  bool authenticated = false);
            
            std::string GenerateNonce();
            std::string GenerateSignature(const std::string& path, const std::string& nonce, 
                                        const std::string& postData);
            
            // Membres privés
            std::string mApiKey;
            std::string mApiSecret;
            std::string mBaseUrl;
            bool mSandboxMode;
            std::string mLastError;
            
            // WebSocket
            std::unique_ptr<class WebSocketClient> mWebSocket;
            std::function<void(const TickerData&)> mTickerCallback;
            std::function<void(const OrderBook&)> mOrderBookCallback;
            std::function<void(const Trade&)> mTradeCallback;
    };

} // API

#endif //KRAKENAPI_H