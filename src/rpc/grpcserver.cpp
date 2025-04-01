// Copyright (c) 2010 Satoshi Nakamoto
// Copyright (c) 2009-2025 The Bitcoin Core developers
// Distributed under the MIT software license; see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include "grpcserver.h"
#include "util/system.h"
#include "util/logging.h"
#include "bridge_endpoints.h"

#include <grpcpp/grpcpp.h>
#include <thread>
#include <memory>
#include <curl/curl.h>
#include <nlohmann/json.hpp>

#include "bitcoin.grpc.pb.h"
#include "validation.h"
#include "node/context.h"
#include "rpc/blockchain.h"

using json = nlohmann::json;

static const char* DEFAULT_GRPC_BIND_ADDRESS = "0.0.0.0:50051";  // open for external access

class GrpcServerImpl final : public bitcoin::BitcoinRPC::Service {
public:
    GrpcServerImpl() = default;
    ~GrpcServerImpl() {
        Shutdown();
    }

    bool Start(const std::string& bind_address) {
        grpc::ServerBuilder builder;
        builder.AddListeningPort(bind_address, grpc::SslServerCredentials(grpc::SslServerCredentialsOptions()));
        builder.RegisterService(this);

        server_ = builder.BuildAndStart();
        if (!server_) {
            LogPrintf("Failed to start gRPC server on %s\n", bind_address);
            return false;
        }

        LogPrintf("gRPC server started on %s\n", bind_address);
        run_thread_ = std::thread([this]() {
            server_->Wait();
        });

        return true;
    }

    void Interrupt() {
        if (server_) {
            LogPrintf("Interrupting gRPC server...\n");
            server_->Shutdown();
        }
    }

    void Shutdown() {
        if (server_) {
            LogPrintf("Shutting down gRPC server...\n");
            server_->Shutdown();
            if (run_thread_.joinable()) {
                run_thread_.join();
            }
            server_.reset();
        }
    }

    grpc::Status GetBlock(grpc::ServerContext*,
                          const bitcoin::GetBlockRequest* request,
                          bitcoin::GetBlockResponse* response) override
    {
        LOCK(cs_main);

        uint256 hash;
        if (!ParseHashStr(request->block_hash(), hash)) {
            return grpc::Status(grpc::StatusCode::INVALID_ARGUMENT, "Invalid block hash");
        }

        const CBlockIndex* pblockindex = LookupBlockIndex(hash);
        if (!pblockindex) {
            return grpc::Status(grpc::StatusCode::NOT_FOUND, "Block not found");
        }

        response->set_hash(pblockindex->GetBlockHash().ToString());
        response->set_height(pblockindex->nHeight);
        response->set_confirmations(::ChainActive().Height() - pblockindex->nHeight + 1);
        response->set_time(pblockindex->GetBlockTime());

        if (pblockindex->pprev) response->set_previousblockhash(pblockindex->pprev->GetBlockHash().ToString());
        if (ChainActive().Next(pblockindex)) response->set_nextblockhash(ChainActive().Next(pblockindex)->GetBlockHash().ToString());

        if (request->verbose()) {
            CBlock block;
            if (ReadBlockFromDisk(block, pblockindex, Params().GetConsensus())) {
                for (const auto& tx : block.vtx) {
                    response->add_tx(tx->GetHash().ToString());
                }
            }
        }

        return grpc::Status::OK;
    }

    grpc::Status ForwardToBSC(grpc::ServerContext*,
                              const bitcoin::BSCRequest* request,
                              bitcoin::BSCResponse* response) override
    {
        json payload = {
            {"jsonrpc", "2.0"},
            {"id", 1},
            {"method", request->method()},
            {"params", request->params()}
        };

        std::string result;
        CURL* curl = curl_easy_init();
        if (!curl) return grpc::Status(grpc::StatusCode::INTERNAL, "Failed to init curl");

        struct curl_slist* headers = nullptr;
        headers = curl_slist_append(headers, "Content-Type: application/json");
        curl_easy_setopt(curl, CURLOPT_URL, bridge::BSC_RPC_ENDPOINT.c_str());
        curl_easy_setopt(curl, CURLOPT_POSTFIELDS, payload.dump().c_str());
        curl_easy_setopt(curl, CURLOPT_HTTPHEADER, headers);
        curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, [](void* contents, size_t size, size_t nmemb, std::string* out) {
            size_t total = size * nmemb;
            out->append((char*)contents, total);
            return total;
        });
        curl_easy_setopt(curl, CURLOPT_WRITEDATA, &result);

        CURLcode res = curl_easy_perform(curl);
        curl_slist_free_all(headers);
        curl_easy_cleanup(curl);

        if (res != CURLE_OK) {
            return grpc::Status(grpc::StatusCode::INTERNAL, "curl error: " + std::string(curl_easy_strerror(res)));
        }

        response->set_result(result);
        return grpc::Status::OK;
    }

    grpc::Status ForwardToBitcoinUpstream(grpc::ServerContext*,
        const bitcoin::BitcoinPassthroughRequest* request,
        bitcoin::BitcoinPassthroughResponse* response) override
    {
        auto channel = grpc::CreateChannel("ssl://mempool.space:50002", grpc::SslCredentials(grpc::SslCredentialsOptions()));
        auto stub = bitcoin::BitcoinRPC::NewStub(channel);

        bitcoin::GetBlockRequest inner_request;
        inner_request.set_block_hash(request->params(0));
        inner_request.set_verbose(true);

        bitcoin::GetBlockResponse inner_response;
        grpc::ClientContext context;

        grpc::Status status = stub->GetBlock(&context, inner_request, &inner_response);

        if (!status.ok()) {
            return grpc::Status(grpc::StatusCode::INTERNAL, "dRPC call failed: " + status.error_message());
        }

        json j = {
            {"hash", inner_response.hash()},
            {"height", inner_response.height()},
            {"confirmations", inner_response.confirmations()},
            {"tx", inner_response.tx()}
        };

        response->set_result(j.dump());
        return grpc::Status::OK;
    }

private:
    std::unique_ptr<grpc::Server> server_;
    std::thread run_thread_;
};

static std::unique_ptr<GrpcServerImpl> g_rpc_server;

void StartGRPCServer(const std::string& bind_address) {
    if (g_rpc_server) return;
    g_rpc_server = std::make_unique<GrpcServerImpl>();
    if (!g_rpc_server->Start(bind_address)) {
        throw std::runtime_error("Failed to start gRPC server");
    }
}

void InterruptGRPCServer() {
    if (g_rpc_server) {
        g_rpc_server->Interrupt();
    }
}

void StopGRPCServer() {
    if (g_rpc_server) {
        g_rpc_server->Shutdown();
        g_rpc_server.reset();
    }
}
