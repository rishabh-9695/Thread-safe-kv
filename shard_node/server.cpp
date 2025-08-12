#include <grpcpp/server.h>
#include <grpcpp/server_builder.h>
#include <grpcpp/security/server_credentials.h>
#include <iostream>
#include <memory>
#include "PartitionedKVStore.hpp"
#include "service.hpp"

void Serve(PartitionedKVStore* store) {
    KVStoreServiceImpl service(store);
    grpc::ServerBuilder builder;
    builder.AddListeningPort("0.0.0.0:50051", grpc::InsecureServerCredentials());
    builder.RegisterService(&service);
    std::unique_ptr<grpc::Server> server(builder.BuildAndStart());
    std::cout << "gRPC KVStore server listening on 0.0.0.0:50051\n";
    server->Wait();
}

int main() {
    // Create the partitioned KV store with 16 partitions for testing
    auto store = std::make_unique<PartitionedKVStore>(8);
    
    std::cout << "Starting gRPC KVStore server with " << store->getPartitionCount() << " partitions...\n";
    
    // Start the gRPC server
    Serve(store.get());
    
    return 0;
}
