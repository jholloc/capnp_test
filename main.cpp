#include <iostream>
#include <capnp/message.h>
#include <capnp/serialize-packed.h>
#include <vector>
#include <memory>
#include <sys/mman.h>
#include <sys/stat.h>        /* For mode constants */
#include <fcntl.h>           /* For O_* constants */

#include "schema.capnp.h"

struct Buffer {
    char* data;
    size_t size;
};

class VectorOutputStream : public kj::OutputStream {
public:
    std::vector<char> buffer_;

    void write(const void* buffer, size_t size) override {
        std::cout << "HERE " << size << std::endl;
        buffer_.resize(buffer_.size() + size);
        auto data = reinterpret_cast<const char*>(buffer);
        std::copy(data, data + size, buffer_.data() + size);
    }

    void write(kj::ArrayPtr<const kj::ArrayPtr<const kj::byte>> pieces) override {
        for (auto& piece : pieces) {
            auto size = piece.size();
            std::cout << "HERE2 " << size << std::endl;
            buffer_.resize(buffer_.size() + size);
            auto data = reinterpret_cast<const char*>(piece.begin());
            std::copy(data, data + size, buffer_.data() + size);
        }
    }
};

Buffer uda_capnp_serialise(capnp::MessageBuilder& builder)
{
    kj::VectorOutputStream out;
    capnp::writePackedMessage(out, builder);

    auto arr = out.getArray();
    char* buffer = (char*)malloc(arr.size());
    memcpy(buffer, arr.begin(), arr.size());

    return Buffer{ buffer, arr.size() };
}

std::shared_ptr<capnp::MessageReader> uda_capnp_deserialise(const char* bytes, size_t size)
{
    // ArrayPtr requires non-const ptr, but we are only using this to read from the bytes array
    kj::ArrayPtr<kj::byte> buffer(reinterpret_cast<kj::byte*>(const_cast<char*>(bytes)), size);
    kj::ArrayInputStream in(buffer);

    return std::make_shared<capnp::PackedMessageReader>(in);
}

Buffer uda_capnp_serialise2(capnp::MessageBuilder& builder)
{
    auto array = capnp::messageToFlatArray(builder);
    auto bytes = array.asBytes();

    char* buffer = (char*)malloc(bytes.size() * sizeof(capnp::word));
    std::copy(bytes.begin(), bytes.end(), buffer);

    return Buffer{ buffer, bytes.size() };
}

std::shared_ptr<capnp::MessageReader> uda_capnp_deserialise2(const char* bytes, size_t size)
{
    // ArrayPtr requires non-const ptr, but we are only using this to read from the bytes array
    kj::ArrayPtr<capnp::word> buffer(reinterpret_cast<capnp::word*>(const_cast<char*>(bytes)), size / sizeof(capnp::word));

    return std::make_shared<capnp::FlatArrayMessageReader>(buffer);
}

Buffer uda_capnp_serialise3(capnp::MessageBuilder& builder)
{
    kj::VectorOutputStream out;
    capnp::writePackedMessage(out, builder);

    auto arr = out.getArray();
    char* buffer = (char*)malloc(arr.size());
    memcpy(buffer, arr.begin(), arr.size());

    return Buffer{ buffer, arr.size() };
}

std::shared_ptr<capnp::MessageReader> uda_capnp_deserialise3(const char* bytes, size_t size)
{
    auto file = fopen("buffer.bin", "wb");
    fwrite(bytes, 1, size, file);
    fclose(file);

    file = fopen("buffer.bin", "rb");
    auto fd = fileno(file);

    return std::make_shared<capnp::PackedFdMessageReader>(fd);
}

void print(capnp::MessageReader& reader)
{
    auto root = reader.getRoot<TreeNode>();
    std::cout << "name = " << root.getName().cStr() << ", num children = " << root.getChildren().size() << std::endl;
}

int main(int argc, const char** argv) {
    if (argc < 3) {
        return 1;
    }
    std::string arg1 = argv[1];
    std::string arg2 = argv[2];

    int type = std::stoi(arg1);
    int n = std::stoi(arg2);

    capnp::MallocMessageBuilder builder;
    auto root = builder.initRoot<TreeNode>();
    root.setName("root");
    root.initChildren(n);

    int i = 0;
    for (auto child : root.getChildren()) {
        std::string name = "child-" + std::to_string(i);
        child.setName(name);
        auto array = child.initArray();
        array.setType(TreeNode::Type::FLT64);
        int size = 100;
        array.setLen(size);
        auto shape = array.initShape(1);
        shape.set(0, size);
        std::vector<double> data(size);
        for (int j = 0; j < size; ++j) {
            data[j] = i;
        }
        kj::ArrayPtr<kj::byte> ptr(reinterpret_cast<kj::byte*>(data.data()), size * sizeof(double));
        array.setData(ptr);
        ++i;
    }

    switch (type) {
        case 1: {
            std::cout << "PackedMessage serialisation" << std::endl;

            Buffer buffer = uda_capnp_serialise(builder);

            std::cout << "size = " << buffer.size << std::endl;

            auto reader = uda_capnp_deserialise(buffer.data, buffer.size);
            print(*reader);

            break;
        }
        case 2: {
            std::cout << "FlatArrayMessage serialisation" << std::endl;

            Buffer buffer = uda_capnp_serialise2(builder);

            std::cout << "size = " << buffer.size << std::endl;

            auto reader = uda_capnp_deserialise2(buffer.data, buffer.size);
            print(*reader);

            break;
        }
        case 3: {
            std::cout << "PackedMessage with file serialisation" << std::endl;

            Buffer buffer = uda_capnp_serialise3(builder);

            std::cout << "size = " << buffer.size << std::endl;

            auto reader = uda_capnp_deserialise3(buffer.data, buffer.size);
            print(*reader);

            break;
        }
    }

    return 0;
}
