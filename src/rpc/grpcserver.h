/// File: src/rpc/grpcserver.h
// Copyright (c) 2025
// SPDX-License-Identifier: MIT

#ifndef BITCOIN_GRPCSERVER_H
#define BITCOIN_GRPCSERVER_H

#include <memory>
#include <string>

namespace grpcserver {

// Forward declaration
class GrpcServerImpl;

/**
 * @brief Encapsulates the gRPC server lifecycle.
 */
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

/**
 * @brief Global lifecycle helpers (for use in main.cpp or node glue code)
 */
void StartGRPCServer(const std::string& bind_address);
void InterruptGRPCServer();
void StopGRPCServer();

} // namespace grpcserver

#endif // BITCOIN_GRPCSERVER_H
