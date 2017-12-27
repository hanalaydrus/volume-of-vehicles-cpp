build:
	protoc -I . --cpp_out=. helloworld.proto
	g++ -std=c++11 `pkg-config --cflags protobuf grpc`  -c -o helloworld.pb.o helloworld.pb.cc
	protoc -I . --grpc_out=. --plugin=protoc-gen-grpc=`which grpc_cpp_plugin` helloworld.proto
	g++ -std=c++11 `pkg-config --cflags protobuf grpc`  -c -o helloworld.grpc.pb.o helloworld.grpc.pb.cc
	g++ -std=c++11 `pkg-config --cflags protobuf grpc opencv`  -c -o main.o main.cc
	g++ -std=c++11 `pkg-config --cflags opencv`  -c -o Blob.o Blob.cc
	g++ helloworld.pb.o helloworld.grpc.pb.o main.o Blob.o -L/usr/local/lib `pkg-config --libs protobuf grpc++ grpc opencv` -Wl,--no-as-needed -lgrpc++_reflection -Wl,--as-needed -ldl -o main

run:
	./main

clean:
	rm -f *.o *.pb.cc *.pb.h main