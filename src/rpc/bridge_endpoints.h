#pragma once

#include <string>

namespace bridge {

    // 🌐 Bitcoin over gRPC via dRPC (used for external peer relays or read-only cross-validation)
    // Access requires a valid dkey (like an API key)
    static const std::string BITCOIN_DRPC_ENDPOINT =
        "https://lb.drpc.org/ogrpc?network=bitcoin&dkey=Ah6qDmllBELvtUQj4oOxopX07-O5Dp8R8LPVik6p2x9s";

    // 🌉 Binance Smart Chain via JSON-RPC HTTP (public access)
    // This can be any dRPC BSC endpoint or a preferred high-performance node
    static const std::string BSC_RPC_ENDPOINT =
        "https://rpc.ankr.com/bsc";

    // 🔧 Optional fallback or additional multi-chain endpoints can be added here
    // static const std::string POLYGON_RPC_ENDPOINT = "https://rpc.ankr.com/polygon";
    // static const std::string ETHEREUM_RPC_ENDPOINT = "https://mainnet.infura.io/v3/YOUR_PROJECT_ID";

}
