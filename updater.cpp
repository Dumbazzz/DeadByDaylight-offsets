#include <fstream>
#include <iostream>
#include <vector>
#include <windows.h>
#include <filesystem>

class Updater {
private:
	std::ifstream file;
	size_t fileSize;
	uint32_t world_offset;
	uint32_t names_offset;
private:
	unsigned long GetInt(char* hex) {
		const auto uhex = reinterpret_cast<unsigned char*>(hex);
		unsigned long num = uhex[0] << 0 | uhex[1] << 8 | uhex[2] << 16 | uhex[3] << 24;
		return num;
	}
	bool CompareByteArray(unsigned char* data, unsigned char* sig, size_t size) {
		for (size_t i = 0; i < size; i++) {
			if (sig[i] == 0x00) {
				continue;
			}
			if (data[i] != sig[i]) {
				return false;
			}
		}
		return true;
	}
	size_t FindSignature(unsigned char* sig, size_t size) {
		for (size_t i = 13000000; i < fileSize; i += 8192) {
			file.seekg(i);
			std::vector<unsigned char> bytes(8192);
			file.read((char*)&bytes[0], 8192);
			for (size_t k = 0; k < 8192; k++) {
				if (bytes[k] == sig[0]) {
					file.seekg(i + k);
					std::vector<unsigned char> bytesArray(size);
					file.read((char*)&bytesArray[0], size);
					bool equals = CompareByteArray(bytesArray.data(), sig, size);
					if (equals) {
						return i + k;
					}
				}
			}
		}
		return 0;
	}
	uint32_t FindOffset(unsigned char* sig, size_t size) {
		auto instructionAddr = FindSignature(sig, size);
		if (instructionAddr == 0) return 0;
		file.seekg(instructionAddr + 3);
		char lEndian[4];
		file.read(lEndian, 4);
		auto rOffset = GetInt(lEndian);
		auto offset = (instructionAddr + 7 + rOffset + 0xC00);
		return offset;
	}
public:
	size_t LoadFile(const char* path) {
		fileSize = 0;
		if (std::filesystem::exists(path)) {
			file = std::ifstream(path, std::ios::binary);
			file.seekg(0, file.end);
			fileSize = file.tellg();
		}
		return fileSize;
	};
	uint32_t GetGWorld() {
		static unsigned char sig[] = { 0x48, 0x8B, 0x1D, 0x0, 0x0, 0x0, 0x00, 0x48, 0x85, 0xDB, 0x74, 0x3B };
		world_offset = FindOffset(sig, 12);
		return world_offset;
	}
	uint64_t GetGNames() {
		static unsigned char sig[] = { 0x48, 0x8B, 0x05, 0x00, 0x00, 0x00, 0x00, 0x48, 0x85, 0xC0, 0x75, 0x5F };
		names_offset = FindOffset(sig, 12);
		return names_offset;
	}
	void SaveAsFile(const char* path) {
		file.close();
		std::ofstream saveFile(path);
		saveFile << "{\n\t\"World\": " << std::dec << world_offset << ",\n\t\"Names\": " << std::dec << names_offset << "\n}";
	}
};

int main(int argc, const char* argv[]) {
	if (argc < 2) {
		std::cout << "Options:\n\t-s\tSaveAsFile\n";
		std::cout << "Usage:\n\t.\\updater.exe [-s] \"pathToExe\"";
		return 0;
	}
	Updater updater;
	const char* path = argv[argc - 1];
	bool save = false;
	for (int i = 1; i < argc - 1; i++) {
		if (!strcmp(argv[i], "-s")) save = true;
	}

	if (!updater.LoadFile(path)) return 0;

	std::cout << "World: " << std::hex << updater.GetGWorld() << std::endl;
	std::cout << "Names: " << std::hex << updater.GetGNames();
	if (save) {
		updater.SaveAsFile("offsets.json");
	}
	return 1;
}