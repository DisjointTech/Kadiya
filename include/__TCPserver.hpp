#include <fstream>

class FileHandler {
	std::ifstream fileStream;
	std::string pathToFile;
	char* buf;
	unsigned int fileSize;
	bool isOpened = false;

public:
	FileHandler(std::string pathToFile) {
		this->pathToFile = pathToFile;
		this->fileStream.open(pathToFile, std::ifstream::binary);
		this->isOpened = true;
		buf = new char[1];
	}
	bool isOpen(void) {
		return this->fileStream.is_open();
	}
	bool eof(void) {
		return this->fileStream.eof();
	}
	std::string getLine() {
		std::string buffer;
		if (this->isOpen())
			std::getline(fileStream, buffer);
		return buffer;
	}

	void seek(unsigned int value) {
		this->fileStream.seekg(value);
	}

	int getSize(void) {
		int currentPos = this->fileStream.tellg();
		this->fileStream.seekg(0, std::ios_base::end);
		int size = this->fileStream.tellg();
		this->fileStream.seekg(currentPos);
		return size;
	}
	void close() {
		this->fileStream.close();
	}

	char* getChunk(unsigned int chunk) {
		std::memset(buf, 0, 1);
		this->fileStream.read(buf, chunk);
		this->seek(0);
		return buf;
	}
	std::string getContent(void) {
		if (!this->isOpen())
			return "";
		std::string buffer((std::istreambuf_iterator<char>(this->fileStream)),
			(std::istreambuf_iterator<char>()));
		return buffer;
	}
	~FileHandler() {
		delete[] buf;
		//this->close();
	}
};