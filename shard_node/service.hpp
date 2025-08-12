#pragma once

#include "kvstore.grpc.pb.h"
#include "PartitionedKVStore.hpp"
#include <grpcpp/grpcpp.h>

class KVStoreServiceImpl : public kvstore::KVStore::Service {
public:
    explicit KVStoreServiceImpl(PartitionedKVStore* store);
    
    grpc::Status Put(grpc::ServerContext* context, 
                     const kvstore::PutRequest* request, 
                     kvstore::PutResponse* response) override;
    
    grpc::Status Get(grpc::ServerContext* context, 
                     const kvstore::GetRequest* request, 
                     kvstore::GetResponse* response) override;
    
    grpc::Status Remove(grpc::ServerContext* context, 
                        const kvstore::RemoveRequest* request, 
                        kvstore::RemoveResponse* response) override;

private:
    PartitionedKVStore* store_;
};
