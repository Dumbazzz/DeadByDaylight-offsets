#include <vector>
#include <windows.h>
#include <filesystem>

class Updater {
private:
	BYTE* fileBytes = nullptr;

	size_t fileSize = 0;
	uint32_t codeOffset = 0;

	uint32_t world_offset = 0;
	uint32_t objects_offset = 0;
	uint32_t names_offset = 0;
	uint32_t pool_offset = 0;
private:
	bool CompareByteArray(BYTE* data, BYTE* sig, size_t size) {
		for (size_t i = 0; i < size; i++) {
			if (sig[i] == 0x00) continue;
			if (data[i] != sig[i]) return false;
		}
		return true;
	}
	size_t FindSignature(std::vector<BYTE> sig, size_t start_at = 0) {
		for (size_t offset = start_at; offset < fileSize - sig.size(); offset++) {
			if (fileBytes[offset] == sig[0]) {
				bool equals = CompareByteArray(reinterpret_cast<BYTE*>(fileBytes + offset + 1), sig.data() + 1, sig.size() - 1);
				if (equals) {
					return offset;
				}
			}
		}
		return 0;
	}
	uint32_t FindOffset(std::vector<BYTE>* sigv, uint32_t size, size_t start_at = 0) {
		for (auto i = 0; i < size; i++) {
			auto sig = sigv[i];
			auto offset = FindSignature(sig, start_at);
			if (!offset) continue;
			auto k = 0;
			for (; sig[k]; k++);
			uint32_t nextOffset = *reinterpret_cast<uint32_t*>(fileBytes + offset + k);
			return offset + k + 4 + nextOffset + codeOffset;
		}
		return 0;
	}
public:
	~Updater(){
		if (fileBytes){
			delete[] fileBytes;
		}
	}
	size_t LoadFile(const char* path) {
		auto file = CreateFileA(path, GENERIC_READ, FILE_SHARE_READ, NULL, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, nullptr);
		if (file != INVALID_HANDLE_VALUE) {
			LARGE_INTEGER size;
			GetFileSizeEx(file, &size);
			fileSize = size.QuadPart;
			fileBytes = new BYTE[fileSize];
			DWORD readed;
			if (!ReadFile(file, fileBytes, fileSize, &readed, 0) || readed != fileSize) {
				return 0;
			};
			auto new_header_ptr = reinterpret_cast<PIMAGE_DOS_HEADER>(fileBytes)->e_lfanew;
			auto baseOfCode = reinterpret_cast<PIMAGE_NT_HEADERS>(fileBytes + new_header_ptr)->OptionalHeader.BaseOfCode;
			auto rawDataPtr = reinterpret_cast<PIMAGE_SECTION_HEADER>(fileBytes + new_header_ptr + sizeof(IMAGE_NT_HEADERS))->PointerToRawData;
			codeOffset = baseOfCode - rawDataPtr;
			CloseHandle(file);
		}
		return fileSize;
	};
	uint32_t GetGWorld() {
		if (!world_offset) {
			static std::vector<BYTE> sigv[] = { { 0x48, 0x8B, 0x1D, 0x00, 0x00, 0x00, 0x00, 0x48, 0x85, 0xDB, 0x74, 0x00, 0x41, 0xB0, 0x01 }, { 0x48, 0x8b, 0x05, 0x00, 0x00, 0x00, 0x00, 0x75, 0x1b }, {0x48, 0x8B, 0x05, 0x00, 0x00, 0x00, 0x00, 0x48, 0x8B, 0x88, 0x00, 0x00, 0x00, 0x00, 0x48, 0x85, 0xC9, 0x74, 0x06, 0x48, 0x8B, 0x49, 0x70} };
			world_offset = FindOffset(sigv, 3);
		}
		return world_offset;
	}
	uint32_t GetGObjects() {
		if (!objects_offset) {
			static std::vector<BYTE> sigv[] = { {0x48, 0x8B, 0x05, 0x00, 0x00, 0x00, 0x00, 0x48, 0x8B, 0x0C, 0xC8, 0x48, 0x8D, 0x04, 0xD1, 0xEB}, {0x48 , 0x8b , 0x0d , 0x00 , 0x00 , 0x00 , 0x00 , 0x81 , 0x4c , 0xd1 , 0x08 , 0x00 , 0x00 , 0x00 , 0x40} };
			objects_offset = FindOffset(sigv, 2);
		}
		return objects_offset;
	}
	uint32_t GetGNames() {
		if (!names_offset) {
			static std::vector<BYTE> sigv[] = { { 0x48, 0x8B, 0x05, 0x00, 0x00, 0x00, 0x00, 0x48, 0x85, 0xC0, 0x75, 0x5F }, { 0x48, 0x8D, 0x15, 0x00, 0x00, 0x00, 0x00, 0xEB, 0x00, 0x48, 0x8D, 0x0D }, {0x48, 0x8b, 0x3d, 0x00, 0x00, 0x00, 0x00, 0x48, 0x85, 0xff, 0x75, 0x3c} };
			names_offset = FindOffset(sigv, 3);
		}
		return names_offset;
	}

	uint32_t GetNamePool() {
		if (!pool_offset) {
			static std::vector<BYTE> sigv[] = { { 0x48, 0x8d, 0x35, 0x00, 0x00, 0x00, 0x00, 0xeb, 0x16 } };
			pool_offset = FindOffset(sigv, 1);
		}
		return pool_offset;
	}
};

int main(int argc, const char* argv[]) {
	if (argc != 2) {
		printf("Usage: .\\%ls \"pathToUE4Game\"\n", std::filesystem::path(argv[0]).filename().c_str());
		return 0;
	}
	Updater* updater = new Updater;
	if (!updater->LoadFile(argv[1])) return 0;
	printf("%-17s|\t%.7X\n", "UWorld", updater->GetGWorld());
	Beep(370, 100);
	printf("%-17s|\t%.7X\n", "TUObjectArray", updater->GetGObjects());
	Beep(370, 100);
	printf("%-17s|\t%.7X\n", "TNameEntryArray", updater->GetGNames());
	Beep(370, 100);
	printf("%-17s|\t%.7X\n", "FNamePool", updater->GetNamePool());
	Beep(370, 100);
	delete updater;

	return 1;
}