package main

import (
	"context"
	"fmt"
	"time"

	"google.golang.org/grpc"
	test "sylar.top/grpc_test/proto"
)

func testA() {
	cli, err := grpc.Dial("10.104.16.110:8099", grpc.WithInsecure())
	if err != nil {
		fmt.Println(err.Error())
	}

	hc := test.NewHelloServiceClient(cli)

	st, e := hc.HelloStreamA(context.Background())
	fmt.Printf("=== %+v === %+v", st, e)

    for i:= 0; i < 10; i++ {
		st.Send(&test.HelloRequest{
			Id:  "hello",
			Msg: "world",
		})
		//time.Sleep(time.Second)
	}

    rsp, err := st.CloseAndRecv()
    fmt.Printf("%+v - %+v\n", rsp, err);

}

func testB() {
	cli, err := grpc.Dial("10.104.16.110:8099", grpc.WithInsecure())
	if err != nil {
		fmt.Println(err.Error())
	}

	hc := test.NewHelloServiceClient(cli)

	st, e := hc.HelloStreamB(context.Background(), &test.HelloRequest{Id: "hello", Msg: "StreamB"})
	fmt.Printf("=== %+v === %+v", st, e)

    for {
		r, e := st.Recv()
		if e != nil {
			fmt.Println(e.Error())
			break
		}
		fmt.Printf("HelloStreamB %+v\n", r)
	}
}

func testC() {
	cli, err := grpc.Dial("10.104.16.110:8099", grpc.WithInsecure())
	if err != nil {
		fmt.Println(err.Error())
	}

	hc := test.NewHelloServiceClient(cli)

	st, e := hc.HelloStreamC(context.Background())
	fmt.Printf("=== %+v === %+v", st, e)

    time.Sleep(time.Second * 10);

	for {
		st.Send(&test.HelloRequest{
			Id:  "hello",
			Msg: "StreamC",
		})
		time.Sleep(time.Second)
        break
	}
}

func main() {
	testA()
}

