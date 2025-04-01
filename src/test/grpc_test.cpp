#include <test/util/setup_common.h>
#include <boost/test/unit_test.hpp>

#include <grpcpp/grpcpp.h>
#include <memory>
#include <thread>

#include "rpc/grpcserver.h"
#include "rpc/bitcoin.grpc.pb.h"

BOOST_FIXTURE_TEST_SUITE(grpc_tests, BasicTestingSetup)

// Simple test to validate the gRPC server starts and binds
BOOST_AUTO_TEST_CASE(startup_and_shutdown)
{
    BOOST_CHECK_NO_THROW(StartGRPCServer());
    BOOST_CHECK_NO_THROW(InterruptGRPCServer());
    BOOST_CHECK_NO_THROW(StopGRPCServer());
}

// Optional: Extend with a real client request to BitcoinRPC::GetBlock
// NOTE: Would require a running node with blockchain data or mocks

BOOST_AUTO_TEST_SUITE_END()
