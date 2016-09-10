# bridgeserver
bridge for rtp session

### 说明
	lib里有编译好的eice；
	编译eice依赖voice/pj;
	
	
### depends
ubuntu  
  
	sudo apt-get install libcppunit-dev liblog4cplus-dev libevent-dev uuid-dev

osx  

	brew install cppunit log4cplus libevent
	
### build main
	cd lib && ln -s ubuntu/libeice-full.a && cd ..
	./build.sh
	
### build pj library
	./configure --disable-video
	make dep && make clean && make

### build eice library
	export PJ_PATH="/mnt/hgfs/simon/projects/easemob/src/voice/pj"
	./build.sh eice
	
### clean build
	./build.sh clean
	./build.sh eice clean

