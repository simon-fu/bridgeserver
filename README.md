# bridgeserver
bridge for rtp session

### 说明
	lib里有编译好的eice；
	编译eice依赖voice/pj;
	
	
### depends
	sudo apt-get install libcppunit-dev liblog4cplus-dev libevent-dev
	
### build main
	./build.sh
	
### build eice library
	export PJ_PATH="/mnt/hgfs/simon/projects/easemob/src/voice/pj"
	./build.sh eice
	
### clean build
	./build.sh clean
	./build.sh eice clean

