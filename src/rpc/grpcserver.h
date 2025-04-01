// File: src/rpc/grpcserver.h

#ifndef BITCOIN_GRPCSERVER_H
#define BITCOIN_GRPCSERVER_H

#include <memory>
#include <thread>
#include <grpcpp/grpcpp.h>

namespace grpcserver {

class GrpcServerImpl;

class GrpcServer {
public:
    GrpcServer();
    ~GrpcServer();

    // Start the gRPC server
    bool Start(const std::string& server_address);

    // Shut down the server gracefully
    void Shutdown();

    // Interrupt the server (e.g., on SIGINT/SIGTERM)
    void Interrupt();

private:
    std::unique_ptr<GrpcServerImpl> impl;
};

} // namespace grpcserver

#endif // BITCOIN_GRPCSERVER_H
