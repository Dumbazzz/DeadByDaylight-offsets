#include <fstream>
#include <iostream>
#include <vector>
#include <windows.h>

class Updater {
private:
	const char* path;
	std::ifstream file;
	size_t fileSize;
	uint32_t codeOffset;

	uint32_t world_offset;
	uint32_t objects_offset;
	uint32_t names_offset;
private:
	unsigned long GetInt(char* hex) {
		const auto uhex = reinterpret_cast<unsigned char*>(hex);
		unsigned long num = uhex[0] << 0 | uhex[1] << 8 | uhex[2] << 16 | uhex[3] << 24;
		return num;
	}
	bool CompareByteArray(unsigned char* data, unsigned char* sig, size_t size) {
		for (size_t i = 0; i < size; i++) {
			if (sig[i] == 0x00) continue;
			if (data[i] != sig[i]) return false;
		}
		return true;
	}
	size_t FindSignature(unsigned char* sig, size_t size, size_t start_at = 0) {
		if (size <= 64) {
			static const unsigned int chunkSize = 16000;
			if (!file) {
				file.close();
				file.open(path, std::ios::in | std::ios::binary);
			}
			for (size_t i = start_at; i < fileSize; i += chunkSize) {
				file.seekg(i);
				unsigned char bytes[chunkSize];
				memset(bytes, 0, chunkSize);
				file.read((char*)&bytes, chunkSize);
				for (size_t k = 0; k < chunkSize; k++) {
					if (bytes[k] == sig[0]) {
						auto offset = i + k;
						file.seekg(offset);
						unsigned char bytesArray[64];
						memset(bytesArray, 0, 64);
						file.read((char*)&bytesArray, size);
						bool equals = CompareByteArray(bytesArray, sig, size);
						if (equals) return offset;
					}
				}
			}
		}
		return 0;
	}
	uint32_t FindOffset(unsigned char* sig, size_t size, size_t start_at = 0) {
		auto instructionAddr = FindSignature(sig, size, start_at);
		if (instructionAddr == 0) return 0;
		file.seekg(instructionAddr + 3);
		char lEndian[4];
		file.read(lEndian, 4);
		auto rOffset = GetInt(lEndian);
		return instructionAddr + 7 + rOffset + codeOffset;
	}
public:
	size_t LoadFile(const char* _path) {
		fileSize = 0;
		file = std::ifstream(_path, std::ios::binary);
		if (file.is_open()) {
			path = _path;

			std::vector<unsigned char> bytes(sizeof(IMAGE_DOS_HEADER));

			file.read((char*)bytes.data(), sizeof(IMAGE_DOS_HEADER));
			auto new_header_ptr = reinterpret_cast<PIMAGE_DOS_HEADER>(bytes.data())->e_lfanew;

			file.seekg(new_header_ptr);
			bytes.reserve(sizeof(IMAGE_NT_HEADERS));
			file.read((char*)bytes.data(), sizeof(IMAGE_NT_HEADERS));
			auto baseOfCode = reinterpret_cast<PIMAGE_NT_HEADERS>(bytes.data())->OptionalHeader.BaseOfCode;

			file.read((char*)bytes.data(), sizeof(IMAGE_SECTION_HEADER));
			auto rawDataPtr = reinterpret_cast<PIMAGE_SECTION_HEADER>(bytes.data())->PointerToRawData;

			codeOffset = baseOfCode - rawDataPtr;

			file.seekg(0, file.end);
			fileSize = file.tellg();
		}
		return fileSize;
	};
	uint32_t GetGWorld() {
		static unsigned char sig[] = { 0x48, 0x8B, 0x1D, 0x00, 0x00, 0x00, 0x00, 0x48, 0x85, 0xDB, 0x74, 0x00, 0x41, 0xB0, 0x01 };
		world_offset = FindOffset(sig, sizeof(sig));
		return world_offset;
	}
	size_t GetGObjects() {
		static unsigned char sig[] = { 0x48, 0x8B, 0x05, 0x00, 0x00, 0x00, 0x00, 0x48, 0x8B, 0x0C, 0xC8, 0x48, 0x8D, 0x04, 0xD1, 0xEB };
		objects_offset = FindOffset(sig, sizeof(sig));
		return objects_offset;
	}
	uint64_t GetGNames() {
		static unsigned char sigv[][12] = { { 0x48, 0x8B, 0x05, 0x00, 0x00, 0x00, 0x00, 0x48, 0x85, 0xC0, 0x75, 0x5F }, { 0x48, 0x8D, 0x15, 0x00, 0x00, 0x00, 0x00, 0xEB, 0x00, 0x48, 0x8D, 0x0D } };
		for (unsigned char i = 0; i < sizeof(sigv)/sizeof(sigv[0]); i++) {
			names_offset = FindOffset(sigv[i], sizeof(sigv[i]));
			if (names_offset) break;
		}
		return names_offset;
	}
};

int main(int argc, const char* argv[]) {
	if (argc != 2) {
		std::cout << "Usage:\t.\\updater.exe \"pathToUE4Game\"\n";
		return 0;
	}
	Updater updater;
	if (!updater.LoadFile(argv[1])) return 0;
	std::cout << "World\t:\t" << std::hex << updater.GetGWorld() << std::endl;
	Beep(370, 100);
	std::cout << "Objects\t:\t" << std::hex << updater.GetGObjects() << std::endl;
	Beep(370, 100);
	std::cout << "Names\t:\t" << std::hex << updater.GetGNames() << std::endl;
	Beep(370, 100);
	return 1;
}