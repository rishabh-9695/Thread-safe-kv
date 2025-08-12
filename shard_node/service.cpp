#include "service.hpp"

KVStoreServiceImpl::KVStoreServiceImpl(PartitionedKVStore* store) : store_(store) {}

grpc::Status KVStoreServiceImpl::Put(grpc::ServerContext*, const kvstore::PutRequest* req, kvstore::PutResponse* resp) {
    try {
        if (req->ttl_ms() > 0)
            store_->put(req->key(), req->value(), static_cast<int>(req->ttl_ms()));
        else
            store_->put(req->key(), req->value());
        resp->set_success(true);
        return grpc::Status::OK;
    } catch (const std::exception& e) {
        resp->set_success(false);
        resp->set_error(e.what());
        return grpc::Status(grpc::StatusCode::INTERNAL, e.what());
    }
}

grpc::Status KVStoreServiceImpl::Get(grpc::ServerContext*, const kvstore::GetRequest* req, kvstore::GetResponse* resp) {
    try {
        auto result = store_->get(req->key());
        if (result) {
            resp->set_found(true);
            resp->set_value(*result);
        } else {
            resp->set_found(false);
        }
        return grpc::Status::OK;
    } catch (const std::exception& e) {
        resp->set_found(false);
        resp->set_error(e.what());
        return grpc::Status(grpc::StatusCode::INTERNAL, e.what());
    }
}

grpc::Status KVStoreServiceImpl::Remove(grpc::ServerContext*, const kvstore::RemoveRequest* req, kvstore::RemoveResponse* resp) {
    try {
        store_->remove(req->key());
        resp->set_success(true);
        return grpc::Status::OK;
    } catch (const std::exception& e) {
        resp->set_success(false);
        resp->set_error(e.what());
        return grpc::Status(grpc::StatusCode::INTERNAL, e.what());
    }
}
