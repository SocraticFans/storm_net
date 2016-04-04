#ifndef STORM_SERVER_H_
#define STORM_SERVER_H_

namespace storm {

class Server {
public:
	virtual bool init() = 0;
	virtual void destroy() = 0;

protected:

};

// SocketLoop 加个inLoop标记
// OneThreadServer		单线程Server，用户loop、rpc接口实现和网络在同一个线程 loopOnce loop loopOnce(保证网络包发出去)
// DoubleThreadServer	双线程Server，用户loop、rpc接口一个线程，网络一个线程
// MultiThreadServer	多线程Server，用户loop单独一个线程，网络单独一个线程，rpc接口多个线程

}
#endif
