/*
 *
 * Copyright 2015 gRPC authors.
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 *
 */

#include <iostream>
#include <memory>
#include <string>
#include <cstdlib>
#include <pthread.h>

#include <grpcpp/grpcpp.h>

#include "robot.grpc.pb.h"

using grpc::Channel;
using grpc::ClientContext;
using grpc::Status;
using robot::HealthCheckRequest;
using robot::Health;
using robot::State;
using robot::Empty;
using robot::Step;
using robot::RobotService;
using robot::StepType;


struct thread_data {
  int  thread_id;
  char *message;
};


class GreeterClient {
 public:
  GreeterClient(std::shared_ptr<Channel> channel)
      : stub_(RobotService::NewStub(channel)) {}

  // Assembles the client's payload, sends it and presents the response back
  // from the server.
  std::string SayHello(const std::string& user) {
    // Data we are sending to the server.
    HealthCheckRequest request;
    request.set_request_time(1);

    // Container for the data we expect from the server.
    Health reply;

    // Context for the client. It could be used to convey extra information to
    // the server and/or tweak certain RPC behaviors.
    ClientContext context;

    // The actual RPC.
    Status status = stub_->HealthCheck(&context, request, &reply);

    // Act upon its status.
    if (status.ok()) {
      return reply.healthy()? "true" : "false";
    } else {
      std::cout << status.error_code() << ": " << status.error_message()
                << std::endl;
      return "RPC failed";
    }
  }

  std::string SendCommands(const std::string& user) {

    while(true)
    {
        std::string i;
        std::cout << "Please enter an integer value: ";
        std::cin >> i;

        Step request;

        if( i == "w") {
            request.set_type(StepType::FORWARD);
        } else if( i == "s") {
            request.set_type(StepType::BACK);
        } else if( i == "a") {
            request.set_type(StepType::LEFT);
        } else if( i == "d") {
            request.set_type(StepType::RIGHT);
        } else {
            break;
        }


        State reply;
        ClientContext context;
        Status status = stub_->ExecuteStep(&context, request, &reply);
    }


    return "commands sent";
  }

  void *PrintHello(void *threadarg) {
    struct thread_data *my_data;
    my_data = (struct thread_data *) threadarg;

    std::cout << "Thread ID : " << my_data->thread_id ;
    std::cout << " Message : " << my_data->message << std::endl;

    Empty request;
    State state;
    ClientContext context;

    std::unique_ptr<grpc::ClientReader<State> > reader(
            stub_->ChatStream(&context, request));

    while (reader->Read(&state)) {
      std::cout << "Status change called "
                << state.speed() << " at "
                << state.heading() << ", "
                << state.mode() << std::endl;
    }

    pthread_exit(NULL);
  }

 private:
  std::unique_ptr<RobotService::Stub> stub_;
};

void *callChatFunction(void *object){
  struct thread_data td1;
  td1.thread_id = 1;
  td1.message = "This is message 1";
  ((GreeterClient *)object)->PrintHello((void *)&td1);
  return NULL;
}

int main(int argc, char** argv) {
  // Instantiate the client. It requires a channel, out of which the actual RPCs
  // are created. This channel models a connection to an endpoint (in this case,
  // localhost at port 50051). We indicate that the channel isn't authenticated
  // (use of InsecureChannelCredentials()).



  GreeterClient greeter(grpc::CreateChannel(
      "localhost:50051", grpc::InsecureChannelCredentials()));
  std::string user("world");
  std::string reply = greeter.SayHello(user);

  int rc;
  std::cout <<"main() : creating thread, 1" << std::endl;
  pthread_t thread1;

  rc = pthread_create(&thread1, NULL, callChatFunction, (void *)&greeter);

  if (rc) {
    std::cout << "Error:unable to create thread," << rc << std::endl;
    exit(-1);
  }

  std::cout << "Greeter received: " << reply << std::endl;

  std::string cmds = greeter.SendCommands(user);

  return 0;
}
