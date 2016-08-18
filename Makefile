OPTS=-march=native -O3 -g -W -Wall -std=c++11 -fPIC -shared -DNDEBUG -DNO_DEBUG 
INCLUDES=-I. -Icharls/src
LIBJPEGLS_PATH=charls/build

MAJOR = 0
MINOR = 2
NAME1 = h5jpegls
VERSION = $(MAJOR).$(MINOR)

%.o: %.cc
	$(CXX) $(OPTS) $(INCLUDES) -c $< -o $@


lib: lib$(NAME1).so.$(VERSION)

lib$(NAME1).so.$(VERSION): $(NAME1).o
	$(CXX) $(OPTS) $^ -L$(LIBJPEGLS_PATH) -Wl,-rpath=$(shell readlink -e $(LIBJPEGLS_PATH)) -lpthread -lhdf5 -lCharLS -o $@
	ln -sf lib$(NAME1).so.$(VERSION) lib$(NAME1).so
	ln -sf lib$(NAME1).so.$(VERSION) lib$(NAME1).so.$(MAJOR)
                                
    
clean:
	rm -f $(CCSDS_OBJS) $(NAME1).o lib$(NAME1).so*
