#include "grpcserver.h"
#include "util/system.h"
#include "util/logging.h"

#include <grpcpp/grpcpp.h>
#include <thread>
#include <atomic>
#include <memory>
#include <string>

static const char* DEFAULT_GRPC_BIND_ADDRESS = "127.0.0.1:50051";

class GrpcServerImpl final {
public:
    GrpcServerImpl() = default;
    ~GrpcServerImpl() {
        Shutdown();
    }

    bool Start(const std::string& bind_address) {
        grpc::ServerBuilder builder;
        builder.AddListeningPort(bind_address, grpc::InsecureServerCredentials());

        // Register services here when implemented
        // e.g., builder.RegisterService(&bitcoin_service_);

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

private:
    std::unique_ptr<grpc::Server> server_;
    std::thread run_thread_;
    // Future: BitcoinRPCService bitcoin_service_;
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
