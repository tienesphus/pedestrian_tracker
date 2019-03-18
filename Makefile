all: graph demo
convert:
	# face model is from: https://github.com/dannyblueliu/YOLO-version-2-Face-detection
	#1) enter folder with models
	#2) launch docker image with dl-converter, connect data directories in docker and host,
	#   change user to current user to prevent creating files with root permissions,
	#   run bash->python->converter on specified models 
	#   (.cfg .weights) -> (.prototxt .caffemodel)
	#3) fix input format in .prototxt with utility script
	#4) go back
	cd models/face; \
	sudo docker run -v `pwd`:/workspace/data \
	-u `id -u` -ti dlconverter:latest bash -c \
	"python ./pytorch-caffe-darknet-convert/darknet2caffe.py \
	./data/yolo-face.cfg ./data/yolo-face_final.weights \
	./data/yolo-face.prototxt ./data/yolo-face.caffemodel"; \
	python ../../utils/fix_proto_input_format.py yolo-face.prototxt yolo-face-fix.prototxt; \
	cd ../..
graph: convert
	cd models/face; \
	mvNCCompile -s 12 -o graph -w yolo-face.caffemodel yolo-face-fix.prototxt; \
	cd ../..
graph_yolo:
	cd models/face; \
	mvNCCompile -s 12 -o graph -w yolo-face.caffemodel yolo-face-fix.prototxt; \
	cd ../..
graph_ssd:
	cd models/face; \
	mvNCCompile -s 12 -o graph_ssd -w ssd-face.caffemodel ssd-face.prototxt; \
	cd ../..
graph_ssd_longrange:
	cd models/face; \
	mvNCCompile -s 12 -o graph_ssd -w ssd-face-longrange.caffemodel ssd-face-longrange.prototxt; \
	cd ../..
demo:
	g++ \
	test.cpp detection_layer.c ./wrapper/fp16.c ./wrapper/ncs_wrapper.cpp \
	-o test \
	-std=c++11 \
	-I. \
	-lmvnc \
	`pkg-config --cflags --libs opencv`
demo_ssd:
	g++ \
	-I. \
	ssd.cpp ./wrapper/fp16.c ./wrapper/ncs_wrapper.cpp \
	-pg \
	-o ssd_test \
	-lmvnc \
	`pkg-config opencv --cflags --libs`
ssd_track:
	g++ \
	-std=c++11 \
	-fopenmp \
	-I. \
	ssd_track.cpp ./wrapper/fp16.c ./wrapper/ncs_wrapper.cpp \
	-o track \
	-lmvnc \
	`pkg-config opencv --cflags --libs`
bus_track_cv:
	g++ \
	-std=c++11 \
	-fopenmp \
	-I. \
	bus_track.cpp \
	-o bus_cv \
	`pkg-config opencv --cflags --libs`
bus_track_ncs:
	g++ \
	-std=c++11 \
	-fopenmp \
	-I. \
	bus_track_ncs.cpp ./detection.cpp ./wrapper/fp16.c ./wrapper/ncs_wrapper.cpp \
	-pg -O3 \
	-o bus_ncs \
	-lmvnc \
	`pkg-config opencv --cflags --libs`
bus_track_p2:
	g++ \
	-std=c++11 \
	-fopenmp \
	-I. \
	bus_track_p2.cpp ./matt_utils.cpp ./detection.cpp ./wrapper/fp16.c ./wrapper/ncs_wrapper.cpp \
	-pg -O3 \
	-o bus_p2 \
	-lmvnc \
	`pkg-config opencv --cflags --libs`
bus_track_manual:
	g++ \
	-std=c++11 \
	-fopenmp \
	-I. \
	bus_track_ncs_manual.cpp ./detection.cpp ./wrapper/fp16.c ./wrapper/ncs_wrapper.cpp \
	-pg -O3 \
	-o bus_manual \
	-lmvnc \
	`pkg-config opencv --cflags --libs`
profile: convert
	cd models/face; \
	mvNCProfile yolo-face-fix.prototxt -w yolo-face.caffemodel -s 12; \
	cd ../..

