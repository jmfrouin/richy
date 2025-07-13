//
// Created by Jean-Michel Frouin on 13/07/2025.
//

#include "KrakenApi.h"
#include <iostream>
#include <sstream>
#include <iomanip>
#include <chrono>
#include <ctime>
#include <algorithm>
#include <curl/curl.h>
#include <openssl/hmac.h>
#include <openssl/sha.h>
#include <openssl/bio.h>
#include <openssl/evp.h>
#include <openssl/buffer.h>
#include <json/json.h>

namespace API {

    // Callback pour CURL
    static size_t WriteCallback(void* contents, size_t size, size_t nmemb, void* userp) {
        ((std::string*)userp)->append((char*)contents, size * nmemb);
        return size * nmemb;
    }

    KrakenApi::KrakenApi() : 
        mApiKey(""), 
        mApiSecret(""), 
        mBaseUrl("https://api.kraken.com"),
        mSandboxMode(false),
        mLastError("") {
        
        // Initialisation de CURL
        curl_global_init(CURL_GLOBAL_DEFAULT);
    }

    KrakenApi::~KrakenApi() {
        DisconnectWebSocket();
        curl_global_cleanup();
    }

    void KrakenApi::SetCredentials(const std::string& apiKey, const std::string& apiSecret) {
        mApiKey = apiKey;
        mApiSecret = apiSecret;
    }

    void KrakenApi::SetSandboxMode(bool enabled) {
        mSandboxMode = enabled;
        if (enabled) {
            mBaseUrl = "https://api.demo.kraken.com";
        } else {
            mBaseUrl = "https://api.kraken.com";
        }
    }

    // ===== MÉTHODES PRIVÉES =====

    std::string KrakenApi::MakeRequest(const std::string& endpoint, const std::string& method, 
                                     const std::map<std::string, std::string>& params, 
                                     bool authenticated) {
        CURL* curl;
        CURLcode res;
        std::string readBuffer;
        
        curl = curl_easy_init();
        if (!curl) {
            mLastError = "Failed to initialize CURL";
            return "";
        }
        
        std::string url = mBaseUrl + endpoint;
        std::string postData = "";
        
        // Construction des paramètres POST
        if (!params.empty()) {
            for (auto it = params.begin(); it != params.end(); ++it) {
                if (it != params.begin()) postData += "&";
                postData += it->first + "=" + it->second;
            }
        }
        
        // Headers
        struct curl_slist* headers = NULL;
        headers = curl_slist_append(headers, "Content-Type: application/x-www-form-urlencoded");
        headers = curl_slist_append(headers, "User-Agent: Richy Trading Bot 1.0");
        
        // Authentification si nécessaire
        if (authenticated) {
            std::string nonce = GenerateNonce();
            postData = "nonce=" + nonce + (postData.empty() ? "" : "&" + postData);
            
            std::string signature = GenerateSignature(endpoint, nonce, postData);
            std::string apiKeyHeader = "API-Key: " + mApiKey;
            std::string signHeader = "API-Sign: " + signature;
            
            headers = curl_slist_append(headers, apiKeyHeader.c_str());
            headers = curl_slist_append(headers, signHeader.c_str());
        }
        
        // Configuration CURL
        curl_easy_setopt(curl, CURLOPT_URL, url.c_str());
        curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, WriteCallback);
        curl_easy_setopt(curl, CURLOPT_WRITEDATA, &readBuffer);
        curl_easy_setopt(curl, CURLOPT_HTTPHEADER, headers);
        curl_easy_setopt(curl, CURLOPT_TIMEOUT, 30L);
        
        if (method == "POST" || authenticated) {
            curl_easy_setopt(curl, CURLOPT_POSTFIELDS, postData.c_str());
        }
        
        // Exécution
        res = curl_easy_perform(curl);
        
        // Nettoyage
        curl_slist_free_all(headers);
        curl_easy_cleanup(curl);
        
        if (res != CURLE_OK) {
            mLastError = "CURL error: " + std::string(curl_easy_strerror(res));
            return "";
        }
        
        return readBuffer;
    }

    std::string KrakenApi::GenerateNonce() {
        auto now = std::chrono::duration_cast<std::chrono::microseconds>(
            std::chrono::system_clock::now().time_since_epoch()
        ).count();
        return std::to_string(now);
    }

    std::string KrakenApi::GenerateSignature(const std::string& path, const std::string& nonce, 
                                           const std::string& postData) {
        // Décoder la clé secrète base64
        BIO* bio = BIO_new_mem_buf(mApiSecret.c_str(), -1);
        BIO* b64 = BIO_new(BIO_f_base64());
        bio = BIO_push(b64, bio);
        BIO_set_flags(bio, BIO_FLAGS_BASE64_NO_NL);
        
        char decoded_key[256];
        int decoded_len = BIO_read(bio, decoded_key, sizeof(decoded_key));
        BIO_free_all(bio);
        
        // Créer le message à signer
        std::string message = nonce + postData;
        
        // Hash SHA256 du message
        unsigned char hash[SHA256_DIGEST_LENGTH];
        SHA256_CTX sha256;
        SHA256_Init(&sha256);
        SHA256_Update(&sha256, message.c_str(), message.length());
        SHA256_Final(hash, &sha256);
        
        // Concatener path + hash
        std::string pathHash = path + std::string((char*)hash, SHA256_DIGEST_LENGTH);
        
        // HMAC SHA512
        unsigned char hmac_result[EVP_MAX_MD_SIZE];
        unsigned int hmac_len;
        
        HMAC(EVP_sha512(), decoded_key, decoded_len, 
             (unsigned char*)pathHash.c_str(), pathHash.length(), 
             hmac_result, &hmac_len);
        
        // Encoder en base64
        bio = BIO_new(BIO_s_mem());
        b64 = BIO_new(BIO_f_base64());
        bio = BIO_push(b64, bio);
        BIO_set_flags(bio, BIO_FLAGS_BASE64_NO_NL);
        
        BIO_write(bio, hmac_result, hmac_len);
        BIO_flush(bio);
        
        char* encoded_data;
        long encoded_len = BIO_get_mem_data(bio, &encoded_data);
        std::string signature(encoded_data, encoded_len);
        
        BIO_free_all(bio);
        return signature;
    }

    // ===== MÉTHODES PUBLIQUES =====

    std::vector<std::string> KrakenApi::GetTradingPairs() {
        std::vector<std::string> pairs;
        std::string response = MakeRequest("/0/public/AssetPairs");
        
        if (response.empty()) {
            return pairs;
        }
        
        Json::Value root;
        Json::Reader reader;
        
        if (reader.parse(response, root)) {
            if (root["error"].empty() && root["result"].isObject()) {
                for (const auto& member : root["result"].getMemberNames()) {
                    pairs.push_back(member);
                }
            }
        }
        
        return pairs;
    }

    std::map<std::string, std::string> KrakenApi::GetAssetInfo() {
        std::map<std::string, std::string> assets;
        std::string response = MakeRequest("/0/public/Assets");
        
        if (response.empty()) {
            return assets;
        }
        
        Json::Value root;
        Json::Reader reader;
        
        if (reader.parse(response, root)) {
            if (root["error"].empty() && root["result"].isObject()) {
                for (const auto& member : root["result"].getMemberNames()) {
                    assets[member] = root["result"][member]["altname"].asString();
                }
            }
        }
        
        return assets;
    }

    TickerData KrakenApi::GetTicker(const std::string& pair) {
        TickerData ticker;
        ticker.pair = pair;
        
        std::map<std::string, std::string> params;
        params["pair"] = pair;
        
        std::string response = MakeRequest("/0/public/Ticker", "GET", params);
        
        if (response.empty()) {
            return ticker;
        }
        
        Json::Value root;
        Json::Reader reader;
        
        if (reader.parse(response, root)) {
            if (root["error"].empty() && root["result"].isObject()) {
                auto pairData = root["result"][pair];
                if (!pairData.isNull()) {
                    ticker.ask = std::stod(pairData["a"][0].asString());
                    ticker.bid = std::stod(pairData["b"][0].asString());
                    ticker.last = std::stod(pairData["c"][0].asString());
                    ticker.volume = std::stod(pairData["v"][1].asString());
                    ticker.high = std::stod(pairData["h"][1].asString());
                    ticker.low = std::stod(pairData["l"][1].asString());
                    ticker.open = std::stod(pairData["o"].asString());
                    ticker.timestamp = std::chrono::duration_cast<std::chrono::seconds>(
                        std::chrono::system_clock::now().time_since_epoch()
                    ).count();
                }
            }
        }
        
        return ticker;
    }

    std::vector<TickerData> KrakenApi::GetMultipleTickers(const std::vector<std::string>& pairs) {
        std::vector<TickerData> tickers;
        
        std::string pairList = "";
        for (size_t i = 0; i < pairs.size(); ++i) {
            if (i > 0) pairList += ",";
            pairList += pairs[i];
        }
        
        std::map<std::string, std::string> params;
        params["pair"] = pairList;
        
        std::string response = MakeRequest("/0/public/Ticker", "GET", params);
        
        if (response.empty()) {
            return tickers;
        }
        
        Json::Value root;
        Json::Reader reader;
        
        if (reader.parse(response, root)) {
            if (root["error"].empty() && root["result"].isObject()) {
                for (const auto& pair : pairs) {
                    auto pairData = root["result"][pair];
                    if (!pairData.isNull()) {
                        TickerData ticker;
                        ticker.pair = pair;
                        ticker.ask = std::stod(pairData["a"][0].asString());
                        ticker.bid = std::stod(pairData["b"][0].asString());
                        ticker.last = std::stod(pairData["c"][0].asString());
                        ticker.volume = std::stod(pairData["v"][1].asString());
                        ticker.high = std::stod(pairData["h"][1].asString());
                        ticker.low = std::stod(pairData["l"][1].asString());
                        ticker.open = std::stod(pairData["o"].asString());
                        ticker.timestamp = std::chrono::duration_cast<std::chrono::seconds>(
                            std::chrono::system_clock::now().time_since_epoch()
                        ).count();
                        tickers.push_back(ticker);
                    }
                }
            }
        }
        
        return tickers;
    }

    OrderBook KrakenApi::GetOrderBook(const std::string& pair, int depth) {
        OrderBook orderBook;
        
        std::map<std::string, std::string> params;
        params["pair"] = pair;
        params["count"] = std::to_string(depth);
        
        std::string response = MakeRequest("/0/public/Depth", "GET", params);
        
        if (response.empty()) {
            return orderBook;
        }
        
        Json::Value root;
        Json::Reader reader;
        
        if (reader.parse(response, root)) {
            if (root["error"].empty() && root["result"].isObject()) {
                auto pairData = root["result"][pair];
                if (!pairData.isNull()) {
                    // Asks (vendeurs)
                    for (const auto& ask : pairData["asks"]) {
                        OrderBookEntry entry;
                        entry.price = std::stod(ask[0].asString());
                        entry.volume = std::stod(ask[1].asString());
                        entry.timestamp = ask[2].asInt64();
                        orderBook.asks.push_back(entry);
                    }
                    
                    // Bids (acheteurs)
                    for (const auto& bid : pairData["bids"]) {
                        OrderBookEntry entry;
                        entry.price = std::stod(bid[0].asString());
                        entry.volume = std::stod(bid[1].asString());
                        entry.timestamp = bid[2].asInt64();
                        orderBook.bids.push_back(entry);
                    }
                }
            }
        }
        
        return orderBook;
    }

    std::vector<Balance> KrakenApi::GetAccountBalance() {
        std::vector<Balance> balances;
        
        std::string response = MakeRequest("/0/private/Balance", "POST", {}, true);
        
        if (response.empty()) {
            return balances;
        }
        
        Json::Value root;
        Json::Reader reader;
        
        if (reader.parse(response, root)) {
            if (root["error"].empty() && root["result"].isObject()) {
                for (const auto& member : root["result"].getMemberNames()) {
                    Balance balance;
                    balance.currency = member;
                    balance.total = std::stod(root["result"][member].asString());
                    balance.available = balance.total; // Kraken ne distingue pas available/locked dans Balance
                    balance.locked = 0.0;
                    balances.push_back(balance);
                }
            }
        }
        
        return balances;
    }

    std::string KrakenApi::PlaceMarketOrder(const std::string& pair, const std::string& type, double volume) {
        std::map<std::string, std::string> params;
        params["pair"] = pair;
        params["type"] = type;
        params["ordertype"] = "market";
        params["volume"] = std::to_string(volume);
        
        std::string response = MakeRequest("/0/private/AddOrder", "POST", params, true);
        
        if (response.empty()) {
            return "";
        }
        
        Json::Value root;
        Json::Reader reader;
        
        if (reader.parse(response, root)) {
            if (root["error"].empty() && root["result"].isObject()) {
                if (root["result"]["txid"].isArray() && root["result"]["txid"].size() > 0) {
                    return root["result"]["txid"][0].asString();
                }
            } else {
                mLastError = "Order failed: " + root["error"][0].asString();
            }
        }
        
        return "";
    }

    std::string KrakenApi::PlaceLimitOrder(const std::string& pair, const std::string& type, 
                                         double volume, double price) {
        std::map<std::string, std::string> params;
        params["pair"] = pair;
        params["type"] = type;
        params["ordertype"] = "limit";
        params["volume"] = std::to_string(volume);
        params["price"] = std::to_string(price);
        
        std::string response = MakeRequest("/0/private/AddOrder", "POST", params, true);
        
        if (response.empty()) {
            return "";
        }
        
        Json::Value root;
        Json::Reader reader;
        
        if (reader.parse(response, root)) {
            if (root["error"].empty() && root["result"].isObject()) {
                if (root["result"]["txid"].isArray() && root["result"]["txid"].size() > 0) {
                    return root["result"]["txid"][0].asString();
                }
            } else {
                mLastError = "Order failed: " + root["error"][0].asString();
            }
        }
        
        return "";
    }

    bool KrakenApi::CancelOrder(const std::string& orderId) {
        std::map<std::string, std::string> params;
        params["txid"] = orderId;
        
        std::string response = MakeRequest("/0/private/CancelOrder", "POST", params, true);
        
        if (response.empty()) {
            return false;
        }
        
        Json::Value root;
        Json::Reader reader;
        
        if (reader.parse(response, root)) {
            if (root["error"].empty()) {
                return true;
            } else {
                mLastError = "Cancel failed: " + root["error"][0].asString();
            }
        }
        
        return false;
    }

    std::vector<Order> KrakenApi::GetOpenOrders(const std::string& pair) {
        std::vector<Order> orders;
        
        std::map<std::string, std::string> params;
        if (!pair.empty()) {
            params["pair"] = pair;
        }
        
        std::string response = MakeRequest("/0/private/OpenOrders", "POST", params, true);
        
        if (response.empty()) {
            return orders;
        }
        
        Json::Value root;
        Json::Reader reader;
        
        if (reader.parse(response, root)) {
            if (root["error"].empty() && root["result"].isObject()) {
                auto openOrders = root["result"]["open"];
                for (const auto& member : openOrders.getMemberNames()) {
                    Order order;
                    order.orderId = member;
                    auto orderData = openOrders[member];
                    order.pair = orderData["descr"]["pair"].asString();
                    order.type = orderData["descr"]["type"].asString();
                    order.orderType = orderData["descr"]["ordertype"].asString();
                    order.volume = std::stod(orderData["vol"].asString());
                    order.price = std::stod(orderData["descr"]["price"].asString());
                    order.filled = std::stod(orderData["vol_exec"].asString());
                    order.status = orderData["status"].asString();
                    order.timestamp = orderData["opentm"].asInt64();
                    orders.push_back(order);
                }
            }
        }
        
        return orders;
    }

    // ===== MÉTHODES UTILITAIRES =====

    bool KrakenApi::ValidatePair(const std::string& pair) {
        auto pairs = GetTradingPairs();
        return std::find(pairs.begin(), pairs.end(), pair) != pairs.end();
    }

    std::string KrakenApi::GetServerTime() {
        std::string response = MakeRequest("/0/public/Time");
        
        if (response.empty()) {
            return "";
        }
        
        Json::Value root;
        Json::Reader reader;
        
        if (reader.parse(response, root)) {
            if (root["error"].empty() && root["result"].isObject()) {
                return root["result"]["rfc1123"].asString();
            }
        }
        
        return "";
    }

    bool KrakenApi::TestConnection() {
        std::string serverTime = GetServerTime();
        return !serverTime.empty();
    }

    bool KrakenApi::TestAuthentication() {
        if (mApiKey.empty() || mApiSecret.empty()) {
            mLastError = "API credentials not set";
            return false;
        }
        
        std::string response = MakeRequest("/0/private/Balance", "POST", {}, true);
        
        if (response.empty()) {
            return false;
        }
        
        Json::Value root;
        Json::Reader reader;
        
        if (reader.parse(response, root)) {
            if (root["error"].empty()) {
                return true;
            } else {
                mLastError = "Authentication failed: " + root["error"][0].asString();
            }
        }
        
        return false;
    }

    std::string KrakenApi::GetLastError() const {
        return mLastError;
    }

    bool KrakenApi::HasError() const {
        return !mLastError.empty();
    }

    // ===== MÉTHODES WEBSOCKET (STUBS) =====

    bool KrakenApi::ConnectWebSocket() {
        // TODO: Implémenter WebSocket
        mLastError = "WebSocket not implemented yet";
        return false;
    }

    void KrakenApi::DisconnectWebSocket() {
        // TODO: Implémenter WebSocket
    }

    bool KrakenApi::SubscribeToTicker(const std::string& pair) {
        // TODO: Implémenter WebSocket
        mLastError = "WebSocket not implemented yet";
        return false;
    }

    void KrakenApi::SetTickerCallback(std::function<void(const TickerData&)> callback) {
        mTickerCallback = callback;
    }

    // ===== MÉTHODES NON IMPLÉMENTÉES (STUBS) =====

    std::vector<Trade> KrakenApi::GetRecentTrades(const std::string& pair, int count) {
        // TODO: Implémenter
        return std::vector<Trade>();
    }

    std::vector<std::vector<double>> KrakenApi::GetOHLC(const std::string& pair, int interval) {
        // TODO: Implémenter
        return std::vector<std::vector<double>>();
    }

    std::string KrakenApi::GetSystemStatus() {
        // TODO: Implémenter
        return "";
    }

    std::map<std::string, double> KrakenApi::GetTradingBalance() {
        // TODO: Implémenter
        return std::map<std::string, double>();
    }

    std::string KrakenApi::PlaceOrder(const std::string& pair, const std::string& type, 
                                     const std::string& orderType, double volume, 
                                     double price, const std::map<std::string, std::string>& options) {
        // TODO: Implémenter version générique
        return "";
    }

    std::string KrakenApi::PlaceStopLossOrder(const std::string& pair, const std::string& type, 
                                             double volume, double stopPrice) {
        // TODO: Implémenter
        return "";
    }

    bool KrakenApi::CancelAllOrders(const std::string& pair) {
        // TODO: Implémenter
        return false;
    }

    std::vector<Order> KrakenApi::GetClosedOrders(const std::string& pair, int count) {
        // TODO: Implémenter
        return std::vector<Order>();
    }

    Order KrakenApi::GetOrderInfo(const std::string& orderId) {
        // TODO: Implémenter
        return Order();
    }

    std::vector<Trade> KrakenApi::GetTradeHistory(const std::string& pair, int count) {
        // TODO: Implémenter
        return std::vector<Trade>();
    }

    std::vector<Position> KrakenApi::GetOpenPositions() {
        // TODO: Implémenter
        return std::vector<Position>();
    }

    std::vector<std::string> KrakenApi::GetDepositMethods(const std::string& asset) {
        // TODO: Implémenter
        return std::vector<std::string>();
    }

    std::string KrakenApi::GetDepositAddress(const std::string& asset, const std::string& method) {
        // TODO: Implémenter
        return "";
    }

    std::string KrakenApi::RequestWithdrawal(const std::string& asset, const std::string& key, 
                                            double amount) {
        // TODO: Implémenter
        return "";
    }

    double KrakenApi::GetMinOrderSize(const std::string& pair) {
        // TODO: Implémenter
        return 0.0;
    }

    double KrakenApi::GetTickSize(const std::string& pair) {
        // TODO: Implémenter
        return 0.0;
    }

    double KrakenApi::CalculateOrderValue(const std::string& pair, double volume, double price) {
        // TODO: Implémenter
        return volume * price;
    }

    double KrakenApi::CalculateFees(const std::string& pair, double volume, const std::string& type) {
        // TODO: Implémenter (frais Kraken ~0.26% pour maker, ~0.16% pour taker)
        return volume * 0.0026;
    }

    bool KrakenApi::SubscribeToOrderBook(const std::string& pair) {
        // TODO: Implémenter WebSocket
        return false;
    }

    bool KrakenApi::SubscribeToTrades(const std::string& pair) {
        // TODO: Implémenter WebSocket
        return false;
    }

    bool KrakenApi::SubscribeToOwnTrades() {
        // TODO: Implémenter WebSocket
        return false;
    }

    void KrakenApi::SetOrderBookCallback(std::function<void(const OrderBook&)> callback) {
        mOrderBookCallback = callback;
    }

    void KrakenApi::SetTradeCallback(std::function<void(const Trade&)> callback) {
        mTradeCallback = callback;
    }

} // API