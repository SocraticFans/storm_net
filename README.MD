#项目说明
		一个c++的rpc架子，方便server间交互。这个东西的主要参考ice和腾讯taf系统，自己造一个简单易用的轮子，没有自己
	去写序列化工具，采用protobuffer作为rpc的序列化工具，自定义接口文件，生成相应服务端和客户端代码。
		rpc的好处就是隐藏了网络层细节，自定义协议等一些繁琐而机械化的工作。在游戏服务器领域，用的竟然不多。所以我希望
	把他搞成一个通用的游戏服务器框架。
		支持同步调用，异步调用和单向调用。

#example
	以echo服务为例
	1.定义Echo.proto

		syntax = "proto2";

		package Common;

		message EchoRequest {
			optional string msg = 1;
		};

		message EchoResponse {
			optional string msg = 1;
		};

	2.EchoService.rpc
		#include "Echo.proto"

		using namespace Common;

		service EchoService {
			Echo(EchoRequest req, EchoResponse resp) = 1;
		};

		其中service表示一个服务，Echo是服务中的一个接口，EchoRequest和EchoResponse就是Echo.proto中定义的请求和
		相应消息结构， 后面的 = 1跟Echo这个方法对应，就是这个方法的协议id，在传输过程中实际传送的不是函数名字，
		而是这个id。
	3. 运行rpc2cpp EchoService.rpc 将会生成EchoService.cpp和EchoService.h
	4. 实现接口
		Server端继承EchoService这个类，实现Echo接口

		class EchoServiceImp : public EchoService {
		public:

			virtual void Echo(Connection::ptr conn, const EchoRequest& req, EchoResponse& resp);
		};

		void EchoServiceImp::Echo(Connection::ptr conn, const EchoRequest& request, EchoResponse& resp) {
			resp.set_msg(request.msg());
		}

		Client(调用方)获得一个ServiceProxy代理类，之后就可以请求服务了

		EchoServiceProxy* g_prx = m_connector->stringToPrx<EchoServiceProxy>("EchoService@tcp -h 127.0.0.1 -p 1234");

		同步调用
		EchoResponse response;
		int ret = g_prx->Echo(request, response);
		cout << response.msg() << endl;

		//异步调用(实现cb类)
		class EchoServiceProxyCB : public EchoServiceProxyCallBack {
		public:
			void callback_Echo(int ret, const EchoResponse& response) {
				cout << response.msg() << endl;
			}
		};

		ServiceProxyCallBackPtr cb(new EchoServiceProxyCB);
		g_prx->async_Echo(cb, request);

		//单向调用
		g_prx->async_Echo(NULL, request);

		

#安装、测试
	依赖： gcc4.8 cmake2.8 protobuf flex bison

	*1. 编译安装
			mkdir build
			cd build 
			cmake ..
			make -j4
			[可选]sudo make install
			make Client
			make Server

	*2. 设置环境变量(可选)
			.bashrc 里添加
			export PATH=$PATH:/usr/local/storm/bin/
			export LD_LIBRARY_PATH=$LD_LIBRARY_PATH:/usr/local/storm/lib/

	*3. 测试：
			cd build
			cp ../config/*.conf ./
			1.启动Server
			  ./Server -f server.conf
			2.启动Client
			  ./Client  -f client.conf

#计划
	log 类
	http parser
	asyncResponce

