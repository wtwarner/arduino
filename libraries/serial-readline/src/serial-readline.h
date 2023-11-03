/*
 * serial-readline.h - Library for buffered serial line reading
 * Created by MSZ, March 3, 2022.
 * Released into the public domain under MIT Licence.
 * Copyright (c) 2022 Zawisza (MSZ98)
*/

#ifndef SERIAL_READLINE_H
#define SERIAL_READLINE_H

#include <Arduino.h>
#include <string.h>

class LineQueue {

private:
	typedef struct {
		char *line;
		void *next;
	} Line;

	Line *first = NULL, *last = NULL;
	int _size = 0;
	
public:
	void add(char* line) {
		Line *l = new Line;
		l->line = line;
		l->next = NULL;
		if(last != NULL) last->next = l;
		last = l;
		if(first == NULL) first = last;
		_size++;
	}
	
	char* get() {
		Line *l = first;
		first = (Line*) l->next;
		if(first == NULL) last = NULL;
		char *line = l->line;
		delete l;
		_size--;
		return line;
	}

	boolean isEmpty() {
		return first == NULL;
	}

	int size() {return _size;}
	int firstLineLength() {return strlen(first->line);}
	
};

template <class Serial_>
class SerialLineReader {

private:
	LineQueue queue;
	Serial_ *hs;
	void (*isr)(char*) = NULL;

	int buffer_len = 0;
	int buffer_limit;
	char *buffer;

public:
	SerialLineReader(Serial_ &hs, int bufsize, void (*isr)(char*)) {
		initialize(hs, bufsize, isr);
	}
	SerialLineReader(Serial_ &hs, void (*isr)(char*)) {
		initialize(hs, 100, isr);
	}
	SerialLineReader(Serial_ &hs, int bufsize) {
		initialize(hs, bufsize, NULL);
	}
	SerialLineReader(Serial_ &hs) {
		initialize(hs, 100, NULL);
	}
	~SerialLineReader() {
		delete buffer;
	}

	int available() {return queue.size();}
	int len() {return queue.firstLineLength();}
	
  void poll() {
	while(hs->available()) {
//		Read single character and save it in the buffer
		char c = hs->read();
		if(buffer_len >= buffer_limit) return;
		buffer[buffer_len++] = c;
//		If character saved in the buffer is \n, save this line as independent string, add to the Line Queue and clear the buffer
		if(c == '\n') {
			buffer[buffer_len - 1] = 0;
			char *line = new char[buffer_len];
			strcpy(line, buffer);
			queue.add(line);
			buffer_len = 0;
		}
//		If isr is set and line is ready, execute isr (this automatically disposes one line)
		if(!queue.isEmpty() && isr != NULL) {
			char *line = queue.get();
			isr(line);
			delete line;
		}
	}
  }
  
  void read(char* line) {
    	char *l = queue.get();
	strcpy(line, l);
	delete l;
  }

private:
	void initialize(Serial_ &hs, int bufsize, void (*isr)(char*)) {
		this->hs = &hs;
		this->isr = isr;
		buffer = new char[bufsize];
		buffer_limit = bufsize;
	}
};

#endif
